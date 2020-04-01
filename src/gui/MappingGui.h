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
  virtual QSharedPointer<ShapeGraphicsItem> getGraphicsItem() const { return _graphicsItem; }

  /// Returns the input/source ShapeGraphicsItem.
  virtual QSharedPointer<ShapeGraphicsItem> getInputGraphicsItem() { return _inputGraphicsItem; }

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);
  virtual void updateShape(MShape* shape);
	virtual void updatePaints();

signals:
  void valueChanged();
  void paintChanged();
  
protected:
  Mapping::ptr _mapping;

  QSharedPointer<QtTreePropertyBrowser> _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
	QtEnumPropertyManager* _paintEnumManager;

  QtVariantProperty* _idItem;
  QtVariantProperty* _opacityItem;
	QtVariantProperty* _paintItem;
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
  PolygonColorMappingGui(Mapping::ptr mapping);
  virtual ~PolygonColorMappingGui() {}
};

class MeshColorMappingGui : public PolygonColorMappingGui
{
  Q_OBJECT

public:
  MeshColorMappingGui(Mapping::ptr mapping);
  virtual ~MeshColorMappingGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

private:
  QtVariantProperty* _meshItem;
};

class EllipseColorMappingGui : public ColorMappingGui
{
  Q_OBJECT

public:
  EllipseColorMappingGui(Mapping::ptr mapping);
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

}

#endif /* MAPPER_H_ */
