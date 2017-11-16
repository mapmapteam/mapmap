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

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "Shapes.h"

#include "Paint.h"
#include "Mapping.h"
#include "MapperGLCanvas.h"

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
  bool isMappingVisible() const;

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

/**
 * Graphics item for textured color.
 */
class MeshColorGraphicsItem : public PolygonColorGraphicsItem
{
public:
  MeshColorGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~MeshColorGraphicsItem(){}

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
  QWeakPointer<MShape> _inputShape;

	QSharedPointer<Texture> _getTexture();
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

/**
 * Graphics item for textured mesh.
 * The drawing technique recursively subdivides the quad to approximate projective mapping and thus
 * avoiding artifacts on the diagonals. Subdivided structure is cached to increase performance.
 * Source: Oliveira, M. "Correcting Texture Mapping Errors Introduced by Graphics Hardware"
 */
class MeshTextureGraphicsItem : public PolygonTextureGraphicsItem
{
  // Internal use (cache). A structure consisting of the input and output quads of mapping.
  struct CacheQuadMapping {
    Quad::ptr input;
    Quad::ptr output;
  };

  // Internal use (cache). Contains a parent mapping and all its sub-mappings.
  struct CacheQuadItem {
    CacheQuadMapping parent;
    QList<CacheQuadMapping> subQuads;
  };
public:
  MeshTextureGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~MeshTextureGraphicsItem(){}

  virtual void _doDrawOutput(QPainter* painter);

private:
  /**
   * Builds cache item recursively using the technique described in
   * Oliveira, M. "Correcting Texture Mapping Errors Introduced by Graphics Hardware"
   */
  void _buildCacheQuadItem(CacheQuadItem& item, const Quad::ptr& inputQuad, const Quad::ptr& outputQuad,
                           float outputArea, float inputThreshold = 0.0001f, float outputThreshold = 0.001f,
                           int minArea=MM::MESH_SUBDIVISION_MIN_AREA, int maxDepth=-1);

  // Help function that returns four equal-size sub-quads from a quad.
  QList<Quad::ptr> _split(const Quad& quad);

  // Contains the current cache.
  QVector<QVector<CacheQuadItem> > _cachedQuadItems;
  int _nHorizontalQuads;
  int _nVerticalQuads;

  // True iff shape was being grabbed last time _buildCacheQuadItem() was called.
  bool _wasGrabbing;
};

/// Graphics item for textured mesh.
class EllipseTextureGraphicsItem : public TextureGraphicsItem
{
  static const int N_QUARTERS = 4;

  class DrawingData {
  public:
    QPointF center;
    QPointF controlCenter;
    float   horizontalRadius;
    float   verticalRadius;
    float   rotation;
    float   quarterAngles[N_QUARTERS];

    DrawingData(const QSharedPointer<Ellipse>& ellipse);
    float getSpanInQuarter(int quarter) const;
    void setPointOfEllipseAtAngle(QPointF& point, float circularAngle);
  };

public:
  EllipseTextureGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~EllipseTextureGraphicsItem(){}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void _doDrawOutput(QPainter* painter);

  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};

}

#endif
