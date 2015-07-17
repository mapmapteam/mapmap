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

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Paint.h"
#include "Mapping.h"
#include "MapperGLCanvas.h"

#include "Util.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

class MapperGLCanvas;

class ShapeGraphicsItem;

class ShapeControlPainter
{
public:
  typedef QSharedPointer<ShapeControlPainter> ptr;

  ShapeControlPainter(ShapeGraphicsItem* shapeItem);
  virtual ~ShapeControlPainter() {}

  MShape::ptr getShape() const;

  virtual void paint(QPainter *painter, const QList<int>& selectedVertices = QList<int>());

protected:
  virtual void _paintShape(QPainter *painter) = 0;
  virtual void _paintVertices(QPainter *painter, const QList<int>& selectedVertices = QList<int>());

  ShapeGraphicsItem* _shapeItem;
};

class PolygonControlPainter : public ShapeControlPainter
{
public:
  PolygonControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~PolygonControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

class EllipseControlPainter : public ShapeControlPainter
{
public:
  EllipseControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~EllipseControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

class MeshControlPainter : public ShapeControlPainter
{
public:
  MeshControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~MeshControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

class ShapeGraphicsItem : public QGraphicsItem
{
  Q_DECLARE_TR_FUNCTIONS(ShapeGraphicsItem)

protected:
  ShapeGraphicsItem(Mapping::ptr mapping, bool output=true);
public:
  virtual ~ShapeGraphicsItem() {}

public:

  // TODO: dangereux: confusion possible entre shape() et getShape()...
  MShape::ptr getShape() const { return _shape.toStrongRef(); }

  Mapping::ptr getMapping() const { return _mapping.toStrongRef(); }

  ShapeControlPainter::ptr getControlPainter() { return _controlPainter; }

  bool isOutput() const { return _output; }
  MapperGLCanvas* getCanvas() const;

  bool isMappingCurrent() const;
  bool isMappingVisible() const { return getMapping()->isVisible(); }

  virtual QRectF boundingRect() const { return shape().boundingRect(); }
//  virtual QPainterPath shape() const;
//  virtual void paint(QPainter *painter,
//                     const QStyleOptionGraphicsItem *option, QWidget *widget);

//  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
  virtual void _doPaint(QPainter *painter, const QStyleOptionGraphicsItem *option) = 0;
  virtual void _prePaint(QPainter *painter, const QStyleOptionGraphicsItem *option)
  { Q_UNUSED(painter); Q_UNUSED(option); }
  virtual void _postPaint(QPainter *painter, const QStyleOptionGraphicsItem *option)
  { Q_UNUSED(painter); Q_UNUSED(option); }

public:
  virtual void _doPaintControls(QPainter *painter, const QStyleOptionGraphicsItem *option);

  // Utility function: returns a stroke with rescaled width such that the stroke appears
  // invariant to the zoom level (to be used in _doPaintControls() method).
  QPen _getRescaledShapeStroke(bool innerStroke=false);
protected:
  QWeakPointer<Mapping> _mapping;
  QWeakPointer<MShape> _shape;
  ShapeControlPainter::ptr  _controlPainter;
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
  PolygonColorGraphicsItem(Mapping::ptr mapping, bool output=true)
    : ColorGraphicsItem(mapping, output) {
    _controlPainter.reset(new PolygonControlPainter(this));
  }
  virtual ~PolygonColorGraphicsItem() {}

  virtual QPainterPath shape() const;

protected:
  virtual void _doPaint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option);
public:
  void _doPaintControls(QPainter* painter, const QStyleOptionGraphicsItem *option);
};

/// Graphics item for colored polygons (eg. quad, triangle).
class EllipseColorGraphicsItem : public ColorGraphicsItem
{
public:
  EllipseColorGraphicsItem(Mapping::ptr mapping, bool output=true)
    : ColorGraphicsItem(mapping, output) {
      _controlPainter.reset(new EllipseControlPainter(this));
    }
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
  PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : TextureGraphicsItem(mapping, output) {
    _controlPainter.reset(new PolygonControlPainter(this));
  }
  virtual ~PolygonTextureGraphicsItem(){}

public:
  virtual void _doPaintControls(QPainter* painter, const QStyleOptionGraphicsItem *option);

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
  MeshTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : PolygonTextureGraphicsItem(mapping, output) {
    _controlPainter.reset(new MeshControlPainter(this));
  }
  virtual ~MeshTextureGraphicsItem(){}

  virtual void _doPaintControls(QPainter* painter, const QStyleOptionGraphicsItem *option);
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
  EllipseTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : TextureGraphicsItem(mapping, output) {
    _controlPainter.reset(new EllipseControlPainter(this));
  }
  virtual ~EllipseTextureGraphicsItem(){}

  virtual QPainterPath shape() const;
  virtual QRectF boundingRect() const;

  virtual void _doDrawOutput(QPainter* painter);

  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
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
  typedef QSharedPointer<Mapper> ptr;

protected:
  /// Constructor. A mapper applies to a mapping.
  Mapper(Mapping::ptr mapping);

public:
  virtual ~Mapper();

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor();

  virtual ShapeGraphicsItem* getGraphicsItem() {
    return _graphicsItem;
  }
  virtual ShapeGraphicsItem* getInputGraphicsItem() {
    return _inputGraphicsItem;
  }

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void updateShape(MShape* shape) { Q_UNUSED(shape); }
  virtual void updatePaint();

signals:
  void valueChanged();

protected:
  Mapping::ptr _mapping;

  QtTreePropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;

  QtProperty* _topItem;
  QtVariantProperty* _opacityItem;
  QtProperty* _outputItem;

  std::map<QtProperty*, std::pair<MShape*, int> > _propertyToVertex;

  ShapeGraphicsItem* _graphicsItem;
  ShapeGraphicsItem* _inputGraphicsItem;

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

protected:
  ColorMapper(Mapping::ptr mapping);
  virtual ~ColorMapper() {}

protected:
  QSharedPointer<Color> color;
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
  TextureMapper(QSharedPointer<TextureMapping> mapping);
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

//protected:
//  virtual void _doDraw(QPainter* painter) = 0;
//  void _preDraw(QPainter* painter);
//  void _postDraw(QPainter* painter);

protected:
  QtProperty* _inputItem;
  QtVariantProperty* _meshItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  QWeakPointer<TextureMapping> textureMapping;
  QWeakPointer<Texture> texture;
  QWeakPointer<MShape> inputShape;
};

class PolygonTextureMapper : public TextureMapper
{
  Q_OBJECT

public:
  PolygonTextureMapper(QSharedPointer<TextureMapping> mapping) : TextureMapper(mapping) {}
  virtual ~PolygonTextureMapper() {}
};

class TriangleTextureMapper : public PolygonTextureMapper
{
  Q_OBJECT

public:
  TriangleTextureMapper(QSharedPointer<TextureMapping> mapping);
  virtual ~TriangleTextureMapper() {}
};

class MeshTextureMapper : public PolygonTextureMapper
{
  Q_OBJECT

public:
  MeshTextureMapper(QSharedPointer<TextureMapping> mapping);
  virtual ~MeshTextureMapper() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseTextureMapper : public PolygonTextureMapper {
  Q_OBJECT

public:
  EllipseTextureMapper(QSharedPointer<TextureMapping> mapping);
  virtual ~EllipseTextureMapper() {}

protected:
  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};


#endif /* MAPPER_H_ */
