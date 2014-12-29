/*
 * Mapper.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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


#ifndef MAPPER_H_
#define MAPPER_H_

#include <QtGlobal>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <tr1/memory>

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Paint.h"
#include "Mapping.h"

#include "Util.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

class VertexGraphicsItem;

class ShapeGraphicsItem : public QGraphicsItem
{
  Q_DECLARE_TR_FUNCTIONS(ShapeGraphicsItem)

public:
  ShapeGraphicsItem(Mapping::ptr mapping, bool output=true);
  virtual ~ShapeGraphicsItem() {}

  // TODO: dangereux: confusion possible entre shape() et getShape()...
  MShape::ptr getShape() const { return _shape; }

  Mapping::ptr getMapping() const { return _mapping; }

  bool isOutput() const { return _output; }

  virtual QRectF boundingRect() const = 0;
//  virtual QPainterPath shape() const;
//  virtual void paint(QPainter *painter,
//                     const QStyleOptionGraphicsItem *option, QWidget *widget);

  virtual bool sceneEventFilter(QGraphicsItem * watched, QEvent * event);

  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

protected:
  virtual void _createVertices();
  virtual void _syncShape();
  virtual void _syncVertices();

  Mapping::ptr _mapping;
  MShape::ptr _shape;
  bool _output;
};

/// Graphics item for vertices / control points.
class VertexGraphicsItem : public QGraphicsEllipseItem
{
  Q_DECLARE_TR_FUNCTIONS(VertexGraphicsItem)

public:
  VertexGraphicsItem(int index) : _index(index) {
    setFlags(ItemIsMovable | ItemIsSelectable);
  }
  virtual ~VertexGraphicsItem() {}

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget)
  {
    Q_UNUSED(widget);
    Util::drawControlsVertex(painter, QPointF(0,0), (option->state & QStyle::State_Selected));
  }

protected:
  int _index;
};

/// Graphics item for colored polygons (eg. quad, triangle).
class PolygonColorGraphicsItem : public ShapeGraphicsItem
{
public:
  PolygonColorGraphicsItem(Mapping::ptr mapping, bool output=true) : ShapeGraphicsItem(mapping, output) {}
  virtual ~PolygonColorGraphicsItem() {}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget);
};

/// Graphics item for colored polygons (eg. quad, triangle).
class EllipseColorGraphicsItem : public ShapeGraphicsItem
{
public:
  EllipseColorGraphicsItem(Mapping::ptr mapping, bool output=true) : ShapeGraphicsItem(mapping, output) {}
  virtual ~EllipseColorGraphicsItem() {}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget);
};

/// Abstract class for texture graphics items.
class TextureGraphicsItem : public ShapeGraphicsItem
{
public:
  TextureGraphicsItem(Mapping::ptr mapping, bool output=true);

  virtual ~TextureGraphicsItem() {}

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget);

  virtual void _doDraw(QPainter* painter, bool selected) = 0;
  virtual void _doDrawControls(QPainter* painter) { Q_UNUSED(painter); }
  void _preDraw(QPainter* painter);
  void _postDraw(QPainter* painter);

protected:
  std::tr1::shared_ptr<TextureMapping> _textureMapping;
  std::tr1::shared_ptr<Texture> _texture;
  std::tr1::shared_ptr<MShape> _inputShape;
};

/// Graphics item for textured polygons (eg. triangles).
class PolygonTextureGraphicsItem : public TextureGraphicsItem
{
public:
  PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : TextureGraphicsItem(mapping, output) {}
  virtual ~PolygonTextureGraphicsItem(){}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;
};

/// Graphics item for textured triangles.
class TriangleTextureGraphicsItem : public PolygonTextureGraphicsItem
{
public:
  TriangleTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : PolygonTextureGraphicsItem(mapping, output) {}
  virtual ~TriangleTextureGraphicsItem(){}

  virtual void _doDraw(QPainter* painter, bool selected);
  virtual void _doDrawControls(QPainter* painter) {
    painter->setPen(MM::SHAPE_STROKE);
    Polygon* poly = static_cast<Polygon*>(_shape.get());
    Q_ASSERT(poly);
    painter->drawPolygon(mapFromScene(poly->toPolygon()));
  }
};

/**
 * A way to draw on some kind of shape.
 *
 * This is an abstract class for specific ways to draw
 * a mapping ie. a shape applied to a paint.
 *
 * Each mapping (ie. shape x paint combination) that is possible in
 * this software are implemented using a child of this
 * class.
 */
class Mapper : public QObject
{
  Q_OBJECT

public:
  typedef std::tr1::shared_ptr<Mapper> ptr;

protected:
  /// Constructor. A mapper applies to a mapping.
  Mapper(Mapping::ptr mapping);
  virtual ~Mapper();

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor();

  virtual QGraphicsItem* getGraphicsItem() {
    return _graphicsItem;
  }
  virtual QGraphicsItem* getInputGraphicsItem() {
    return _inputGraphicsItem;
  }

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void updateShape(MShape* shape) { Q_UNUSED(shape); }
  virtual void updatePaint() {}

signals:
  void valueChanged();

protected:
  Mapping::ptr _mapping;
  QtAbstractPropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
  QtProperty* _topItem;
  QtProperty* _outputItem;

  std::map<QtProperty*, std::pair<MShape*, int> > _propertyToVertex;

  QGraphicsItem* _graphicsItem;
  QGraphicsItem* _inputGraphicsItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  MShape::ptr outputShape;

  virtual void _buildShapeProperty(QtProperty* shapeItem, MShape* shape);
  virtual void _updateShapeProperty(QtProperty* shapeItem, MShape* shape);
};

/**
 * Draws a color (abstract class).
 */
class ColorMapper : public Mapper
{
  Q_OBJECT

public slots:
  virtual void updatePaint();

protected:
  ColorMapper(Mapping::ptr mapping);
  virtual ~ColorMapper() {}

protected:
  std::tr1::shared_ptr<Color> color;
};

class PolygonColorMapper : public ColorMapper
{
  Q_OBJECT

public:
  PolygonColorMapper(Mapping::ptr mapping) : ColorMapper(mapping) {
    _graphicsItem = new PolygonColorGraphicsItem(mapping, true);
  }
  virtual ~PolygonColorMapper() {}
};

//class MeshColorMapper : public ColorMapper
//{
//  Q_OBJECT
//
//public:
//  MeshColorMapper(Mapping::ptr mapping);
//  virtual ~MeshColorMapper() {}
//
//  virtual void draw(QPainter* painter);
//  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);
//
//public slots:
//  virtual void setValue(QtProperty* property, const QVariant& value);
//
//private:
//  QtVariantProperty* _meshItem;
//};

class EllipseColorMapper : public ColorMapper
{
  Q_OBJECT

public:
  EllipseColorMapper(Mapping::ptr mapping) : ColorMapper(mapping) {
    _graphicsItem = new EllipseColorGraphicsItem(mapping, true);
  }

  virtual ~EllipseColorMapper() {}
};

/**
 * Draws a texture (abstract class).
 */
class TextureMapper : public Mapper
{
  Q_OBJECT

public:
  TextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~TextureMapper() {}

//  /**
//   * This method will call the _doDraw(QPainter*) method to actually perform the drawing.
//   * of the mapping.
//   */
//  virtual void draw(QPainter* painter);
//  virtual void drawInput(QPainter* painter);
//
//  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0) = 0;
//  virtual void drawInputControls(QPainter* painter, const QList<int>* selectedVertices=0) = 0;

public slots:
  virtual void updateShape(MShape* shape);
  virtual void updatePaint();

//protected:
//  virtual void _doDraw(QPainter* painter) = 0;
//  void _preDraw(QPainter* painter);
//  void _postDraw(QPainter* painter);

protected:
  QtProperty* _inputItem;
  QtVariantProperty* _meshItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  std::tr1::shared_ptr<TextureMapping> textureMapping;
  std::tr1::shared_ptr<Texture> texture;
  std::tr1::shared_ptr<MShape> inputShape;
};

class PolygonTextureMapper : public TextureMapper
{
  Q_OBJECT

public:
  PolygonTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping) : TextureMapper(mapping) {}
  virtual ~PolygonTextureMapper() {}
};

class TriangleTextureMapper : public PolygonTextureMapper
{
  Q_OBJECT

public:
  TriangleTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~TriangleTextureMapper() {}
};

class MeshTextureMapper : public PolygonTextureMapper
{
  Q_OBJECT

public:
  MeshTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~MeshTextureMapper() {}

  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);
  virtual void drawInputControls(QPainter* painter, const QList<int>* selectedVertices=0);

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

protected:
  virtual void _doDraw(QPainter* painter);

private:
  QtVariantProperty* _meshItem;
};

class EllipseTextureMapper : public PolygonTextureMapper {
  Q_OBJECT

public:
  EllipseTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~EllipseTextureMapper() {}

  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);
  virtual void drawInputControls(QPainter* painter, const QList<int>* selectedVertices=0);

protected:
  virtual void _doDraw(QPainter* painter);
  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};


#endif /* MAPPER_H_ */
