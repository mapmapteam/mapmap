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

namespace mmp {

LayerGui::LayerGui(Layer::ptr layer)
  : _layer(layer),
    _graphicsItem(nullptr),
    _inputGraphicsItem(nullptr)
{
  outputShape = layer->getShape();
  Q_CHECK_PTR(outputShape);

  // Create editor.
  _propertyBrowser.reset(new QtTreePropertyBrowser);
  VariantManager* variantManager = new VariantManager;
  _variantManager = variantManager;
  _variantFactory = new VariantFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  // The panel is made from the settings of the layer and of its shapes, and follows them.
  _settingsBrowser = new SettingsBrowser(_layer->settings(), _propertyBrowser.data(), variantManager, this);
  _settingsBrowser->setGroupLabel("output", tr("Output shape"));
  _settingsBrowser->setGroupLabel("input", tr("Input shape"));
  connect(_layer.data(), &Layer::settingChanged, _settingsBrowser,
          [this](uid, const QString& path, const QVariant& value) { _settingsBrowser->refresh(path, value); });
  connect(_layer.data(), &Layer::settingsStructureChanged, _settingsBrowser, &SettingsBrowser::rebuild);
}

void LayerGui::updateSources()
{
  // The menu is computed again from the options of the setting.
  _settingsBrowser->refresh("sourceId", QVariant());
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
}

EllipseColorLayerGui::EllipseColorLayerGui(Layer::ptr layer) : ColorLayerGui(layer) {
    _graphicsItem.reset(new EllipseColorGraphicsItem(_layer, true));
}

//MeshColorLayerGui::MeshColorLayerGui(Layer::ptr layer)
//  : ColorLayerGui(layer) {
//  // Add mesh sub property.
//  Mesh* mesh = (Mesh*)layer->getShape().get();
//  _meshItem = _variantManager->addProperty(QMetaType::QSize, QObject::tr("Mesh Subdivisions"));
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
  : LayerGui(mapping)
{
  // Assign members pointers.
  textureLayer = qSharedPointerCast<TextureLayer>(_layer);
  Q_CHECK_PTR(textureLayer);

  inputShape = textureLayer.toStrongRef()->getInputShape();
  Q_CHECK_PTR(inputShape);
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
}

EllipseTextureLayerGui::EllipseTextureLayerGui(QSharedPointer<TextureLayer> mapping)
: PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new EllipseTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new EllipseTextureGraphicsItem(_layer, false));
}

}
