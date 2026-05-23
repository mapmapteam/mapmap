/*
 * LayerGui.cpp
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

#include "LayerGui.h"
#include "MainWindow.h"

namespace mmp {

LayerGui::LayerGui(Layer::ptr layer)
  : _layer(layer),
    _graphicsItem(NULL),
    _inputGraphicsItem(NULL)
{
  outputShape = layer->getShape();
  Q_CHECK_PTR(outputShape);

  // Create editor.
  _propertyBrowser.reset(new QtTreePropertyBrowser);
  _variantManager = new VariantManager;
  _variantFactory = new VariantFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  _sourceEnumManager = new QtEnumPropertyManager(this);

  // Mapping UID.
  _idItem = _variantManager->addProperty(QVariant::Int, QObject::tr("ID"));
  _idItem->setEnabled(false);
  _idItem->setValue(_layer->getId());
  _propertyBrowser->addProperty(_idItem);

  // Mapping basic properties.
  _opacityItem = _variantManager->addProperty(QVariant::Double, QObject::tr("Opacity (%)"));
  _opacityItem->setAttribute("minimum", 0.0);
  _opacityItem->setAttribute("maximum", 100.0);
  _opacityItem->setAttribute("decimals", 1);
  _opacityItem->setValue(_layer->getOpacity()*100.0);
  _propertyBrowser->addProperty(_opacityItem);

  _sourceItem = _variantManager->addProperty(QtVariantPropertyManager::enumTypeId(), "Source");
  _propertyBrowser->addProperty(_sourceItem);
  updateSources();

  // Output shape.
  _outputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                             QObject::tr("Output shape"));

  _buildShapeProperty(_outputItem, layer->getShape().data());
  _propertyBrowser->addProperty(_outputItem);

  // Collapse output shape.
  _propertyBrowser->setExpanded(_propertyBrowser->items(_outputItem).at(0), false);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));
  //qDebug() << "Creating mapper" << endl;
}


void LayerGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _opacityItem)
  {
    double opacity = qBound(value.toDouble() / 100.0, 0.0, 1.0);
    if (opacity != _layer->getOpacity())
    {
      _layer->setOpacity(opacity);
      emit valueChanged();
    }
  }
  else if (property == _sourceItem)
  {
    int sourceIndex = value.toInt();
    Source::ptr newSource = MainWindow::window()->getMappingManager().getSource(sourceIndex);
    if (newSource != _layer->getSource() && _layer->sourceIsCompatible(newSource)) {
      _layer->setSource(newSource);
      emit valueChanged();
      emit sourceChanged();
    }
  }
  else
  {
    std::map<QtProperty*, std::pair<MShape*, int> >::iterator it = _propertyToVertex.find(property);
    if (it != _propertyToVertex.end())
    {
      const QPointF& p = value.toPointF();
      MShape* shape = it->second.first;
      int    v     = it->second.second;
      if (shape->getVertex(v) != p)
      {
        shape->setVertex(v, p);
        emit valueChanged();
      }
    }
  }
}

void LayerGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "opacity")
    _opacityItem->setValue(value.toDouble() * 100);
}

void LayerGui::updateShape(MShape* shape)
{
  if (shape == _layer->getShape().data())
  {
    _updateShapeProperty(_outputItem, shape);
  }
}

void LayerGui::updateSources()
{
	int currentSource = -1;
	MappingManager& manager = MainWindow::window()->getMappingManager();
	QStringList sourceList;
	QVector<Source::ptr> sources = manager.getSourcesCompatibleWith(_layer);
	for (int i=0; i<sources.size(); i++)
	{
		sourceList.append(sources[i]->getName());
		if (sources[i] == _layer->getSource())
			currentSource = i;
	}
	_sourceItem->setAttribute("enumNames", sourceList);
	_sourceItem->setValue(currentSource);
}

void LayerGui::_buildShapeProperty(QtProperty* shapeItem, MShape* shape)
{
  for (int i=0; i<shape->nVertices(); i++)
  {
    // Add point.
    QtVariantProperty* pointItem = _variantManager->addProperty(QVariant::PointF,
                                                                QObject::tr("Point %1").arg(i));

    const QPointF& p = shape->getVertex(i);
    pointItem->setValue(p);

    shapeItem->addSubProperty(pointItem);
    _propertyToVertex[pointItem] = std::make_pair(shape, i);
  }

}

void LayerGui::_updateShapeProperty(QtProperty* shapeItem, MShape* shape)
{
  QList<QtProperty*> pointItems = shapeItem->subProperties();
  for (int i=0; i<shape->nVertices(); i++)
  {
    // XXX mesh control points are not added to properties
    if (i < pointItems.size())
    {
      QtVariantProperty* pointItem = (QtVariantProperty*)pointItems[i];
      const QPointF& p = shape->getVertex(i);
      pointItem->setValue(p);
    }
  }
}

ColorLayerGui::ColorLayerGui(Layer::ptr layer)
  : LayerGui(layer)
{
  color = qSharedPointerCast<Color>(_layer->getSource());
  Q_CHECK_PTR(color);
}

PolygonColorLayerGui::PolygonColorLayerGui(Layer::ptr layer) : ColorLayerGui(layer) {
  _graphicsItem.reset(new PolygonColorGraphicsItem(_layer, true));
}

MeshColorLayerGui::MeshColorLayerGui(Layer::ptr layer)
  : PolygonColorLayerGui(layer)
{
  _graphicsItem.reset(new MeshColorGraphicsItem(_layer, true));

  // Add mesh sub property.
  QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_layer->getShape());
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Mesh Subdivisions"));
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _meshItem->setAttribute("minimum", QSize(2,2));
  _propertyBrowser->insertProperty(_meshItem, _sourceItem); // insert at the beginning
}

void MeshColorLayerGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_layer->getShape());
    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
    if (mesh->nColumns() != size.width() || mesh->nRows() != size.height())
    {
      mesh->resize(size.width(), size.height());

//      _graphicsItem->resetVertices();
//      _inputGraphicsItem->resetVertices();

      // TODO: here we need to create the graphicsitems

      emit valueChanged();
    }
  }
  else
    PolygonColorLayerGui::setValue(property, value);
}

EllipseColorLayerGui::EllipseColorLayerGui(Layer::ptr layer) : ColorLayerGui(layer) {
    _graphicsItem.reset(new EllipseColorGraphicsItem(_layer, true));
}

//MeshColorLayerGui::MeshColorLayerGui(Layer::ptr layer)
//  : ColorLayerGui(layer) {
//  // Add mesh sub property.
//  Mesh* mesh = (Mesh*)layer->getShape().get();
//  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Mesh Subdivisions"));
//  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
//  _topItem->insertSubProperty(_meshItem, 0); // insert at the beginning
//}
//
//void MeshColorLayerGui::draw(QPainter* painter)
//{
//  painter->setPen(Qt::NoPen);
//  painter->setBrush(color->getColor());
//
//  QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(outputShape);
//  QVector<QVector<Quad> > outputQuads = outputMesh->getQuads2d();
//  for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
//  {
//    for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
//    {
//      Quad& outputQuad = outputQuads[x][y];
//      painter->drawPolygon(outputQuad.toPolygon());
//    }
//  }
//}
//
//void MeshColorLayerGui::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(outputShape);
//  Util::drawControlsMesh(painter, selectedVertices, *outputMesh);
//}
//
//void MeshColorLayerGui::setValue(QtProperty* property, const QVariant& value)
//{
//  if (property == _meshItem)
//  {
//    Mesh* outputMesh = static_cast<Mesh*>(_layer->getShape().get());
//    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
//    if (outputMesh->nColumns() != size.width() || outputMesh->nRows() != size.height())
//    {
//      outputMesh->resize(size.width(), size.height());
//
//      emit valueChanged();
//    }
//  }
//  else
//    ColorLayerGui::setValue(property, value);
//}

TextureLayerGui::TextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : LayerGui(mapping),
    _meshItem(NULL)
{
  // Assign members pointers.
  textureLayer = qSharedPointerCast<TextureLayer>(_layer);
  Q_CHECK_PTR(textureLayer);

  inputShape = textureLayer.toStrongRef()->getInputShape();
  Q_CHECK_PTR(inputShape);

  // Input shape.
  _inputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                            QObject::tr("Input shape"));
  _buildShapeProperty(_inputItem, inputShape.toStrongRef().data());
  _propertyBrowser->insertProperty(_inputItem, _sourceItem); // insert

  // Collapse input shape.
  _propertyBrowser->setExpanded(_propertyBrowser->items(_inputItem).at(0), false);
}
//
//void TextureLayerGui::drawInput(QPainter* painter)
//{
//  // Prepare drawing.
//  _preDraw(painter);
//
//  // FIXME: Does this draw the quad counterclockwise?
//  glBegin (GL_QUADS);
//  {
//    Util::correctGlTexCoord(0, 0);
//    glVertex3f (texture->getX(), texture->getY(), 0);
//
//    Util::correctGlTexCoord(1, 0);
//    glVertex3f (texture->getX()+texture->getWidth(), texture->getY(), 0);
//
//    Util::correctGlTexCoord(1, 1);
//    glVertex3f (texture->getX()+texture->getWidth(), texture->getY() + texture->getHeight(), 0);
//
//    Util::correctGlTexCoord(0, 1);
//    glVertex3f (texture->getX(), texture->getY() + texture->getHeight(), 0);
//  }
//  glEnd ();
//
//  // End drawing.
//  _postDraw(painter);
//}

void TextureLayerGui::updateShape(MShape* shape)
{
  QSharedPointer<TextureLayer> textureLayer = qSharedPointerCast<TextureLayer>(_layer);
  Q_CHECK_PTR(textureLayer);

  QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(textureLayer->getSource());
  Q_CHECK_PTR(texture);

  MShape* inputShape  = textureLayer->getInputShape().data();
  MShape* outputShape = textureLayer->getShape().data();
  if (shape == inputShape)
  {
    _updateShapeProperty(_inputItem, inputShape);
  }
  else if (shape == outputShape)
  {
    _updateShapeProperty(_outputItem, outputShape);
  }

}

//
//void TextureLayerGui::_preDraw(QPainter* painter)
//{
//  painter->beginNativePainting();
//
//  // Only works for similar shapes.
//  Q_ASSERT( inputShape->nVertices() == outputShape->nVertices());
//
//  // Project source texture and sent it to destination.
//  texture->update();
//
//  glEnable (GL_TEXTURE_2D);
//  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());
//
//  // Copy bits to texture iff necessary.
//  texture->lockMutex();
//  if (texture->bitsHaveChanged())
//  {
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
//      texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
//      GL_UNSIGNED_BYTE, texture->getBits());
//  }
//  texture->unlockMutex();
//
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//}
//
//void TextureLayerGui::_postDraw(QPainter* painter)
//{
//  glDisable(GL_TEXTURE_2D);
//
//  painter->endNativePainting();
//}
//
//void PolygonTextureLayerGui::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Polygon> outputPoly = qSharedPointerCast<Polygon>(outputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *outputPoly);
//}
//
//void PolygonTextureLayerGui::drawInputControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Polygon> inputPoly = qSharedPointerCast<Polygon>(inputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *inputPoly);
//}

TriangleTextureLayerGui::TriangleTextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new TriangleTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new TriangleTextureGraphicsItem(_layer, false));
}
//
//void TriangleTextureLayerGui::_doDraw(QPainter* painter)
//{
//  qDebug() << "Is this really used!" << endl;
////  Q_UNUSED(painter);
////  glBegin(GL_TRIANGLES);
////  {
////    for (int i = 0; i < inputShape->nVertices(); i++)
////    {
////      Util::setGlTexPoint(*texture, inputShape->getVertex(i), outputShape->getVertex(i));
////    }
////  }
////  glEnd();
//}

MeshTextureLayerGui::MeshTextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new MeshTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new MeshTextureGraphicsItem(_layer, false));

  // Add mesh sub property.
  QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_layer->getShape());
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Subdivisions"));

  // Rename subdivision subproperties.
  QList<QtProperty *> subList = _meshItem->subProperties();
  subList[0]->setPropertyName(tr("Horizontal"));
  subList[1]->setPropertyName(tr("Vertical"));

  // Set defaults.
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _meshItem->setAttribute("minimum", QSize(2, 2));

  // Add.
  _propertyBrowser->insertProperty(_meshItem, _sourceItem); // insert at the beginning
}

void MeshTextureLayerGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(_layer->getShape());
    QSharedPointer<Mesh> inputMesh  = qSharedPointerCast<Mesh>(textureLayer.toStrongRef()->getInputShape());
    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
    if (outputMesh->nColumns() != size.width() || outputMesh->nRows() != size.height() ||
        inputMesh->nColumns() != size.width() || inputMesh->nRows() != size.height())
    {
      outputMesh->resize(size.width(), size.height());
      inputMesh->resize(size.width(), size.height());

//      _graphicsItem->resetVertices();
//      _inputGraphicsItem->resetVertices();

      // TODO: here we need to create the graphicsitems

      emit valueChanged();
    }
  }
  else
    TextureLayerGui::setValue(property, value);
}

EllipseTextureLayerGui::EllipseTextureLayerGui(QSharedPointer<TextureLayer> mapping)
: PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new EllipseTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new EllipseTextureGraphicsItem(_layer, false));
}

}
