/*
 * LayerGui.h
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


#ifndef LAYER_GUI_
#define LAYER_GUI_

#include <QtGlobal>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Source.h"
#include "Layer.h"
#include "MappingManager.h"

#include "MapperGLCanvas.h"

#include "ShapeGraphicsItem.h"
#include "ShapeControlPainter.h"

#include "Util.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

namespace mmp {

class MainWindow;

/**
 * This is the "view" side of the Mapping class (model). It contains the graphic items for
 * both input and output as well as the properties editor.
 */
class LayerGui : public QObject
{
  Q_OBJECT

public:
  typedef QSharedPointer<LayerGui> ptr;

protected:
  /// Constructor. A mapper applies to a mapping.
  LayerGui(Layer::ptr mapping);

public:
  virtual ~LayerGui() {}

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor() { return _propertyBrowser.data(); }

  /// Returns the output/destination ShapeGraphicsItem.
  virtual QSharedPointer<ShapeGraphicsItem> getGraphicsItem() const { return _graphicsItem; }

  /// Returns the input/source ShapeGraphicsItem.
  virtual QSharedPointer<ShapeGraphicsItem> getInputGraphicsItem() { return _inputGraphicsItem; }

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);
  virtual void updateShape(MShape* shape);
	virtual void updateSources();

signals:
  void valueChanged();
  void sourceChanged();
  
protected:
  Layer::ptr _layer;

  QSharedPointer<QtTreePropertyBrowser> _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
	QtEnumPropertyManager* _sourceEnumManager;

  QtVariantProperty* _idItem;
  QtVariantProperty* _opacityItem;
	QtVariantProperty* _sourceItem;
  QtProperty* _outputItem;

  std::map<QtProperty*, std::pair<MShape*, int> > _propertyToVertex;

  QSharedPointer<ShapeGraphicsItem> _graphicsItem;
  QSharedPointer<ShapeGraphicsItem> _inputGraphicsItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  MShape::ptr outputShape;

  virtual void _buildShapeProperty(QtProperty* shapeItem, MShape* shape);
  virtual void _updateShapeProperty(QtProperty* shapeItem, MShape* shape);
};

/// Parent class for color -> color mappings.
class ColorLayerGui : public LayerGui
{
  Q_OBJECT

protected:
  ColorLayerGui(Layer::ptr mapping);
  virtual ~ColorLayerGui() {}

protected:
  QSharedPointer<Color> color;
};

class PolygonColorLayerGui : public ColorLayerGui
{
  Q_OBJECT

public:
  PolygonColorLayerGui(Layer::ptr mapping);
  virtual ~PolygonColorLayerGui() {}
};

class MeshColorLayerGui : public PolygonColorLayerGui
{
  Q_OBJECT

public:
  MeshColorLayerGui(Layer::ptr mapping);
  virtual ~MeshColorLayerGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseColorLayerGui : public ColorLayerGui
{
  Q_OBJECT

public:
  EllipseColorLayerGui(Layer::ptr mapping);
  virtual ~EllipseColorLayerGui() {}
};

/// Parent class for texture -> texture mapping.
class TextureLayerGui : public LayerGui
{
  Q_OBJECT

public:
  TextureLayerGui(QSharedPointer<TextureLayer> mapping);
  virtual ~TextureLayerGui() {}

public slots:
  virtual void updateShape(MShape* shape);

protected:
  QtProperty* _inputItem;
  QtVariantProperty* _meshItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  QWeakPointer<TextureLayer> textureLayer;
  QWeakPointer<MShape> inputShape;
};

class PolygonTextureLayerGui : public TextureLayerGui
{
  Q_OBJECT

public:
  PolygonTextureLayerGui(QSharedPointer<TextureLayer> mapping) : TextureLayerGui(mapping) {}
  virtual ~PolygonTextureLayerGui() {}
};

class TriangleTextureLayerGui : public PolygonTextureLayerGui
{
  Q_OBJECT

public:
  TriangleTextureLayerGui(QSharedPointer<TextureLayer> mapping);
  virtual ~TriangleTextureLayerGui() {}
};

class MeshTextureLayerGui : public PolygonTextureLayerGui
{
  Q_OBJECT

public:
  MeshTextureLayerGui(QSharedPointer<TextureLayer> mapping);
  virtual ~MeshTextureLayerGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseTextureLayerGui : public PolygonTextureLayerGui {
  Q_OBJECT

public:
  EllipseTextureLayerGui(QSharedPointer<TextureLayer> mapping);
  virtual ~EllipseTextureLayerGui() {}

protected:
  static void _setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle);
};

}

#endif /* LAYER_GUI_H_ */
