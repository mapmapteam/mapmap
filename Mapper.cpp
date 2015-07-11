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

bool ShapeGraphicsItem::isMappingCurrent() const { return MainWindow::instance()->getCurrentMappingId() == _mapping->getId(); }

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

    // Sticky vertex.
    _glueVertex(&pos);

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

void ShapeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  if (!isMappingVisible())
    return;

  if (isOutput())
  {
    QGraphicsItem::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
    {
      MainWindow::instance()->setCurrentMapping(_mapping->getId());
    }
  }
  else
  {
    if (isMappingCurrent())
      QGraphicsItem::mousePressEvent(event);
    else
      event->ignore(); // prevent mousegrabbing on non-current mapping
  }
}

void ShapeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (!isMappingVisible())
    return;

  QGraphicsItem::mouseMoveEvent(event);

  // Sync shape.
  _syncShape();
}

void ShapeGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  // Sync depth of figure with that of mapping (for layered output).
  if (isOutput())
    setZValue(_mapping->getDepth());

  // Paint if visible.
  if (isMappingVisible())
  {
    _prePaint(painter, option);
    _doPaint(painter, option);
    _postPaint(painter, option);
  }
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

void ShapeGraphicsItem::resetVertices() {
  // Clear vertices.
  QList<QGraphicsItem*> allChildren = children();
  for (QList<QGraphicsItem*>::iterator it = allChildren.begin(); it!=allChildren.end(); ++it)
  {
    (*it)->setParentItem(0);
    scene()->removeItem(*it);
    delete (*it);
  }

  // Re-create them.
  _createVertices();
}

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
    _shape->setVertex(i, children.at(i)->scenePos());
  }

  // The shape object is the model: it contains the logic to make sure the vertices are ok.
  // So here we need to re-sync the vertices (view side) according to the model.
  _syncVertices();
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

void ShapeGraphicsItem::_glueVertex(QPointF* p)
{
  MappingManager manager = MainWindow::instance()->getMappingManager();
  for (int i = 0; i < manager.nMappings(); i++)
  {
    MShape *shape = manager.getMapping(i)->getShape().get();
    if (shape && shape != _shape.get())
    {
      for (int vertex = 0; vertex < shape->nVertices(); vertex++)
      {
        const QPointF& v = shape->getVertex(vertex);
        if (distIsInside(v, *p, MM::VERTEX_STICK_RADIUS))
        {
          p->setX(v.x());
          p->setY(v.y());
        }
      }
    }
  }
}

void VertexGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
  if (!shapeParent->isMappingVisible())
  {
    // Prevent mouse grabbing.
    event->ignore();
  }
  else
  {
    if (shapeParent->isOutput())
    {
      QGraphicsItem::mousePressEvent(event);
      if (event->button() == Qt::LeftButton)
      {
        MainWindow::instance()->setCurrentMapping(shapeParent->getMapping()->getId());
      }
    }
    else
    {
      if (shapeParent->isMappingCurrent())
        QGraphicsItem::mousePressEvent(event);
      else
        event->ignore(); // prevent mousegrabbing on non-current mapping
    }
  }
}

void VertexGraphicsItem::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option,
    QWidget* widget)
{
  Q_UNUSED(widget);
  ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
  if (shapeParent->isMappingVisible() &&
      shapeParent->isMappingCurrent())
    Util::drawControlsVertex(painter, QPointF(0,0), (option->state & QStyle::State_Selected));
}

void ColorGraphicsItem::_prePaint(QPainter *painter,
                                  const QStyleOptionGraphicsItem *option)
{
  Color* color = static_cast<Color*>(_mapping->getPaint().get());
  Q_ASSERT(color);

  // Setup pen and brush.
  if (option->state & QStyle::State_Selected)
    painter->setPen(MM::SHAPE_STROKE);
  else
    painter->setPen(Qt::NoPen);

  // Set brush.
  QColor col = color->getColor();
  col.setAlphaF(_mapping->getOpacity());
  painter->setBrush(col);
}

QPainterPath PolygonColorGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.get());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

void PolygonColorGraphicsItem::_doPaint(QPainter *painter,
                                        const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
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

void EllipseColorGraphicsItem::_doPaint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option)
{
  Q_UNUSED(option);
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

void TextureGraphicsItem::_doPaint(QPainter *painter,
                                   const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  // Perform the actual mapping (done by subclasses).
  if (isOutput())
    _doDrawOutput(painter);
  else
    _doDrawInput(painter);
}

void TextureGraphicsItem::_doDrawInput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isMappingCurrent())
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

void TextureGraphicsItem::_doDrawControls(QPainter* painter)
{
  painter->setPen(MM::SHAPE_STROKE);
  painter->drawPath(shape());
}

void TextureGraphicsItem::_prePaint(QPainter* painter,
                                    const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  painter->beginNativePainting();

  // Only works for similar shapes.
  // TODO:remettre
  //Q_ASSERT( _inputShape->nVertices() == outputShape->nVertices());

  // Project source texture and sent it to destination.
  _texture->update();

  glEnable (GL_BLEND);
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

void TextureGraphicsItem::_postPaint(QPainter* painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  glDisable(GL_TEXTURE_2D);

  painter->endNativePainting();

  if (isMappingCurrent())
    _doDrawControls(painter);
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

void TriangleTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
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
}

void PolygonTextureGraphicsItem::_doDrawControls(QPainter* painter)
{
  painter->setPen(MM::SHAPE_STROKE);
  Polygon* poly = static_cast<Polygon*>(_shape.get());
  Q_ASSERT(poly);
  painter->drawPolygon(mapFromScene(poly->toPolygon()));
}

void MeshTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
  {
    std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(_shape);
    std::tr1::shared_ptr<Mesh> inputMesh  = std::tr1::static_pointer_cast<Mesh>(_inputShape);
    QVector<QVector<Quad> > outputQuads = outputMesh->getQuads2d();
    QVector<QVector<Quad> > inputQuads  = inputMesh->getQuads2d();
    for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
    {
      for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
      {
        Quad& outputQuad = outputQuads[x][y];
        Quad& inputQuad  = inputQuads[x][y];
        glBegin(GL_QUADS);
        for (int i = 0; i < outputQuad.nVertices(); i++)
        {
          Util::setGlTexPoint(*_texture, inputQuad.getVertex(i), mapFromScene(outputQuad.getVertex(i)));
        }
        glEnd();
      }
    }
  }

}

void MeshTextureGraphicsItem::_doDrawControls(QPainter* painter)
{
  Mesh* mesh = static_cast<Mesh*>(_shape.get());
  Q_ASSERT(mesh);

  // Init colors and stroke.
  painter->setPen(MM::SHAPE_INNER_STROKE);

  // Draw inner quads.
  QVector<Quad> quads = mesh->getQuads();
  for (QVector<Quad>::const_iterator it = quads.begin(); it != quads.end(); ++it)
  {
    painter->drawPolygon(mapFromScene(it->toPolygon()));
  }

  // Draw outer quad.
  painter->setPen(MM::SHAPE_STROKE);
  painter->drawPolygon(mapFromScene(mesh->toPolygon()));
}

QPainterPath EllipseTextureGraphicsItem::shape() const
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

QRectF EllipseTextureGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void EllipseTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  // Get input and output ellipses.
  std::tr1::shared_ptr<Ellipse> inputEllipse  = std::tr1::static_pointer_cast<Ellipse>(_inputShape);
  std::tr1::shared_ptr<Ellipse> outputEllipse = std::tr1::static_pointer_cast<Ellipse>(_shape);

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
  const QPointF& outputCenter        = mapFromScene(outputEllipse->getCenter());
  const QPointF& outputControlCenter = mapFromScene(outputEllipse->getVertex(4));
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
      Util::setGlTexPoint(*_texture, inputControlCenter, outputControlCenter);
      Util::setGlTexPoint(*_texture, prevInputPoint,     prevOutputPoint);
      Util::setGlTexPoint(*_texture, currentInputPoint,  currentOutputPoint);
      glEnd();
    }

    // Save point for next iteration.
    prevInputPoint.setX(currentInputPoint.x());
    prevInputPoint.setY(currentInputPoint.y());
    prevOutputPoint.setX(currentOutputPoint.x());
    prevOutputPoint.setY(currentOutputPoint.y());
  }
}

void EllipseTextureGraphicsItem::_setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle)
{
  float xCirc = sin(circularAngle) * hRadius;
  float yCirc = cos(circularAngle) * vRadius;
  float distance = sqrt( xCirc*xCirc + yCirc*yCirc );
  float angle    = atan2( xCirc, yCirc );
  rotation = 2*M_PI-rotation; // rotation needs to be inverted (CW <-> CCW)
  point.setX( sin(angle + rotation) * distance + center.x() );
  point.setY( cos(angle + rotation) * distance + center.y() );
}

void EllipseTextureGraphicsItem::_doDrawControls(QPainter* painter)
{
  painter->setPen(MM::SHAPE_STROKE);
  painter->setBrush(Qt::NoBrush);

  // Just draw the path.
  painter->drawPath(shape());

//  std::tr1::shared_ptr<Ellipse> outputEllipse = std::tr1::static_pointer_cast<Ellipse>(_shape);
//  Util::drawControlsEllipse(painter, selectedVertices, *outputEllipse);
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

//MeshColorMapper::MeshColorMapper(Mapping::ptr mapping)
//  : ColorMapper(mapping) {
//  // Add mesh sub property.
//  Mesh* mesh = (Mesh*)mapping->getShape().get();
//  _meshItem = _variantManager->addProperty(QVariant::Size, QObject::tr("Dimensions"));
//  _meshItem->setValue(QSize(mesh->nColumns(), mesh->nRows()));
//  _topItem->insertSubProperty(_meshItem, 0); // insert at the beginning
//}
//
//void MeshColorMapper::draw(QPainter* painter)
//{
//  painter->setPen(Qt::NoPen);
//  painter->setBrush(color->getColor());
//
//  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
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
//void MeshColorMapper::drawControls(QPainter* painter, const QList<int>* selectedVertices)
//{
//  std::tr1::shared_ptr<Mesh> outputMesh = std::tr1::static_pointer_cast<Mesh>(outputShape);
//  Util::drawControlsMesh(painter, selectedVertices, *outputMesh);
//}
//
//void MeshColorMapper::setValue(QtProperty* property, const QVariant& value)
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
//    ColorMapper::setValue(property, value);
//}

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
  _graphicsItem = new MeshTextureGraphicsItem(_mapping, true);
  _inputGraphicsItem = new MeshTextureGraphicsItem(_mapping, false);

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

      _graphicsItem->resetVertices();
      _inputGraphicsItem->resetVertices();

      // TODO: here we need to create the graphicsitems

      emit valueChanged();
    }
  }
  else
    TextureMapper::setValue(property, value);
}

EllipseTextureMapper::EllipseTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping)
: PolygonTextureMapper(mapping)
{
  _graphicsItem = new EllipseTextureGraphicsItem(_mapping, true);
  _inputGraphicsItem = new EllipseTextureGraphicsItem(_mapping, false);
}

