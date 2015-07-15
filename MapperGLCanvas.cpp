/*
 * MapperGLCanvas.cpp
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

#include "MapperGLCanvas.h"

#include "MainWindow.h"
#include "Commands.h"

MapperGLCanvas::MapperGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget * shareWidget, QGraphicsScene* scene)
  : QGraphicsView(parent),
    _mainWindow(mainWindow),
    _mousePressedOnVertex(false),
    _activeVertex(NO_VERTEX),
    _shapeGrabbed(false), // comment out?
    _shapeFirstGrab(false), // comment out?
    _zoomLevel(0)
{
  // For now clicking on the window doesn't do anything.
  setDragMode(QGraphicsView::NoDrag);

  setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                 QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform);

  setResizeAnchor(AnchorViewCenter);
  setInteractive(true);

  //setFrameStyle(Sunken | StyledPanel);
  // TODO: check this
  // setOptimizationFlags(QGraphicsView::DontSavePainterState);
  //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  resetTransform();
  // setAcceptDrops(true);

  // Render with OpenGL.
  setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers), this, shareWidget));
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

  // TODO: do we need to delete scene (or call new QGraphicsScene(this)?)
  setScene(scene ? scene : new QGraphicsScene);

  // Black background.
  this->scene()->setBackgroundBrush(Qt::black);
}

MShape* MapperGLCanvas::getCurrentShape()
{
  return getShapeFromMappingId(MainWindow::instance()->getCurrentMappingId());
}

ShapeGraphicsItem* MapperGLCanvas::getCurrentShapeGraphicsItem()
{
  return getShapeGraphicsItemFromMappingId(MainWindow::instance()->getCurrentMappingId());
}

// Draws foreground (displays crosshair if needed).
void MapperGLCanvas::drawForeground(QPainter *painter , const QRectF &rect)
{
  Q_UNUSED(rect);
  if (_mainWindow->displayControls())
  {
    uid mid = _mainWindow->getCurrentMappingId();
    if (mid != NULL_UID)
    {
      ShapeGraphicsItem* item = getCurrentShapeGraphicsItem();
      if (item)
        item->getControlPainter()->paint(painter);
    }
  }
}

//
void MapperGLCanvas::mousePressEvent(QMouseEvent* event)
{
  bool mousePressedOnSomething = false;

  _mousePressedPosition = event->pos();
  QPointF pos = mapToScene(event->pos());


  // Check for vertex selection first.
  else if (event->buttons() & Qt::LeftButton)
  {
    MShape* shape = getCurrentShape();
    if (shape)
    {
      // Note: we compare with the square value for fastest computation of the distance
      int minDistance = sq(MM::VERTEX_SELECT_RADIUS);

      // Find the ID of the nearest vertex (from currently selected shape)
      for (int i = 0; i < shape->nVertices(); i++)
      {
        int dist = distSq(_mousePressedPosition, mapFromScene(shape->getVertex(i))); // squared distance
        if (dist < minDistance)
        {
          _activeVertex = i;
          minDistance = dist;

          _mousePressedOnVertex = true;
          mousePressedOnSomething = true;

          _grabbedObjectStartPosition = shape->getVertex(i);
        }
      }
    }
  }

  if (mousePressedOnSomething)
    return;

  // Check for shape selection.
  if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) // Add Right click for context menu
  {
    MShape* selectedShape = getCurrentShape();

    // Possibility of changing shape in output by clicking on it.
    MappingManager manager = getMainWindow()->getMappingManager();
    QVector<Mapping::ptr> mappings = manager.getVisibleMappings();
    for (QVector<Mapping::ptr>::const_iterator it = mappings.end() - 1; it >= mappings.begin(); --it)
    {
      MShape *shape = getShapeFromMappingId((*it)->getId());

      // Check if mouse was pressed on that shape.
      if (shape && shape->includesPoint(pos))
      {
        mousePressedOnSomething = true;

        // Deselect vertices.
        deselectVertices();

        // Change mapping (only available in destination).
        if (isOutput() && shape != selectedShape)
        {
          // Change current mapping.
          getMainWindow()->setCurrentMapping((*it)->getId());

          // Reset selected shape to new one.
          selectedShape = getCurrentShape();
        }

        break;
      }
    }

    // Grab the shape.
    if (event->buttons() & Qt::LeftButton) // This preserve me from duplicate code above
    {
      if (selectedShape && selectedShape->includesPoint(pos))
      {
        _shapeGrabbed = true;
        _shapeFirstGrab = true;

        _grabbedObjectStartPosition = pos;
      }
    }
  }

  if (mousePressedOnSomething)
    return;

  // Deactivate.
  deselectAll();
}

void MapperGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
//  // Click on vertex ==> select the vertex.
//  if ((event->buttons() & Qt::LeftButton) && _mousePressedOnVertex)
//  {
//  }
  if (_mousePressedOnVertex)
  {
  }
  else if (_shapeGrabbed)
  {

  }
  _mousePressedOnVertex = false;
  _shapeGrabbed = false;
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
  static QPoint lastMousePos;

  QPointF pos = mapToScene(event->pos());

  // Prepare to store commands
  undoStack = getMainWindow()->getUndoStack();

  // Vertex grab.
  if (_mousePressedOnVertex)
  {
    // std::cout << "Move event " << std::endl;
    MShape* shape = getCurrentShape();
    if (shape && _activeVertex != NO_VERTEX)
    {
//      QPointF p = shape->getVertex(_activeVertex);
//      // Set point to mouse coordinates.
//      p.setX(pos.x());
//      p.setY(pos.y());

      QPointF p = pos;

      // Stick to vertices.
      if (_mainWindow->stickyVertices())
        _glueVertex(&p);

      shape->setVertex(_activeVertex, p);
    }
  }

  // Shape grab.
  else if (_shapeGrabbed)
  {
    // std::cout << "Move event " << std::endl;
    MShape* shape = getCurrentShape();
    if (shape)
    {
      if (_shapeFirstGrab)
      {
        lastMousePos = _mousePressedPosition;
        _shapeFirstGrab = false;
      }
    }

    QPointF diff = pos - mapToScene(lastMousePos);
    shape->translate(diff.x(), diff.y());
  }

  // Window translation action
  else if (event->buttons() & Qt::MiddleButton)
  {
    QPointF diff = event->pos() - lastMousePos;
    QGraphicsView* view = scene()->views().first();
    view->translate(diff.x(), diff.y());
//    view->update();
  }

  lastMousePos = event->pos();
}

//
//void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
//{
//  // Prepare to store commands
//  undoStack = getMainWindow()->getUndoStack();
//
//  // Checks if the key has been handled by this function or needs to be deferred to superclass.
//  bool handledKey = false;
//
//  // Active vertex selected.
//  if (hasActiveVertex())
//  {
//    Shape* shape = getCurrentShape();
//    QPointF p = shape->getVertex(_activeVertex);
//    handledKey = true;
//    switch (event->key()) {
//    // TODO: key tab should switch to next vertex: not working because somehow caught at a higher level
//    // to switch between frames of the layout
////    case Qt::Key_Tab:
////      if (shape)
////        _activeVertex = (_activeVertex + 1) % shape->nVertices();
////        p = shape->getVertex(_activeVertex); // reset to new vertex
////        qDebug() << "New active vertex : " << _activeVertex << endl;
////      break;
//    // Handle pixel-wise adjustments of vertex.
//    case Qt::Key_Up:
//      p.ry()--;
//      break;
//    case Qt::Key_Down:
//      p.ry()++;
//      break;
//    case Qt::Key_Right:
//      p.rx()++;
//      break;
//    case Qt::Key_Left:
//      p.rx()--;
//      break;
//    default:
//      if (event->matches(QKeySequence::Undo))
//        undoStack->undo();
//
//      else if (event->matches(QKeySequence::Redo))
//        undoStack->redo();
//      else
//        handledKey = false;
//      break;
//    }
//    // TODO: this will always be called even if no arrow key has been pressed (small performance issue).
//    // Enable to Undo and Redo when arrow keys move the position of vertices
//    undoStack->push(new MoveVertexCommand(this, _activeVertex, p));
//  }
//
//  // Defer unhandled keys to parent.
//  if (!handledKey)
//  {
//    QWidget::keyPressEvent(event);
//  }
//
////  std::cout << "Key pressed" << std::endl;
////  int xMove = 0;
////  int yMove = 0;
////  switch (event->key()) {
////  case Qt::Key_Tab:
////    if (event->modifiers() & Qt::ControlModifier)
////      switchImage( (Common::getCurrentSourceId() + 1) % Common::nImages());
////    else
////    {
////      Quad& quad = getQuad();
////      _active_vertex = (_active_vertex + 1 ) % 4;
////    }
////    break;
////  case Qt::Key_Up:
////    yMove = -1;
////    break;
////  case Qt::Key_Down:
////    yMove = +1;
////    break;
////  case Qt::Key_Left:
////    xMove = -1;
////    break;
////  case Qt::Key_Right:
////    xMove = +1;
////    break;
////  default:
////    std::cerr << "Unhandled key" << std::endl;
////    QWidget::keyPressEvent(event);
////    break;
////  }
////
////  Quad& quad = getQuad();
////  Point *p = quad.getVertex(_active_vertex);
////  p->x += xMove;
////  p->y += yMove;
////  quad.setVertex(_active_vertex, p);
////
////  update();
////
////  emit quadChanged();
//}
//
//void MapperGLCanvas::paintEvent(QPaintEvent* )
//{
//  makeCurrent();
//
//  QPainter painter(this);
//  painter.setRenderHint(QPainter::Antialiasing);
//
//  draw(&painter);
//
//  painter.end();
//}

void MapperGLCanvas::updateCanvas()
{
  update();
  scene()->update();
}

///* Stick vertex p of Shape orig to another Shape's vertex, if the 2 vertices are
// * close enough. The distance per coordinate is currently set in dist_stick
// * variable. Perhaps the sticky-sensitivity should be configurable through GUI */
//void MapperGLCanvas::glueVertex(MShape *orig, QPointF *p)
//{
//  MappingManager manager = getMainWindow()->getMappingManager();
//  for (int i = 0; i < manager.nMappings(); i++)
//  {
//    MShape *shape = getShapeFromMappingId(manager.getMapping(i)->getId());
//    if (shape && shape != orig)
//    {
//      for (int vertex = 0; vertex < shape->nVertices(); vertex++)
//      {
//        const QPointF& v = shape->getVertex(vertex);
//        if (distIsInside(v, *p, MM::VERTEX_STICK_RADIUS))
//        {
//          p->setX(v.x());
//          p->setY(v.y());
//        }
//      }
//    }
//  }
//}


void MapperGLCanvas::deselectVertices()
{
  _activeVertex = NO_VERTEX;
  _mousePressedOnVertex = false;
}

void MapperGLCanvas::deselectAll()
{
  deselectVertices();
  _shapeGrabbed = false;
  _shapeFirstGrab = false;
}

void MapperGLCanvas::wheelEvent(QWheelEvent *event)
{
  int deltaLevel = event->delta() / 120;
  qreal zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
  if (deltaLevel > 0)
  {
    // First check if we're already at max.
    while (deltaLevel && zoomFactor < MM::ZOOM_MAX) {
      _zoomLevel++;
      deltaLevel--;
      zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
    }
    zoomFactor = qMin(zoomFactor, MM::ZOOM_MAX);
  }
  else
  {
    // First check if we're already at min.
    while (deltaLevel && zoomFactor > MM::ZOOM_MIN) {
      _zoomLevel--;
      deltaLevel++;
      zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
    }
    zoomFactor = qMax(zoomFactor, MM::ZOOM_MIN);
  }

  // Re-bound zoom (for consistency).
  zoomFactor = getZoomFactor();

  // Apply zoom to view.
  QGraphicsView* view = scene()->views().first();
  view->resetMatrix();
  view->scale(zoomFactor, zoomFactor);
  view->update();

  // Accept wheel scrolling event.
  event->accept();
}

bool MapperGLCanvas::eventFilter(QObject *target, QEvent *event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    setActiveVertexIndex(getMainWindow()->getOutputWindow()->getCanvas()->getActiveVertexIndex());
    MapperGLCanvas::keyPressEvent(keyEvent);
    return true;
  }
  else
  {
    return QObject::eventFilter(target, event);
  }
}


void MapperGLCanvas::_glueVertex(QPointF* p)
{
  MappingManager manager = MainWindow::instance()->getMappingManager();
  for (int i = 0; i < manager.nMappings(); i++)
  {
    MShape *shape = manager.getMapping(i)->getShape().get();
    if (shape && shape != getCurrentShape())
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
