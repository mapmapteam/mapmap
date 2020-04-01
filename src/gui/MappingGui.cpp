/*
 * MappingGui.cpp
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

#include "MappingGui.h"
#include "MainWindow.h"

namespace mmp {

MappingGui::MappingGui(Mapping::ptr mapping)
  : _mapping(mapping),
    _graphicsItem(NULL),
    _inputGraphicsItem(NULL)
{
  outputShape = mapping->getShape();
  Q_CHECK_PTR(outputShape);

  // Create editor.
  _propertyBrowser.reset(new QtTreePropertyBrowser);
  _variantManager = new VariantManager;
  _variantFactory = new VariantFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  _paintEnumManager = new QtEnumPropertyManager(this);

  // Mapping UID.
  _idItem = _variantManager->addProperty(QVariant::Int, QObject::tr("ID"));
  _idItem->setEnabled(false);
  _idItem->setValue(_mapping->getId());
  _propertyBrowser->addProperty(_idItem);

  // Mapping basic properties.
  _opacityItem = _variantManager->addProperty(QVariant::Double, QObject::tr("Opacity (%)"));
  _opacityItem->setAttribute("minimum", 0.0);
  _opacityItem->setAttribute("maximum", 100.0);
  _opacityItem->setAttribute("decimals", 1);
  _opacityItem->setValue(_mapping->getOpacity()*100.0);
  _propertyBrowser->addProperty(_opacityItem);

  _paintItem = _variantManager->addProperty(QtVariantPropertyManager::enumTypeId(), "Source");
  _propertyBrowser->addProperty(_paintItem);
  updatePaints();

  // Output shape.
  _outputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                             QObject::tr("Output shape"));

  _buildShapeProperty(_outputItem, mapping->getShape().data());
  _propertyBrowser->addProperty(_outputItem);

  // Collapse output shape.
  _propertyBrowser->setExpanded(_propertyBrowser->items(_outputItem).at(0), false);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));
  //qDebug() << "Creating mapper" << endl;
}


void MappingGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _opacityItem)
  {
    double opacity = qBound(value.toDouble() / 100.0, 0.0, 1.0);
    if (opacity != _mapping->getOpacity())
    {
      _mapping->setOpacity(opacity);
      emit valueChanged();
    }
  }
  else if (property == _paintItem)
  {
    int paintIndex = value.toInt();
    Paint::ptr newPaint = MainWindow::window()->getMappingManager().getPaint(paintIndex);
    if (newPaint != _mapping->getPaint() && _mapping->paintIsCompatible(newPaint)) {
      _mapping->setPaint(newPaint);
      emit valueChanged();
      emit paintChanged();
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

void MappingGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "opacity")
    _opacityItem->setValue(value.toDouble() * 100);
}

void MappingGui::updateShape(MShape* shape)
{
  if (shape == _mapping->getShape().data())
  {
    _updateShapeProperty(_outputItem, shape);
  }
}

void MappingGui::updatePaints()
{
	int currentPaint = -1;
	MappingManager& manager = MainWindow::window()->getMappingManager();
	QStringList paintList;
	QVector<Paint::ptr> paints = manager.getPaintsCompatibleWith(_mapping);
	for (int i=0; i<paints.size(); i++)
	{
		paintList.append(paints[i]->getName());
		if (paints[i] == _mapping->getPaint())
			currentPaint = i;
	}
	_paintItem->setAttribute("enumNames", paintList);
	_paintItem->setValue(currentPaint);
}

void MappingGui::_buildShapeProperty(QtProperty* shapeItem, MShape* shape)
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

void MappingGui::_updateShapeProperty(QtProperty* shapeItem, MShape* shape)
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

ColorMappingGui::ColorMappingGui(Mapping::ptr mapping)
  : MappingGui(mapping)
{
  color = qSharedPointerCast<Color>(_mapping->getPaint());
  Q_CHECK_PTR(color);
}

PolygonColorMappingGui::PolygonColorMappingGui(Mapping::ptr mapping) : ColorMappingGui(mapping) {
  _graphicsItem.reset(new PolygonColorGraphicsItem(mapping, true));
}

MeshColorMappingGui::MeshColorMappingGui(Mapping::ptr mapping)
  : PolygonColorMappingGui(mapping)
{
  _graphicsItem.reset(new MeshColorGraphicsItem(_mapping, true));

  // Add mesh sub property.
  QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_mapping->getShape());
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Mesh Subdivisions"));
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _meshItem->setAttribute("minimum", QSize(2,2));
  _propertyBrowser->insertProperty(_meshItem, _paintItem); // insert at the beginning
}

void MeshColorMappingGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_mapping->getShape());
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
    PolygonColorMappingGui::setValue(property, value);
}

EllipseColorMappingGui::EllipseColorMappingGui(Mapping::ptr mapping) : ColorMappingGui(mapping) {
    _graphicsItem.reset(new EllipseColorGraphicsItem(mapping, true));
}

//MeshColorMappingGui::MeshColorMappingGui(Mapping::ptr mapping)
//  : ColorMappingGui(mapping) {
//  // Add mesh sub property.
//  Mesh* mesh = (Mesh*)mapping->getShape().get();
//  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Mesh Subdivisions"));
//  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
//  _topItem->insertSubProperty(_meshItem, 0); // insert at the beginning
//}
//
//void MeshColorMappingGui::draw(QPainter* painter)
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
//void MeshColorMappingGui::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(outputShape);
//  Util::drawControlsMesh(painter, selectedVertices, *outputMesh);
//}
//
//void MeshColorMappingGui::setValue(QtProperty* property, const QVariant& value)
//{
//  if (property == _meshItem)
//  {
//    Mesh* outputMesh = static_cast<Mesh*>(_mapping->getShape().get());
//    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
//    if (outputMesh->nColumns() != size.width() || outputMesh->nRows() != size.height())
//    {
//      outputMesh->resize(size.width(), size.height());
//
//      emit valueChanged();
//    }
//  }
//  else
//    ColorMappingGui::setValue(property, value);
//}

TextureMappingGui::TextureMappingGui(QSharedPointer<TextureMapping> mapping)
  : MappingGui(mapping),
    _meshItem(NULL)
{
  // Assign members pointers.
  textureMapping = qSharedPointerCast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  inputShape = textureMapping.toStrongRef()->getInputShape();
  Q_CHECK_PTR(inputShape);

  // Input shape.
  _inputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                            QObject::tr("Input shape"));
  _buildShapeProperty(_inputItem, inputShape.data());
  _propertyBrowser->insertProperty(_inputItem, _paintItem); // insert

  // Collapse input shape.
  _propertyBrowser->setExpanded(_propertyBrowser->items(_inputItem).at(0), false);
}
//
//void TextureMappingGui::drawInput(QPainter* painter)
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

void TextureMappingGui::updateShape(MShape* shape)
{
  QSharedPointer<TextureMapping> textureMapping = qSharedPointerCast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  MShape* inputShape  = textureMapping->getInputShape().data();
  MShape* outputShape = textureMapping->getShape().data();
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
//void TextureMappingGui::_preDraw(QPainter* painter)
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
//void TextureMappingGui::_postDraw(QPainter* painter)
//{
//  glDisable(GL_TEXTURE_2D);
//
//  painter->endNativePainting();
//}
//
//void PolygonTextureMappingGui::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Polygon> outputPoly = qSharedPointerCast<Polygon>(outputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *outputPoly);
//}
//
//void PolygonTextureMappingGui::drawInputControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  QSharedPointer<Polygon> inputPoly = qSharedPointerCast<Polygon>(inputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *inputPoly);
//}

TriangleTextureMappingGui::TriangleTextureMappingGui(QSharedPointer<TextureMapping> mapping)
  : PolygonTextureMappingGui(mapping)
{
  _graphicsItem.reset(new TriangleTextureGraphicsItem(_mapping, true));
  _inputGraphicsItem.reset(new TriangleTextureGraphicsItem(_mapping, false));
}
//
//void TriangleTextureMappingGui::_doDraw(QPainter* painter)
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

MeshTextureMappingGui::MeshTextureMappingGui(QSharedPointer<TextureMapping> mapping)
  : PolygonTextureMappingGui(mapping)
{
  _graphicsItem.reset(new MeshTextureGraphicsItem(_mapping, true));
  _inputGraphicsItem.reset(new MeshTextureGraphicsItem(_mapping, false));

  // Add mesh sub property.
  QSharedPointer<Mesh> mesh = qSharedPointerCast<Mesh>(_mapping->getShape());
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Subdivisions"));

  // Rename subdivision subproperties.
  QList<QtProperty *> subList = _meshItem->subProperties();
  subList[0]->setPropertyName(tr("Horizontal"));
  subList[1]->setPropertyName(tr("Vertical"));

  // Set defaults.
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _meshItem->setAttribute("minimum", QSize(2, 2));

  // Add.
  _propertyBrowser->insertProperty(_meshItem, _paintItem); // insert at the beginning
}

void MeshTextureMappingGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(_mapping->getShape());
    QSharedPointer<Mesh> inputMesh  = qSharedPointerCast<Mesh>(textureMapping.toStrongRef()->getInputShape());
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
    TextureMappingGui::setValue(property, value);
}

EllipseTextureMappingGui::EllipseTextureMappingGui(QSharedPointer<TextureMapping> mapping)
: PolygonTextureMappingGui(mapping)
{
  _graphicsItem.reset(new EllipseTextureGraphicsItem(_mapping, true));
  _inputGraphicsItem.reset(new EllipseTextureGraphicsItem(_mapping, false));
}

}
