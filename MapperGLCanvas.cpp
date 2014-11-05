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

MapperGLCanvas::MapperGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget * shareWidget)
  : QGLWidget(QGLFormat(QGL::SampleBuffers), parent, shareWidget),
    _mainWindow(mainWindow),
    _mousePressedOnVertex(false),
    _activeVertex(NO_VERTEX),
    _shapeGrabbed(false), // comment out?
    _shapeFirstGrab(false), // comment out?
    _displayControls(true),
    _stickyVertices(true)
{
}

void MapperGLCanvas::initializeGL()
{
  //qDebug() << "initializeGL" << endl;
  // Clear to black.
  qglClearColor(Qt::black);
  // Antialiasing options.
  //QGLWidget::setFormat(QGLFormat(QGL::SampleBuffers));
}

void MapperGLCanvas::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (
    0.0f, (GLfloat) width, // left, right
    (GLfloat) height, 0.0f, // bottom, top
    -1.0, 1.0f);
  glMatrixMode (GL_MODELVIEW);
  //qDebug() << "done resizing" << endl;
}

void MapperGLCanvas::paintGL()
{
}

void MapperGLCanvas::draw(QPainter* painter)
{
  //qDebug() << "draw mappergl" << endl;
  enterDraw(painter);
  doDraw(painter);
  exitDraw(painter);
  // qDebug() << "done." << endl;
}

void MapperGLCanvas::enterDraw(QPainter* painter)
{
  painter->beginNativePainting();

  // Clear to black.
  glClearColor(0.0, 0.0, 0.0, 1.0);

  // Clear buffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  painter->endNativePainting();

  // Antialiasing.
  painter->setRenderHint(QPainter::Antialiasing);
  //painter->setBackground(Qt::black);
  painter->setPen(Qt::NoPen);
  painter->setBrush(Qt::NoBrush);
}

void MapperGLCanvas::exitDraw(QPainter* painter)
{
  Q_UNUSED(painter);
}

Shape* MapperGLCanvas::getCurrentShape()
{
  return getShapeFromMappingId(getMainWindow()->getCurrentMappingId());
}

void MapperGLCanvas::mousePressEvent(QMouseEvent* event)
{
  int i;
  int dist;
  int minDistance;

  bool mousePressedOnSomething = false;

  _mousePressedPosition = event->pos();

  // Note: we compare with the square value for fastest computation of the distance
  minDistance = MM::VERTEX_SELECT_RADIUS * MM::VERTEX_SELECT_RADIUS;

  // Drag the closest vertex
  if (event->buttons() & Qt::LeftButton)
  {
    Shape* shape = getCurrentShape();
    if (shape)
    {
      // find the ID of the nearest vertex: (from the selected shape)
      for (i = 0; i < shape->nVertices(); i++)
      {
        dist = distSq(_mousePressedPosition, shape->getVertex(i)); // squared distance
        if (dist < minDistance)
        {
          _activeVertex = i;
          minDistance = dist;

          _mousePressedOnVertex = true;
          mousePressedOnSomething = true;
        }
      }
    }
  }

  if (mousePressedOnSomething)
    return;

  // Select a shape with a click.
  if (event->buttons() & Qt::LeftButton)
  {
    Shape* orig = getCurrentShape();
    MappingManager manager = getMainWindow()->getMappingManager();
    QVector<Mapping::ptr> mappings = manager.getVisibleMappings();
    for (QVector<Mapping::ptr>::const_iterator it = mappings.end() - 1; it >= mappings.begin(); --it)
    {
      Shape *shape = getShapeFromMappingId((*it)->getId());
      // Mouse pressed on a shape.
      if (shape && shape->includesPoint(_mousePressedPosition))
      {
        mousePressedOnSomething = true;
        // Deselect vertices.
        deselectVertices();
        // Change mapping.
        if (shape != orig)
        {
          getMainWindow()->setCurrentMapping((*it)->getId());
        }
        break;
      }
    }

    // Grab the shape.
    if (orig && orig->includesPoint(_mousePressedPosition))
    {
      _shapeGrabbed = true;
      _shapeFirstGrab = true;
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
  _mousePressedOnVertex = false;
  _shapeGrabbed = false;
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
  if (_mousePressedOnVertex)
  {
    // std::cout << "Move event " << std::endl;
    Shape* shape = getCurrentShape();
    if (shape && _activeVertex != NO_VERTEX)
    {
      QPointF p = shape->getVertex(_activeVertex);
      // Set point to mouse coordinates.
      p.setX(event->x());
      p.setY(event->y());

      // Stick to vertices.
      if (stickyVertices())
        glueVertex(shape, &p);
      shape->setVertex(_activeVertex, p);

      update();
      emit shapeChanged(getCurrentShape());
    }
  }
  else if (_shapeGrabbed)
  {
    // std::cout << "Move event " << std::endl;
    Shape* shape = getCurrentShape();
    static QPointF prevMousePosition(0,0); // point that keeps track of last position of the mouse
    if (shape)
    {
      if (!_shapeFirstGrab)
      {
        shape->translate(event->x() - prevMousePosition.x(), event->y() - prevMousePosition.y());
        update();
        emit shapeChanged(getCurrentShape());
      }
      else
        _shapeFirstGrab = false;
    }
    // Update previous mouse position.
    prevMousePosition.setX( event->x() );
    prevMousePosition.setY( event->y() );
  }
}

void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
{
  // Checks if the key has been handled by this function or needs to be deferred to superclass.
  bool handledKey = false;

  // Active vertex selected.
  if (hasActiveVertex())
  {
    Shape* shape = getCurrentShape();
    QPointF p = shape->getVertex(_activeVertex);
    handledKey = true;
    switch (event->key()) {
    // TODO: key tab should switch to next vertex: not working because somehow caught at a higher level
    // to switch between frames of the layout
//    case Qt::Key_Tab:
//      if (shape)
//        _activeVertex = (_activeVertex + 1) % shape->nVertices();
//        p = shape->getVertex(_activeVertex); // reset to new vertex
//        qDebug() << "New active vertex : " << _activeVertex << endl;
//      break;
    // Handle pixel-wise adjustments of vertex.
    case Qt::Key_Up:
      p.ry()--;
      break;
    case Qt::Key_Down:
      p.ry()++;
      break;
    case Qt::Key_Right:
      p.rx()++;
      break;
    case Qt::Key_Left:
      p.rx()--;
      break;
    default:
      handledKey = false;
      break;
    }
    // TODO: this will always be called even if no arrow key has been pressed (small performance issue).
    shape->setVertex(_activeVertex, p);
    update();
    emit shapeChanged(getCurrentShape());
  }

  // Defer unhandled keys to parent.
  if (!handledKey)
  {
    QWidget::keyPressEvent(event);
  }

//  std::cout << "Key pressed" << std::endl;
//  int xMove = 0;
//  int yMove = 0;
//  switch (event->key()) {
//  case Qt::Key_Tab:
//    if (event->modifiers() & Qt::ControlModifier)
//      switchImage( (Common::getCurrentSourceId() + 1) % Common::nImages());
//    else
//    {
//      Quad& quad = getQuad();
//      _active_vertex = (_active_vertex + 1 ) % 4;
//    }
//    break;
//  case Qt::Key_Up:
//    yMove = -1;
//    break;
//  case Qt::Key_Down:
//    yMove = +1;
//    break;
//  case Qt::Key_Left:
//    xMove = -1;
//    break;
//  case Qt::Key_Right:
//    xMove = +1;
//    break;
//  default:
//    std::cerr << "Unhandled key" << std::endl;
//    QWidget::keyPressEvent(event);
//    break;
//  }
//
//  Quad& quad = getQuad();
//  Point *p = quad.getVertex(_active_vertex);
//  p->x += xMove;
//  p->y += yMove;
//  quad.setVertex(_active_vertex, p);
//
//  update();
//
//  emit quadChanged();
}

void MapperGLCanvas::paintEvent(QPaintEvent* )
{
  makeCurrent();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  draw(&painter);

  painter.end();
}

void MapperGLCanvas::updateCanvas()
{
  update();
}

void MapperGLCanvas::enableDisplayControls(bool display)
{
  _displayControls = display;
  updateCanvas();
}

void MapperGLCanvas::enableTestSignal(bool enable)
{
  _displayTestSignal = enable;
  updateCanvas();
}

void MapperGLCanvas::enableStickyVertices(bool value)
{
  _stickyVertices = value;
}

/* Stick vertex p of Shape orig to another Shape's vertex, if the 2 vertices are
 * close enough. The distance per coordinate is currently set in dist_stick
 * variable. Perhaps the sticky-sensitivity should be configurable through GUI */
void MapperGLCanvas::glueVertex(Shape *orig, QPointF *p)
{
  MappingManager manager = getMainWindow()->getMappingManager();
  for (int i = 0; i < manager.nMappings(); i++)
  {
    Shape *shape = getShapeFromMappingId(manager.getMapping(i)->getId());
    if (shape && shape != orig)
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
