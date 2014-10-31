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

class VertexGraphicsItem;

class ShapeGraphicsItem : public QGraphicsItem
{
  Q_DECLARE_TR_FUNCTIONS(ShapeGraphicsItem)

public:
  ShapeGraphicsItem(Mapping::ptr mapping, bool output=true)
    : _mapping(mapping), _output(output)
  {
    _shape = output ? _mapping->getShape() : _mapping->getInputShape();

    setFlags(ItemIsMovable | ItemIsSelectable);

    _createVertices();
  }
  virtual ~ShapeGraphicsItem() {}

  // TODO: dangereux: confusion possible entre shape() et getShape()...
  MShape::ptr getShape() const { return _shape; }

  Mapping::ptr getMapping() const { return _mapping; }

  bool isOutput() const { return _output; }

  virtual QRectF boundingRect() const = 0;
//  virtual QPainterPath shape() const;
//  virtual void paint(QPainter *painter,
//                     const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
  virtual void _createVertices();

  Mapping::ptr _mapping;
  MShape::ptr _shape;
  bool _output;
};

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
    const QPointF& vertex = ((ShapeGraphicsItem*)parentItem())->getShape()->getVertex(_index);
    Util::drawControlsVertex(painter, vertex, (option->state & QStyle::State_Selected));
  }


protected:
  int _index;
};

class PolygonColorGraphicsItem : public ShapeGraphicsItem
{
public:
  PolygonColorGraphicsItem(Mapping::ptr mapping, bool output=true) : ShapeGraphicsItem(mapping, output) {}
  virtual ~PolygonColorGraphicsItem() {}


  virtual QRectF boundingRect() const {
    return ((Polygon*)_shape.get())->toPolygon().boundingRect();
  }

  virtual QPainterPath shape() const {
    QPainterPath path;
    path.addPolygon(((Polygon*)_shape.get())->toPolygon());
    return path;
  }

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget)
  {
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::red);
    painter->setBrush(((Color*)_mapping->getPaint().get())->getColor());

    // Draw shape as polygon.
    Polygon* poly = (Polygon*)_shape.get();
    painter->drawPolygon(poly->toPolygon());

    if (option->state & QStyle::State_Selected)
    {
      QList<int> dummySelectedVertices;
      Util::drawControlsPolygon(painter, &dummySelectedVertices, *poly);
    }
  }
};

class TextureGraphicsItem : public ShapeGraphicsItem
{
public:
  TextureGraphicsItem(Mapping::ptr mapping, bool output=true) : ShapeGraphicsItem(mapping, output) {
    _textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
    Q_CHECK_PTR(_textureMapping);

    _texture = std::tr1::static_pointer_cast<Texture>(_textureMapping->getPaint());
    Q_CHECK_PTR(_texture);

    _inputShape = std::tr1::static_pointer_cast<MShape>(_textureMapping->getInputShape());
    Q_CHECK_PTR(_inputShape);
  }

  virtual ~TextureGraphicsItem() {}

  virtual void paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option, QWidget *widget)
  {
    if (_output)
    {
      // Prepare drawing.
      _preDraw(painter);

      // Perform the actual mapping (done by subclasses).
      _doDraw(painter, option->state & QStyle::State_Selected);

      // End drawing.
      _postDraw(painter);
    }

    if (option->state & QStyle::State_Selected)
      _drawControls(painter);
  }

  virtual void _doDraw(QPainter* painter, bool selected) = 0;
  void _preDraw(QPainter* painter)
  {
    painter->beginNativePainting();

    // Only works for similar shapes.
    // TODO:remettre
    //Q_ASSERT( _inputShape->nVertices() == outputShape->nVertices());

    // Project source texture and sent it to destination.
    _texture->update();

    glEnable (GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());

    // Copy bits to texture iff necessary.
    _texture->lockMutex();
    if (_texture->bitsHaveChanged())
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        _texture->getWidth(), _texture->getHeight(), 0, GL_RGBA,
        GL_UNSIGNED_BYTE, _texture->getBits());
    }
    _texture->unlockMutex();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  }

  void _postDraw(QPainter* painter)
  {
    glDisable(GL_TEXTURE_2D);

    painter->endNativePainting();
  }

  virtual void _drawControls(QPainter *painter)
  {}

protected:
  std::tr1::shared_ptr<TextureMapping> _textureMapping;
  std::tr1::shared_ptr<Texture> _texture;
  std::tr1::shared_ptr<MShape> _inputShape;
};

class PolygonTextureGraphicsItem : public TextureGraphicsItem
{
public:
  PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : TextureGraphicsItem(mapping, output) {}
  virtual ~PolygonTextureGraphicsItem(){}

  virtual void _drawControls(QPainter* painter)
  {
    std::tr1::shared_ptr<Polygon> outputPoly = std::tr1::static_pointer_cast<Polygon>(_shape);
    QList<int> dummySelectedVertices;
    Util::drawControlsPolygon(painter, &dummySelectedVertices, *outputPoly);
  }
};

class TriangleTextureGraphicsItem : public PolygonTextureGraphicsItem
{
public:
  TriangleTextureGraphicsItem(Mapping::ptr mapping, bool output=true) : PolygonTextureGraphicsItem(mapping, output) {}
  virtual ~TriangleTextureGraphicsItem(){}

  virtual QRectF boundingRect() const {
    return ((Polygon*)_shape.get())->toPolygon().boundingRect();
  }

  virtual QPainterPath shape() const {
    QPainterPath path;
    path.addPolygon(((Polygon*)_shape.get())->toPolygon());
    return path;
  }

  virtual void _doDraw(QPainter* painter, bool selected)
  {
    if (_output)
    {
      glBegin(GL_TRIANGLES);
      {
        for (int i = 0; i < _inputShape->nVertices(); i++)
        {
          Util::setGlTexPoint(*_texture, _inputShape->getVertex(i), _shape->getVertex(i));
        }
      }
      glEnd();
    }
  }
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

class MeshColorMapper : public ColorMapper
{
  Q_OBJECT

public:
  MeshColorMapper(Mapping::ptr mapping);
  virtual ~MeshColorMapper() {}

  virtual void draw(QPainter* painter);
  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseColorMapper : public ColorMapper
{
  Q_OBJECT

public:
  EllipseColorMapper(Mapping::ptr mapping);
  virtual ~EllipseColorMapper() {}

  virtual void draw(QPainter* painter);
  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);
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

  /**
   * This method will call the _doDraw(QPainter*) method to actually perform the drawing.
   * of the mapping.
   */
  virtual void draw(QPainter* painter);
  virtual void drawInput(QPainter* painter);

  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0) = 0;
  virtual void drawInputControls(QPainter* painter, const QList<int>* selectedVertices=0) = 0;

public slots:
  virtual void updateShape(MShape* shape);
  virtual void updatePaint();

protected:
  virtual void _doDraw(QPainter* painter) = 0;
  void _preDraw(QPainter* painter);
  void _postDraw(QPainter* painter);

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

  virtual void drawControls(QPainter* painter, const QList<int>* selectedVertices=0);
  virtual void drawInputControls(QPainter* painter, const QList<int>* selectedVertices=0);
};

class TriangleTextureMapper : public PolygonTextureMapper
{
  Q_OBJECT

public:
  TriangleTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~TriangleTextureMapper() {}

protected:
  virtual void _doDraw(QPainter* painter);
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
