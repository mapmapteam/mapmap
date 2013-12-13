/*
 * Mapper.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "Mapper.h"

TextureMapper::TextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
  : Mapper(mapping)
{
  _propertyBrowser = new QtTreePropertyBrowser;
  _variantManager = new QtVariantPropertyManager;
  _variantFactory = new QtVariantEditorFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  _topItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                          QObject::tr("Texture mapping"));

  if (std::tr1::dynamic_pointer_cast<Mesh>(textureMapping->getShape()))
  {
    Mesh* mesh = (Mesh*)textureMapping->getShape().get();
    _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Dimensions"));
    _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
    _topItem->addSubProperty(_meshItem);
  }
  else
    _meshItem = 0;

  // Input shape.
  _inputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                            QObject::tr("Input shape"));

  _buildShapeProperty(_inputItem, textureMapping->getInputShape().get());
  _topItem->addSubProperty(_inputItem);

  // Output shape.
  // Input shape.
  _outputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                             QObject::tr("Output shape"));

  _buildShapeProperty(_outputItem, textureMapping->getShape().get());
  _topItem->addSubProperty(_outputItem);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));

  _propertyBrowser->addProperty(_topItem);

  qDebug() << "Creating mapper" << endl;
}

void TextureMapper::setValue(QtProperty* property, const QVariant& value)
{
  std::map<QtProperty*, std::pair<Shape*, int> >::iterator it = _propertyToVertex.find(property);
  if (it != _propertyToVertex.end())
  {
    QPointF p = value.toPointF();
    it->second.first->setVertex(it->second.second, Point(p.x(), p.y()));
    qDebug() << "Changing vertex: " << it->second.second << " to " << p.x() << "," << p.y() << endl;
  }
  else if (property == _meshItem)
  {
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
    Q_CHECK_PTR(textureMapping);

    Mesh* outputMesh = static_cast<Mesh*>(textureMapping->getShape().get());
    Mesh* inputMesh = static_cast<Mesh*>(textureMapping->getInputShape().get());
    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
    outputMesh->resize(size.width(), size.height());
    inputMesh->resize(size.width(), size.height());
  }

  emit valueChanged();
//  qDebug() << "Property changed to " << property->propertyName() << " " << value.toPointF().x() << ", " << value.toPointF().y() << endl;
}

void TextureMapper::updateShape(Shape* shape)
{
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  Shape* inputShape  = textureMapping->getInputShape().get();
  Shape* outputShape = textureMapping->getShape().get();
  if (shape == inputShape)
  {
    _updateShapeProperty(_inputItem, inputShape);
  }
  else if (shape == outputShape)
  {
    _updateShapeProperty(_outputItem, outputShape);
  }

}

QWidget* TextureMapper::getPropertiesEditor()
{
  return _propertyBrowser;
}

void TextureMapper::draw()
{
  // FIXME: use typedefs, member of the class for type names that are too long to type:
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  std::tr1::shared_ptr<Shape> outputShape = std::tr1::static_pointer_cast<Shape>(textureMapping->getShape());
  Q_CHECK_PTR(outputShape);

  std::tr1::shared_ptr<Shape> inputShape = std::tr1::static_pointer_cast<Shape>(textureMapping->getInputShape());
  Q_CHECK_PTR(inputShape);

  // Only works for similar shapes.
  Q_ASSERT( outputShape->nVertices() == outputShape->nVertices());

  printf("Texid: %d\n", texture->getTextureId());
  // Project source texture and sent it to destination.

  glEnable (GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
    texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
    GL_UNSIGNED_BYTE, texture->getBits());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  if (std::tr1::dynamic_pointer_cast<Mesh>(outputShape))
  {
    std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
    std::tr1::shared_ptr<Mesh> inputMesh  = std::tr1::static_pointer_cast<Mesh>(inputShape);
    std::vector<std::vector<Quad> > outputQuads = outputMesh->getQuads2d();
    std::vector<std::vector<Quad> > inputQuads  = inputMesh->getQuads2d();
    for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
    {
      for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
      {
        Quad& outputQuad = outputQuads[x][y];
        Quad& inputQuad  = inputQuads[x][y];
        glBegin(GL_QUADS);
        for (int i = 0; i < 4; i++)
        {
          Util::correctGlTexCoord(
            (inputQuad.getVertex(i).x - texture->getX()) / (GLfloat) texture->getWidth(),
            (inputQuad.getVertex(i).y - texture->getY()) / (GLfloat) texture->getHeight());
          glVertex2f(
            outputQuad.getVertex(i).x,
            outputQuad.getVertex(i).y
            );
        }
        glEnd();
      }
    }

  }
  else
  {
    if (std::tr1::dynamic_pointer_cast<Quad>(outputShape))
      glBegin(GL_QUADS);
    else if (std::tr1::dynamic_pointer_cast<Triangle>(outputShape))
      glBegin(GL_TRIANGLES);
    else
      // TODO: untested
      glBegin(GL_POLYGON);
    {
      for (int i = 0; i < inputShape->nVertices(); i++)
      {
        Util::correctGlTexCoord(
          (inputShape->getVertex(i).x - texture->getX()) / (GLfloat) texture->getWidth(),
          (inputShape->getVertex(i).y - texture->getY()) / (GLfloat) texture->getHeight());
        glVertex2f(
          outputShape->getVertex(i).x,
          outputShape->getVertex(i).y
          );
      }
    }
    glEnd();
  }

  glDisable(GL_TEXTURE_2D);
}

void TextureMapper::_buildShapeProperty(QtProperty* shapeItem, Shape* shape)
{
  for (int i=0; i<shape->nVertices(); i++)
  {
    // Add point.
    QtVariantProperty* pointItem = _variantManager->addProperty(QVariant::PointF,
                                                                QObject::tr("Point %1").arg(i));

    Point p = shape->getVertex(i);
    pointItem->setValue(QPointF(p.x, p.y));

    shapeItem->addSubProperty(pointItem);
    _propertyToVertex[pointItem] = std::make_pair(shape, i);
  }

}

void TextureMapper::_updateShapeProperty(QtProperty* shapeItem, Shape* shape)
{
  QList<QtProperty*> pointItems = shapeItem->subProperties();
  for (int i=0; i<shape->nVertices(); i++)
  {
    // XXX mesh control points are not added to properties
    if (dynamic_cast<Mesh*>(shape) == 0 && i < pointItems.size())
    {
      QtVariantProperty* pointItem = (QtVariantProperty*)pointItems[i];
      Point p = shape->getVertex(i);
      pointItem->setValue(QPointF(p.x, p.y));
    }
  }
}

