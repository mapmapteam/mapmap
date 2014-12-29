/*
 * Mapper.cpp
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

#include "Mapper.h"
#include "MainWindow.h"

ShapeGraphicsItem::ShapeGraphicsItem(Mapping::ptr mapping, bool output)
  : _mapping(mapping), _output(output)
{
  _shape = output ? _mapping->getShape() : _mapping->getInputShape();

  setFlags(ItemIsMovable | ItemIsSelectable);

  // Shape filters child (control point) events.
  setFiltersChildEvents(true);

  // Create control point graphics items.
  _createVertices();
}

bool ShapeGraphicsItem::sceneEventFilter(QGraphicsItem * watched, QEvent * event)
{
  // Change vertex in model according to moved item.
  if (event->type() == QEvent::GraphicsSceneMouseMove)
  {
    QGraphicsSceneMoveEvent* moveEvent = static_cast<QGraphicsSceneMoveEvent*>(event);
    QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
 //   Q_ASSERT(moveEvent);
    Q_ASSERT(mouseEvent);

    int idx = childItems().indexOf(watched);
    Q_ASSERT(idx != -1);

//    QPointF pos = moveEvent->newPos();// + this->pos();
    QPointF pos = mouseEvent->scenePos();
 //   qDebug() << moveEvent->oldPos() << " " << pos << " " << childItems().at(idx)->pos() << endl;
    _shape->setVertex(idx, pos);

    _syncVertices();

    // Refresh this shape.
   // update();

    // override default
    return true;
  }
  else
  {
    // Returns false to allow the child item to process its event.
    return false;
  }
}

void ShapeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseMoveEvent(event);
  _syncShape();
}

//QVariant ShapeGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
//{
//  if (change == ItemPositionChange)
//  {
//    qDebug() << "Item changed" << endl;
//    return QPointF(pos().x(), value.toPointF().y());
//  }
//
//  return QGraphicsItem::itemChange(change, value);
//}

void ShapeGraphicsItem::_createVertices()
{
  // rect offset
  for (int i=0; i<_shape->nVertices(); i++)
  {
    // XXX is this freed by parent?
    QPointF pos = mapFromScene(_shape->getVertex(i));// - this->pos();
    VertexGraphicsItem* child = new VertexGraphicsItem(i);
//    child->setPos( pos );
    child->setParentItem(this);
    child->setPos( pos );
    child->setRect( -MM::VERTEX_SELECT_RADIUS, -MM::VERTEX_SELECT_RADIUS, MM::VERTEX_SELECT_RADIUS*2, MM::VERTEX_SELECT_RADIUS*2);
//    child->setRect(pos.x()-offset, pos.y()-offset, MM::VERTEX_SELECT_RADIUS, MM::VERTEX_SELECT_RADIUS);
//    qDebug() << "Adding child at " << pos << " " << child->pos();
//    qDebug() << ", after add " << child->pos() << endl;
  }
}

void ShapeGraphicsItem::_syncShape()
{
  QList<QGraphicsItem*> children = childItems();
  for (int i=0; i<_shape->nVertices(); i++)
  {
    QGraphicsItem* child = children.at(i);
    _shape->setVertex(i, child->scenePos());
  }
}

void ShapeGraphicsItem::_syncVertices()
{
  for (int i=0; i<_shape->nVertices(); i++)
  {
    QPointF pos = _shape->getVertex(i) ;//- this->pos(); // this is in scene coordinates
    childItems().at(i)->setPos(this->mapFromScene(pos));
    childItems().at(i)->update();
  }
}

QPainterPath PolygonColorGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.get());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

QRectF PolygonColorGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void PolygonColorGraphicsItem::paint(QPainter *painter,
                                     const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  Color* color = static_cast<Color*>(_mapping->getPaint().get());
  Q_ASSERT(color);

  // Setup pen and brush.
  if (option->state & QStyle::State_Selected)
    painter->setPen(MM::SHAPE_STROKE);
  else
    painter->setPen(Qt::NoPen);

  painter->setBrush(color->getColor());

  // TODO: polygon and ellipse are 99% similar (except for these 3 lines!). we should use that.
  Polygon* poly = static_cast<Polygon*>(_shape.get());
  Q_ASSERT(poly);
  painter->drawPolygon(mapFromScene(poly->toPolygon()));
}

QPainterPath EllipseColorGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  Ellipse* ellipse = static_cast<Ellipse*>(_shape.get());
  Q_ASSERT(ellipse);
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  return mapFromScene(transform.map(path));
}

QRectF EllipseColorGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void EllipseColorGraphicsItem::paint(QPainter* painter,
                                     const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  Color* color = static_cast<Color*>(_mapping->getPaint().get());
  Q_ASSERT(color);

  // Setup pen and brush.
  if (option->state & QStyle::State_Selected)
    painter->setPen(MM::SHAPE_STROKE);
  else
    painter->setPen(Qt::NoPen);

  painter->setBrush(color->getColor());
  // Just draw the path.
  painter->drawPath(shape());
}

TextureGraphicsItem::TextureGraphicsItem(Mapping::ptr mapping, bool output)
  : ShapeGraphicsItem(mapping, output)
{
  _textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
  Q_CHECK_PTR(_textureMapping);

  _texture = std::tr1::static_pointer_cast<Texture>(_textureMapping->getPaint());
  Q_CHECK_PTR(_texture);

  _inputShape = std::tr1::static_pointer_cast<MShape>(_textureMapping->getInputShape());
  Q_CHECK_PTR(_inputShape);
}

void TextureGraphicsItem::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  bool selected = option->state & QStyle::State_Selected;

  // Prepare drawing.
  _preDraw(painter);

  // Perform the actual mapping (done by subclasses).
  _doDraw(painter, selected);

  // End drawing.
  _postDraw(painter);

//  if (selected)
    _doDrawControls(painter);
}

void TextureGraphicsItem::_preDraw(QPainter* painter)
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

void TextureGraphicsItem::_postDraw(QPainter* painter)
{
  glDisable(GL_TEXTURE_2D);

  painter->endNativePainting();
}

QPainterPath PolygonTextureGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.get());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

QRectF PolygonTextureGraphicsItem::boundingRect() const {
  return shape().boundingRect();
}


void TriangleTextureGraphicsItem::_doDraw(QPainter* painter, bool selected)
{
  Q_UNUSED(painter);
  Q_UNUSED(selected);
  if (_output)
  {
    glBegin(GL_TRIANGLES);
    {
      for (int i=0; i<_inputShape->nVertices(); i++)
      {
        Util::setGlTexPoint(*_texture, _inputShape->getVertex(i), mapFromScene(_shape->getVertex(i)));
      }
    }
    glEnd();
  }
  else
  {
    // FIXME: Does this draw the quad counterclockwise?
    glBegin (GL_QUADS);
    {
      QRectF rect = mapFromScene(_texture->getRect()).boundingRect();

      Util::correctGlTexCoord(0, 0);
      glVertex3f (rect.x(), rect.y(), 0);

      Util::correctGlTexCoord(1, 0);
      glVertex3f (rect.x() + rect.width(), rect.y(), 0);

      Util::correctGlTexCoord(1, 1);
      glVertex3f (rect.x()+rect.width(), rect.y()+rect.height(), 0);

      Util::correctGlTexCoord(0, 1);
      glVertex3f (rect.x(), rect.y()+rect.height(), 0);
    }
    glEnd ();
  }
}

Mapper::Mapper(Mapping::ptr mapping)
  : _mapping(mapping),
    _graphicsItem(NULL),
    _inputGraphicsItem(NULL)
{
  outputShape = mapping->getShape();
  Q_CHECK_PTR(outputShape);

  // Create editor.
  _propertyBrowser = new QtTreePropertyBrowser;
  _variantManager = new VariantManager;
  _variantFactory = new VariantFactory;

  _topItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                          QObject::tr("Texture mapping"));

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);


  // Output shape.
  _outputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                             QObject::tr("Output shape"));

  _buildShapeProperty(_outputItem, mapping->getShape().get());
  _topItem->addSubProperty(_outputItem);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));

  _propertyBrowser->addProperty(_topItem);

  //qDebug() << "Creating mapper" << endl;
}

Mapper::~Mapper()
{
  if (_propertyBrowser)
    delete _propertyBrowser;
  if (_graphicsItem)
    delete _graphicsItem;
}

QWidget* Mapper::getPropertiesEditor()
{
  return _propertyBrowser;
}

void Mapper::setValue(QtProperty* property, const QVariant& value)
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

void Mapper::_buildShapeProperty(QtProperty* shapeItem, MShape* shape)
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

void Mapper::_updateShapeProperty(QtProperty* shapeItem, MShape* shape)
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

ColorMapper::ColorMapper(Mapping::ptr mapping)
  : Mapper(mapping)
{
  color = std::tr1::static_pointer_cast<Color>(_mapping->getPaint());
  Q_CHECK_PTR(color);
}

void ColorMapper::updatePaint()
{
  color.reset();
  color = std::tr1::static_pointer_cast<Color>(_mapping->getPaint());
  Q_CHECK_PTR(color);
}

MeshColorMapper::MeshColorMapper(Mapping::ptr mapping)
  : ColorMapper(mapping) {
  // Add mesh sub property.
  Mesh* mesh = (Mesh*)mapping->getShape().get();
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Dimensions"));
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _topItem->insertSubProperty(_meshItem, 0); // insert at the beginning
}

void MeshColorMapper::draw(QPainter* painter)
{
  painter->setPen(Qt::NoPen);
  painter->setBrush(color->getColor());

  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
  QVector<QVector<Quad> > outputQuads = outputMesh->getQuads2d();
  for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
  {
    for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
    {
      Quad& outputQuad = outputQuads[x][y];
      painter->drawPolygon(outputQuad.toPolygon());
    }
  }
}

void MeshColorMapper::drawControls(QPainter* painter, const QList<int>* selectedVertices)
{
  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
  Util::drawControlsMesh(painter, selectedVertices, *outputMesh);
}

void MeshColorMapper::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    Mesh* outputMesh = static_cast<Mesh*>(_mapping->getShape().get());
    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
    if (outputMesh->nColumns() != size.width() || outputMesh->nRows() != size.height())
    {
      outputMesh->resize(size.width(), size.height());

      emit valueChanged();
    }
  }
  else
    ColorMapper::setValue(property, value);
}

TextureMapper::TextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
  : Mapper(mapping),
    _meshItem(NULL)
{
  // Assign members pointers.
  textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  inputShape = std::tr1::static_pointer_cast<MShape>(textureMapping->getInputShape());
  Q_CHECK_PTR(inputShape);

  // Input shape.
  _inputItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                            QObject::tr("Input shape"));

  _buildShapeProperty(_inputItem, textureMapping->getInputShape().get());
  _topItem->insertSubProperty(_inputItem, 0); // insert before output item
}
//
//void TextureMapper::drawInput(QPainter* painter)
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

void TextureMapper::updateShape(MShape* shape)
{
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  MShape* inputShape  = textureMapping->getInputShape().get();
  MShape* outputShape = textureMapping->getShape().get();
  if (shape == inputShape)
  {
    _updateShapeProperty(_inputItem, inputShape);
  }
  else if (shape == outputShape)
  {
    _updateShapeProperty(_outputItem, outputShape);
  }

}

void TextureMapper::updatePaint()
{
  texture.reset();
  texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);
}
//
//void TextureMapper::_preDraw(QPainter* painter)
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
//void TextureMapper::_postDraw(QPainter* painter)
//{
//  glDisable(GL_TEXTURE_2D);
//
//  painter->endNativePainting();
//}
//
//void PolygonTextureMapper::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  std::tr1::shared_ptr<Polygon> outputPoly = std::tr1::static_pointer_cast<Polygon>(outputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *outputPoly);
//}
//
//void PolygonTextureMapper::drawInputControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  std::tr1::shared_ptr<Polygon> inputPoly = std::tr1::static_pointer_cast<Polygon>(inputShape);
//  Util::drawControlsPolygon(painter, selectedVertices, *inputPoly);
//}

TriangleTextureMapper::TriangleTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
  : PolygonTextureMapper(mapping)
{
  _graphicsItem = new TriangleTextureGraphicsItem(_mapping, true);
  _inputGraphicsItem = new TriangleTextureGraphicsItem(_mapping, false);
}
//
//void TriangleTextureMapper::_doDraw(QPainter* painter)
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

MeshTextureMapper::MeshTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
  : PolygonTextureMapper(mapping)
{
  // Add mesh sub property.
  Mesh* mesh = (Mesh*)textureMapping->getShape().get();
  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Dimensions"));
  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
  _topItem->insertSubProperty(_meshItem, 0); // insert at the beginning
}

void MeshTextureMapper::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _meshItem)
  {
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
    Q_CHECK_PTR(textureMapping);

    Mesh* outputMesh = static_cast<Mesh*>(textureMapping->getShape().get());
    Mesh* inputMesh = static_cast<Mesh*>(textureMapping->getInputShape().get());
    QSize size = (static_cast<QtVariantProperty*>(property))->value().toSize();
    if (outputMesh->nColumns() != size.width() || outputMesh->nRows() != size.height() ||
        inputMesh->nColumns() != size.width() || inputMesh->nRows() != size.height())
    {
      outputMesh->resize(size.width(), size.height());
      inputMesh->resize(size.width(), size.height());

      emit valueChanged();
    }
  }
  else
    TextureMapper::setValue(property, value);
}

void MeshTextureMapper::drawControls(QPainter* painter, const QList<int>* selectedVertices)
{
  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
  Util::drawControlsMesh(painter, selectedVertices, *outputMesh);
}

void MeshTextureMapper::drawInputControls(QPainter* painter, const QList<int>* selectedVertices)
{
  std::tr1::shared_ptr<Mesh> inputMesh = std::tr1::static_pointer_cast<Mesh>(inputShape);
  Util::drawControlsMesh(painter, selectedVertices, *inputMesh);
}

void MeshTextureMapper::_doDraw(QPainter* painter)
{
  Q_UNUSED(painter);
  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
  std::tr1::shared_ptr<Mesh> inputMesh  = std::tr1::static_pointer_cast<Mesh>(inputShape);
  QVector<QVector<Quad> > outputQuads = outputMesh->getQuads2d();
  QVector<QVector<Quad> > inputQuads  = inputMesh->getQuads2d();
  for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
  {
    for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
    {
      Quad& outputQuad = outputQuads[x][y];
      Quad& inputQuad  = inputQuads[x][y];
      glBegin(GL_QUADS);
      for (int i = 0; i < 4; i++)
      {
        Util::setGlTexPoint(*texture, inputQuad.getVertex(i), outputQuad.getVertex(i));
      }
      glEnd();
    }
  }
}

EllipseTextureMapper::EllipseTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
: PolygonTextureMapper(mapping)
{
}

void EllipseTextureMapper::drawControls(QPainter* painter, const QList<int>* selectedVertices)
{
  std::tr1::shared_ptr<Ellipse> outputEllipse = std::tr1::static_pointer_cast<Ellipse>(outputShape);
  Util::drawControlsEllipse(painter, selectedVertices, *outputEllipse);
}

void EllipseTextureMapper::drawInputControls(QPainter* painter, const QList<int>* selectedVertices)
{
  std::tr1::shared_ptr<Ellipse> inputEllipse = std::tr1::static_pointer_cast<Ellipse>(inputShape);
  Util::drawControlsEllipse(painter, selectedVertices, *inputEllipse);
}

void EllipseTextureMapper::_doDraw(QPainter* painter)
{
  Q_UNUSED(painter);
  // Get input and output ellipses.
  std::tr1::shared_ptr<Ellipse> inputEllipse = std::tr1::static_pointer_cast<Ellipse>(inputShape);
  std::tr1::shared_ptr<Ellipse> outputEllipse = std::tr1::static_pointer_cast<Ellipse>(outputShape);

  // Start / end angle.
  //const float startAngle = 0;
  //const float endAngle   = 2*M_PI;

  //
  //float angle;
  QPointF currentInputPoint;
  QPointF prevInputPoint(0, 0);
  QPointF currentOutputPoint;
  QPointF prevOutputPoint(0, 0);

  // Input ellipse parameters.
  const QPointF& inputCenter         = inputEllipse->getCenter();
  const QPointF& inputControlCenter  = inputEllipse->getVertex(4);
  float    inputHorizRadius          = inputEllipse->getHorizontalRadius();
  float    inputVertRadius           = inputEllipse->getVerticalRadius();
  float    inputRotation             = inputEllipse->getRotationRadians();

  // Output ellipse parameters.
  const QPointF& outputCenter        = outputEllipse->getCenter();
  const QPointF& outputControlCenter = outputEllipse->getVertex(4);
  float    outputHorizRadius         = outputEllipse->getHorizontalRadius();
  float    outputVertRadius          = outputEllipse->getVerticalRadius();
  float    outputRotation            = outputEllipse->getRotationRadians();

  // Variation in angle at each step of the loop.
  const int N_TRIANGLES = 100;
  const float ANGLE_STEP = 2*M_PI/N_TRIANGLES;

  float circleAngle = 0;
  for (int i=0; i<=N_TRIANGLES; i++, circleAngle += ANGLE_STEP)
  {
    // Set next (current) points.
    _setPointOfEllipseAtAngle(currentInputPoint, inputCenter, inputHorizRadius, inputVertRadius, inputRotation, circleAngle);
    _setPointOfEllipseAtAngle(currentOutputPoint, outputCenter, outputHorizRadius, outputVertRadius, outputRotation, circleAngle);

    // We don't draw the first point.
    if (i > 0)
    {
      // Draw triangle.
      glBegin(GL_TRIANGLES);
      Util::setGlTexPoint(*texture, inputControlCenter, outputControlCenter);
      Util::setGlTexPoint(*texture, prevInputPoint, prevOutputPoint);
      Util::setGlTexPoint(*texture, currentInputPoint, currentOutputPoint);
      glEnd();
    }

    // Save point for next iteration.
    prevInputPoint.setX(currentInputPoint.x());
    prevInputPoint.setY(currentInputPoint.y());
    prevOutputPoint.setX(currentOutputPoint.x());
    prevOutputPoint.setY(currentOutputPoint.y());
  }
}

void EllipseTextureMapper::_setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle)
{
  float xCirc = sin(circularAngle) * hRadius;
  float yCirc = cos(circularAngle) * vRadius;
  float distance = sqrt( xCirc*xCirc + yCirc*yCirc );
  float angle    = atan2( xCirc, yCirc );
  rotation = 2*M_PI-rotation; // rotation needs to be inverted (CW <-> CCW)
  point.setX( sin(angle + rotation) * distance + center.x() );
  point.setY( cos(angle + rotation) * distance + center.y() );
}
