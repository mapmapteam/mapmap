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

#include <QtGui/qopengl.h>

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

#include <functional>

#include "Inspector.h"

namespace mmp {

class MainWindow;

/**
 * This is the "view" side of the Layer class (model). It contains the graphic items for
 * both input and output as well as the inspector: a Surface tab made from the settings of the
 * layer and of its shapes, and a Media tab made from the settings of its source, which both
 * follow their changes.
 */
class LayerGui : public QObject
{
  Q_OBJECT

public:
  typedef QSharedPointer<LayerGui> ptr;

protected:
  /// Constructor. A layer gui applies to a layer.
  LayerGui(Layer::ptr layer);

public:
  virtual ~LayerGui() {}

public:
  /// Returns a pointer to the properties editor for that layer.
  virtual QWidget* getPropertiesEditor() { return _inspector.data(); }

  /// Counts the layers that use a source, for the sentence of the Media tab.
  typedef std::function<int(const Source::ptr&)> SourceUseCounter;

  /// Sets the function counting the layers that use a source; set by the main window.
  static void setSourceUseCounter(SourceUseCounter counter);

  /// Returns the translated kind of the shape of a layer, such as "Mesh".
  static QString kindOf(const Layer::ptr& layer);

  /// Returns the output/destination ShapeGraphicsItem.
  virtual QSharedPointer<ShapeGraphicsItem> getGraphicsItem() const { return _graphicsItem; }

  /// Returns the input/source ShapeGraphicsItem.
  virtual QSharedPointer<ShapeGraphicsItem> getInputGraphicsItem() { return _inputGraphicsItem; }

public slots:
  /// Shows the sources of the project in the "Source" row; call when they were added,
  /// removed or renamed.
  virtual void updateSources();

protected:
  Layer::ptr _layer;

  /// Points the Media tab to the source of the layer, and says when it is shared.
  void _showSource();

  QSharedPointer<Inspector> _inspector;
  /// The tab of the layer and its shapes.
  InspectorPage* _surfacePage;
  /// The tab of the source of the layer.
  InspectorPage* _mediaPage;
  /// The source shown in the Media tab, and the connections that follow it.
  Source::ptr _shownSource;
  QMetaObject::Connection _sourceValueConnection;
  QMetaObject::Connection _sourceStructureConnection;

  static SourceUseCounter _sourceUseCounter;

  QSharedPointer<ShapeGraphicsItem> _graphicsItem;
  QSharedPointer<ShapeGraphicsItem> _inputGraphicsItem;

  // FIXME: use typedefs, member of the class for type names that are too long to type:
  MShape::ptr outputShape;
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

protected:
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
