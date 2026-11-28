/*
 * ShapeGraphicsItem.h
 *
 * (c) 2015 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHAPE_GRAPHICS_ITEM_H_
#define SHAPE_GRAPHICS_ITEM_H_

#include <QtGlobal>

#include <QtGui/qopengl.h>

#include <stdlib.h>
#include <stdio.h>

#include "Shapes.h"

#include <QGraphicsItem>
#include <QCoreApplication>
#include "Source.h"
#include "Layer.h"
#include "MapperGLCanvas.h"
#include "ShapeGeometry.h"
#include "ShapeRenderer.h"

namespace mmp {

class MapperGLCanvas;
class ShapeControlPainter;

/**
 * Represents a shape as part of a scene graph in one of the GL canvas.
 * A ShapeGraphicsItem contains a Mapping and is identified as either an
 * input (source) or an output (destination).
 */
class ShapeGraphicsItem : public QGraphicsItem
{
  Q_DECLARE_TR_FUNCTIONS(ShapeGraphicsItem)
public:
  typedef QSharedPointer<ShapeGraphicsItem> ptr;

protected:
  ShapeGraphicsItem(Layer::ptr mapping, bool output=true);
public:
  virtual ~ShapeGraphicsItem() {}

public:
  /// Returns the MShape (depends on mapping and whether this is an input or output).
  MShape::ptr getShape() const { return _shape.toStrongRef(); }

  /// Returns the mapping.
  Layer::ptr getLayer() const { return _mapping.toStrongRef(); }

  /// Returns the control painter (see below).
  QSharedPointer<ShapeControlPainter> getControlPainter() { return _controlPainter; }

  /// Return whether the item is in the destination/output (true) or source/input (false).
  bool isOutput() const { return _output; }

  /// Returns pointer to GL canvas.
  MapperGLCanvas* getCanvas() const;

  /// Returns whether the mapping this shape is associated with is currently selected.
  bool isLayerCurrent() const;

  /// Returns whether the mapping this shape is associated should be visible.
  bool isLayerVisible() const;

  /// Returns the bounding rectangle of this item.
  virtual QRectF boundingRect() const { return shape().boundingRect(); }
//  virtual QPainterPath shape() const;
//  virtual void paint(QPainter *painter,
//                     const QStyleOptionGraphicsItem *option, QWidget *widget);

//  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

  /// Built-in function: paints the shape.
  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
  /// Main paint function that needs to be overriden (called by paint()).
  virtual void _doPaint(QPainter *painter, const QStyleOptionGraphicsItem *option) = 0;

  /// Can be overriden to perform operations *before* _doPaint() is called.
  virtual void _prePaint(QPainter *painter, const QStyleOptionGraphicsItem *option)
  { Q_UNUSED(painter); Q_UNUSED(option); }

  /// Can be overriden to perform operations *after* _doPaint() is called.
  virtual void _postPaint(QPainter *painter, const QStyleOptionGraphicsItem *option)
  { Q_UNUSED(painter); Q_UNUSED(option); }

protected:
  QWeakPointer<Layer> _mapping;
  QWeakPointer<MShape> _shape;
  QSharedPointer<ShapeControlPainter>  _controlPainter;
  bool _output;
};

/// Abstract class for texture graphics items.
class TextureGraphicsItem : public ShapeGraphicsItem
{
public:
  TextureGraphicsItem(Layer::ptr mapping, bool output=true);

  virtual ~TextureGraphicsItem() {}

protected:
  virtual void _doPaint(QPainter *painter, const QStyleOptionGraphicsItem *option);
  void _prePaint(QPainter* painter, const QStyleOptionGraphicsItem *option);
  void _postPaint(QPainter* painter, const QStyleOptionGraphicsItem *option);

  virtual void _doDrawOutput(QPainter* painter) = 0;
  virtual void _doDrawInput(QPainter* painter);

  /// Draws triangles, in the item's coordinates, with the texture bound by _prePaint().
  void _drawTriangles(const QVector<TexturedVertex>& vertices);

protected:
  QWeakPointer<TextureLayer> _textureMapping;
  QWeakPointer<MShape> _inputShape;

  /// From the item's coordinates to OpenGL's clip coordinates, set by _prePaint().
  QMatrix4x4 _viewMatrix;

  /// Opacity of the texture, set by _prePaint().
  float _opacity = 1.0f;

	QSharedPointer<Texture> _getTexture();
};

/// Graphics item for textured polygons (eg. triangles).
class PolygonTextureGraphicsItem : public TextureGraphicsItem
{
public:
  PolygonTextureGraphicsItem(Layer::ptr mapping, bool output=true);
  virtual ~PolygonTextureGraphicsItem(){}

public:
  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;
};

/// Graphics item for textured triangles.
class TriangleTextureGraphicsItem : public PolygonTextureGraphicsItem
{
public:
  TriangleTextureGraphicsItem(Layer::ptr mapping, bool output=true) : PolygonTextureGraphicsItem(mapping, output) {}
  virtual ~TriangleTextureGraphicsItem(){}

  virtual void _doDrawOutput(QPainter* painter);

};

/**
 * Graphics item for textured mesh.
 * The drawing technique recursively subdivides the quad to approximate projective mapping and thus
 * avoiding artifacts on the diagonals. Subdivided structure is cached to increase performance.
 * Source: Oliveira, M. "Correcting Texture Mapping Errors Introduced by Graphics Hardware"
 */
class MeshTextureGraphicsItem : public PolygonTextureGraphicsItem
{
  // Internal use (cache). The source and destination polygons of a quad of the mesh,
  // in scene coordinates, and the smaller quads that draw it.
  struct CacheQuadItem {
    QPolygonF input;
    QPolygonF output;
    QVector<QuadCorners> subQuads;
  };
public:
  MeshTextureGraphicsItem(Layer::ptr mapping, bool output=true);
  virtual ~MeshTextureGraphicsItem(){}

  virtual void _doDrawOutput(QPainter* painter);

private:
  // Contains the current cache.
  QVector<QVector<CacheQuadItem> > _cachedQuadItems;
  int _nHorizontalQuads;
  int _nVerticalQuads;

  // The triangles that draw the mesh, rebuilt when a quad or the texture changes.
  QVector<TexturedVertex> _vertices;

  // The place of the texture when _vertices was built.
  QRectF _verticesTextureRect;

  // True iff shape was being grabbed when the mesh was last drawn.
  bool _wasGrabbing;

  // True iff the two meshes had different grids when last drawn, so that it is said once.
  bool _gridsDiffered;
};

/// Graphics item for textured mesh.
class EllipseTextureGraphicsItem : public TextureGraphicsItem
{
public:
  EllipseTextureGraphicsItem(Layer::ptr mapping, bool output=true);
  virtual ~EllipseTextureGraphicsItem(){}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void _doDrawOutput(QPainter* painter);

};

}

#endif
