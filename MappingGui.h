/*
 * MappingGui.h
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


#ifndef MAPPING_GUI_
#define MAPPING_GUI_

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

/**
 * An object responsible to paint control points for a specific ShapeGraphicsItem.
 */
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

/// Control painter for polygons.
class PolygonControlPainter : public ShapeControlPainter
{
public:
  PolygonControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~PolygonControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

/// Control painter for ellipses.
class EllipseControlPainter : public ShapeControlPainter
{
public:
  EllipseControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~EllipseControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

/// Control painter for meshes.
class MeshControlPainter : public ShapeControlPainter
{
public:
  MeshControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~MeshControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter);
};

/**
 * This is the "view" side of the Mapping class (model). It contains the graphic items for
 * both input and output as well as the properties editor.
 */
class MappingGui : public QObject
{
  Q_OBJECT

public:
  typedef QSharedPointer<MappingGui> ptr;

protected:
  /// Constructor. A mapper applies to a mapping.
  MappingGui(Mapping::ptr mapping);

public:
  virtual ~MappingGui() {}

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor() { return _propertyBrowser.data(); }

  /// Returns the output/destination ShapeGraphicsItem.
  virtual ShapeGraphicsItem::ptr getGraphicsItem() const { return _graphicsItem; }

  /// Returns the input/source ShapeGraphicsItem.
  virtual ShapeGraphicsItem::ptr getInputGraphicsItem() { return _inputGraphicsItem; }

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void updateShape(MShape* shape);

signals:
  void valueChanged();

protected:
  Mapping::ptr _mapping;

  QSharedPointer<QtTreePropertyBrowser> _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;

  QtProperty* _topItem;
  QtVariantProperty* _opacityItem;
  QtProperty* _outputItem;

  std::map<QtProperty*, std::pair<MShape*, int> > _propertyToVertex;

  ShapeGraphicsItem::ptr _graphicsItem;
  ShapeGraphicsItem::ptr _inputGraphicsItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  MShape::ptr outputShape;

  virtual void _buildShapeProperty(QtProperty* shapeItem, MShape* shape);
  virtual void _updateShapeProperty(QtProperty* shapeItem, MShape* shape);
};

/// Parent class for color -> color mappings.
class ColorMappingGui : public MappingGui
{
  Q_OBJECT

protected:
  ColorMappingGui(Mapping::ptr mapping);
  virtual ~ColorMappingGui() {}

protected:
  QSharedPointer<Color> color;
};

class PolygonColorMappingGui : public ColorMappingGui
{
  Q_OBJECT

public:
  PolygonColorMappingGui(Mapping::ptr mapping) : ColorMappingGui(mapping) {
    _graphicsItem.reset(new PolygonColorGraphicsItem(mapping, true));
  }
  virtual ~PolygonColorMappingGui() {}
};

class EllipseColorMappingGui : public ColorMappingGui
{
  Q_OBJECT

public:
  EllipseColorMappingGui(Mapping::ptr mapping) : ColorMappingGui(mapping) {
    _graphicsItem.reset(new EllipseColorGraphicsItem(mapping, true));
  }

  virtual ~EllipseColorMappingGui() {}
};

/// Parent class for texture -> texture mapping.
class TextureMappingGui : public MappingGui
{
  Q_OBJECT

public:
  TextureMappingGui(QSharedPointer<TextureMapping> mapping);
  virtual ~TextureMappingGui() {}

public slots:
  virtual void updateShape(MShape* shape);

protected:
  QtProperty* _inputItem;
  QtVariantProperty* _meshItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  QWeakPointer<TextureMapping> textureMapping;
  QWeakPointer<Texture> texture;
  QWeakPointer<MShape> inputShape;
};

class PolygonTextureMappingGui : public TextureMappingGui
{
  Q_OBJECT

public:
  PolygonTextureMappingGui(QSharedPointer<TextureMapping> mapping) : TextureMappingGui(mapping) {}
  virtual ~PolygonTextureMappingGui() {}
};

class TriangleTextureMappingGui : public PolygonTextureMappingGui
{
  Q_OBJECT

public:
  TriangleTextureMappingGui(QSharedPointer<TextureMapping> mapping);
  virtual ~TriangleTextureMappingGui() {}
};

class MeshTextureMappingGui : public PolygonTextureMappingGui
{
  Q_OBJECT

public:
  MeshTextureMappingGui(QSharedPointer<TextureMapping> mapping);
  virtual ~MeshTextureMappingGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseTextureMappingGui : public PolygonTextureMappingGui {
  Q_OBJECT

public:
  EllipseTextureMappingGui(QSharedPointer<TextureMapping> mapping);
  virtual ~EllipseTextureMappingGui() {}

protected:
  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};


#endif /* MAPPER_H_ */
