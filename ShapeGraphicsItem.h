/*
 * ShapeGraphicsItem.h
 *
 * (c) 2015 Sofian Audry -- info(@)sofianaudry(.)com
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

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Paint.h"
#include "Mapping.h"
#include "MapperGLCanvas.h"

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
  ShapeGraphicsItem(Mapping::ptr mapping, bool output=true);
public:
  virtual ~ShapeGraphicsItem() {}

public:
  /// Returns the MShape (depends on mapping and whether this is an input or output).
  MShape::ptr getShape() const { return _shape.toStrongRef(); }

  /// Returns the mapping.
  Mapping::ptr getMapping() const { return _mapping.toStrongRef(); }

  /// Returns the control painter (see below).
  QSharedPointer<ShapeControlPainter> getControlPainter() { return _controlPainter; }

  /// Return whether the item is in the destination/output (true) or source/input (false).
  bool isOutput() const { return _output; }

  /// Returns pointer to GL canvas.
  MapperGLCanvas* getCanvas() const;

  /// Returns whether the mapping this shape is associated with is currently selected.
  bool isMappingCurrent() const;

  /// Returns whether the mapping this shape is associated should be visible.
  bool isMappingVisible() const { return getMapping()->isVisible(); }

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

public:
  /**
   *  Utility function: returns a stroke with rescaled width such that the stroke appears
   *  invariant to the zoom level (to be used in paint functions).
   */
  QPen getRescaledShapeStroke(bool innerStroke=false);

protected:
  QWeakPointer<Mapping> _mapping;
  QWeakPointer<MShape> _shape;
  QSharedPointer<ShapeControlPainter>  _controlPainter;
  bool _output;
};

class ColorGraphicsItem : public ShapeGraphicsItem
{
protected:
  ColorGraphicsItem(Mapping::ptr mapping, bool output=true)
    : ShapeGraphicsItem(mapping, output) {}
public:
  virtual ~ColorGraphicsItem() {}

protected:
  virtual void _prePaint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option);
};

/// Graphics item for colored polygons (eg. quad, triangle).
class PolygonColorGraphicsItem : public ColorGraphicsItem
{
public:
  PolygonColorGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~PolygonColorGraphicsItem() {}

  virtual QPainterPath shape() const;

protected:
  virtual void _doPaint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option);
};

/// Graphics item for colored polygons (eg. quad, triangle).
class EllipseColorGraphicsItem : public ColorGraphicsItem
{
public:
  EllipseColorGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~EllipseColorGraphicsItem() {}

  virtual QPainterPath shape() const;

protected:
  virtual void _doPaint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option);
};

/// Abstract class for texture graphics items.
class TextureGraphicsItem : public ShapeGraphicsItem
{
public:
  TextureGraphicsItem(Mapping::ptr mapping, bool output=true);

  virtual ~TextureGraphicsItem() {}

protected:
  virtual void _doPaint(QPainter *painter, const QStyleOptionGraphicsItem *option);
  void _prePaint(QPainter* painter, const QStyleOptionGraphicsItem *option);
  void _postPaint(QPainter* painter, const QStyleOptionGraphicsItem *option);

  virtual void _doDrawOutput(QPainter* painter) = 0;
  virtual void _doDrawInput(QPainter* painter);

protected:
  QWeakPointer<TextureMapping> _textureMapping;
  QWeakPointer<Texture> _texture;
  QWeakPointer<MShape> _inputShape;
};

/// Graphics item for textured polygons (eg. triangles).
class PolygonTextureGraphicsItem : public TextureGraphicsItem
{
public:
  PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~PolygonTextureGraphicsItem(){}

public:
  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;
};

/// Graphics item for textured triangles.
class TriangleTextureGraphicsItem : public PolygonTextureGraphicsItem
{
public:
  TriangleTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : PolygonTextureGraphicsItem(mapping, output) {}
  virtual ~TriangleTextureGraphicsItem(){}

  virtual void _doDrawOutput(QPainter* painter);

};

/// Graphics item for textured mesh.
class MeshTextureGraphicsItem : public PolygonTextureGraphicsItem
{
public:
  MeshTextureGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~MeshTextureGraphicsItem(){}

  virtual void _doDrawOutput(QPainter* painter);

private:
  // Draws quad recursively using the technique described in Oliveira, M. "Correcting Texture Mapping Errors Introduced by Graphics Hardware"
  void _drawQuad(const Texture& texture, const Quad& inputQuad, const Quad& outputQuad, float outputArea, float inputThreshold = 0.0001f, float outputThreshold = 0.001f);
  QList<Quad> _split(const Quad& quad);
};

/// Graphics item for textured mesh.
class EllipseTextureGraphicsItem : public TextureGraphicsItem
{
public:
  EllipseTextureGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~EllipseTextureGraphicsItem(){}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void _doDrawOutput(QPainter* painter);

  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};

#endif
