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
    _vertexGrabbed(false),
    _activeVertex(NO_VERTEX),
    _shapeGrabbed(false), // comment out?
    _shapeFirstGrab(false), // comment out?
    _zoomLevel(0),
    _shapeIsAdapted(false)
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

  // Create zoom tool buttons
  createZoomToolButtons();
  // Disable zoom tool buttons
  enableZoomToolButtons(false);
}

MShape::ptr MapperGLCanvas::getCurrentShape()
{
  return getShapeFromMappingId(MainWindow::instance()->getCurrentMappingId());
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getCurrentShapeGraphicsItem()
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
      // Use current shape graphics item to draw controls.
      ShapeGraphicsItem::ptr item = getCurrentShapeGraphicsItem();
      if (item)
      {
        QList<int> selected;
        if (hasActiveVertex())
          selected.push_back(getActiveVertexIndex());
        item->getControlPainter()->paint(painter, this, selected);
      }
    }
  }
}

void MapperGLCanvas::currentShapeWasChanged()
{
  emit shapeChanged(getCurrentShape().data());
}

void MapperGLCanvas::applyZoomToView()
{
  // Re-bound zoom (for consistency).
  qreal zoomFactor = getZoomFactor();
  // Get first of the list of all the views
  QGraphicsView* view = this->scene()->views().first();
  // Resets the view transformation matrix
  view->resetMatrix();
  // Scale the current view
  view->scale(zoomFactor, zoomFactor);
  // And update
  view->update();
}

void MapperGLCanvas::createZoomToolButtons()
{
  // Create zoom tool bar
  _zoomToolBox = new QWidget(this);
  _zoomToolBox->setWindowFlags(Qt::WindowStaysOnTopHint);
  _zoomToolBox->setObjectName("zoom-toolbox");

  // Create vertical layout for buttons
  QVBoxLayout* buttonsLayout = new QVBoxLayout;
  buttonsLayout->setMargin(0);
  // Create buttons
  // Zoom In button
  _zoomInButton = new QPushButton;
  _zoomInButton->setIcon(QIcon(":/zoom-in"));
  _zoomInButton->setIconSize(QSize(22, 22));
  _zoomInButton->setToolTip(tr("Enlarge the shape"));
  _zoomInButton->setFixedSize(32, 32);
  _zoomInButton->setObjectName("zoom-in");
  connect(_zoomInButton, SIGNAL(clicked()), this, SLOT(increaseZoomLevel()));
  // Zoom Out button
  _zoomOutButton = new QPushButton;
  _zoomOutButton->setIcon(QIcon(":/zoom-out"));
  _zoomOutButton->setIconSize(QSize(22, 22));
  _zoomOutButton->setToolTip(tr("Shrink the shape"));
  _zoomOutButton->setFixedSize(32, 32);
  _zoomOutButton->setObjectName("zoom-out");
  connect(_zoomOutButton, SIGNAL(clicked()), this, SLOT(decreaseZoomLevel()));
  // Reset to normal size button.
  _resetZoomButton = new QPushButton;
  _resetZoomButton->setIcon(QIcon(":/reset-zoom"));
  _resetZoomButton->setIconSize(QSize(22, 22));
  _resetZoomButton->setToolTip(tr("Reset the shape to the normal size"));
  _resetZoomButton->setFixedSize(32, 32);
  _resetZoomButton->setObjectName("reset-zoom");
  connect(_resetZoomButton, SIGNAL(clicked()), this, SLOT(resetZoomLevel()));
  // Fit to view button
  _fitToViewButton = new QPushButton;
  _fitToViewButton->setIcon(QIcon(":/zoom-fit"));
  _fitToViewButton->setIconSize(QSize(22, 22));
  _fitToViewButton->setToolTip(tr("Fit the shape to content view"));
  _fitToViewButton->setFixedSize(32, 32);
  _fitToViewButton->setObjectName("zoom-fit");
  connect(_fitToViewButton, SIGNAL(clicked()), this, SLOT(fitShapeInView()));

  // Add buttons into layout
  buttonsLayout->addWidget(_zoomInButton);
  buttonsLayout->addWidget(_zoomOutButton);
  buttonsLayout->addWidget(_resetZoomButton);
  buttonsLayout->addWidget(_fitToViewButton);
  // Insert layout in widget
  _zoomToolBox->setLayout(buttonsLayout);
}


//
void MapperGLCanvas::mousePressEvent(QMouseEvent* event)
{
  bool mousePressedOnSomething = false;

  _mousePressedPosition = event->pos();
  QPointF pos = mapToScene(event->pos());

  // Drag the scene with middle button.
  if (event->buttons() & Qt::MiddleButton)
  {
    // NOTE: This is a trick code to implement scroll hand drag using the middle button.
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->pos(), event->globalPos(), Qt::LeftButton, 0, event->modifiers());
    QGraphicsView::mouseReleaseEvent(&releaseEvent);
    setDragMode(QGraphicsView::ScrollHandDrag);

    // We need to pretend it is actually the left button that was pressed!
    QMouseEvent fakeEvent(event->type(), event->pos(), event->globalPos(),
                          Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
    QGraphicsView::mousePressEvent(&fakeEvent);
  }

  // Check for vertex selection first.
  else if (event->buttons() & Qt::LeftButton)
  {
    MShape::ptr shape = getCurrentShape();
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

          _vertexGrabbed = true;
          mousePressedOnSomething = true;

          _grabbedObjectStartScenePosition = shape->getVertex(i);
        }
      }
    }
  }

  if (mousePressedOnSomething)
    return;

  // Check for shape selection.
  if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) // Add Right click for context menu
  {
    MShape::ptr selectedShape = getCurrentShape();

    // Possibility of changing shape in output by clicking on it.
    MappingManager manager = getMainWindow()->getMappingManager();
    QVector<Mapping::ptr> mappings = manager.getVisibleMappings();
    for (QVector<Mapping::ptr>::const_iterator it = mappings.end() - 1; it >= mappings.begin(); --it)
    {
      MShape::ptr shape = getShapeFromMappingId((*it)->getId());

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

        _grabbedObjectStartScenePosition = pos;
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

  // Wrap-up dragging the scene with middle button.
  if (event->buttons() & Qt::MiddleButton)
  {
    QMouseEvent fakeEvent(event->type(), event->pos(), event->globalPos(),
        Qt::LeftButton, event->buttons() & ~Qt::LeftButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(&fakeEvent);
    setDragMode(QGraphicsView::NoDrag);
    setCursor(Qt::ArrowCursor);
  }
//  // Click on vertex ==> select the vertex.
//  if ((event->buttons() & Qt::LeftButton) && _mousePressedOnVertex)
//  {
//  }
  if (_vertexGrabbed)
  {
    // TODO : code repetition here!!!
    QPointF p = mapToScene(event->pos());

    // Stick to vertices.
    if (_mainWindow->stickyVertices())
      _glueVertex(&p);

    undoStack->push(new MoveVertexCommand(this, TransformShapeCommand::RELEASE, _activeVertex, p));
  }
  else if (_shapeGrabbed)
  {
    undoStack->push(new TranslateShapeCommand(this, TransformShapeCommand::RELEASE, QPointF()));
  }
  _vertexGrabbed = false;
  _shapeGrabbed = false;
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
  static QPoint lastMousePos;

  QPointF scenePos = mapToScene(event->pos());

  // Prepare to store commands
  undoStack = getMainWindow()->getUndoStack();

  // Vertex grab.
  if (_vertexGrabbed)
  {
    // std::cout << "Move event " << std::endl;
    MShape::ptr shape = getCurrentShape();
    if (shape && hasActiveVertex())
    {
//      QPointF p = shape->getVertex(_activeVertex);
//      // Set point to mouse coordinates.
//      p.setX(pos.x());
//      p.setY(pos.y());

      QPointF p = scenePos;

      // Stick to vertices.
      if (_mainWindow->stickyVertices())
        _glueVertex(&p);

      undoStack->push(new MoveVertexCommand(this, TransformShapeCommand::FREE, _activeVertex, p));
    }
  }

  // Shape grab.
  else if (_shapeGrabbed)
  {
    // std::cout << "Move event " << std::endl;
    MShape::ptr shape = getCurrentShape();
    if (shape)
    {
      if (_shapeFirstGrab)
      {
        lastMousePos = _mousePressedPosition;
        _shapeFirstGrab = false;
      }
    }

    QPointF diff = scenePos - mapToScene(lastMousePos);
    undoStack->push(new TranslateShapeCommand(this, TransformShapeCommand::FREE, diff));
  }

  // Window translation action
  else if (event->buttons() & Qt::MiddleButton)
  {
    QPointF diff = event->pos() - lastMousePos;
    QGraphicsView* view = scene()->views().first();
    view->translate(diff.x(), diff.y());
//    view->update();
  }

  // Reset last mouse position.
  lastMousePos = event->pos();
}


void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
{
  // Prepare to store commands.
  undoStack = getMainWindow()->getUndoStack();

  // Checks if the key has been handled by this function or needs to be deferred to superclass.
  bool handledKey = false;
  // Active vertex selected.
  if (hasActiveVertex())
  {
    MShape::ptr shape = getCurrentShape();
    QPoint pos = mapFromScene(shape->getVertex(_activeVertex));
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
      pos.ry()--;
      break;
    case Qt::Key_Down:
      pos.ry()++;
      break;
    case Qt::Key_Right:
      pos.rx()++;
      break;
    case Qt::Key_Left:
      pos.rx()--;
      break;
    default:
        handledKey = false;
      break;
    }

    // Remap window position to scene.
    QPointF scenePos = mapToScene(pos);

    // TODO: this will always be called even if no arrow key has been pressed (small performance issue).
    // Enable to Undo and Redo when arrow keys move the position of vertices
    undoStack->push(new MoveVertexCommand(this, TransformShapeCommand::STEP, _activeVertex, scenePos));
  } else {
    handledKey = true;
    if (event->matches(QKeySequence::Undo))
      undoStack->undo();
    else if (event->matches(QKeySequence::Redo))
      undoStack->redo();
    else if (event->matches(QKeySequence::ZoomIn))
      increaseZoomLevel();
    else if (event->matches(QKeySequence::ZoomOut))
      decreaseZoomLevel();
    else if (event->modifiers() == Qt::CTRL)  {
      if(event->key() == Qt::Key_0)
        resetZoomLevel();
    }
    else
      handledKey = false;
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
  _vertexGrabbed = false;
}

void MapperGLCanvas::deselectAll()
{
  deselectVertices();
  _shapeGrabbed = false;
  _shapeFirstGrab = false;
}

void MapperGLCanvas::wheelEvent(QWheelEvent *event)
{
  // [-120]-----[-1]|[1]++++++[120]
  _deltaLevel = event->delta() / 120;

  if (_deltaLevel > 0)
  {
    // Increase zoom level
    increaseZoomLevel();
  }
  else
  {
    // Decrease zoom level
    decreaseZoomLevel();
  }

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

void MapperGLCanvas::increaseZoomLevel()
{
  qreal zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);

  if (zoomFactor < MM::ZOOM_MAX) {
    _zoomLevel++;
    if (_deltaLevel)
      _deltaLevel--;
    zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
  }
  zoomFactor = qMin(zoomFactor, MM::ZOOM_MAX);

  // Reset adaptation
  _shapeIsAdapted = false;

  // Apply to view
  applyZoomToView();
}

void MapperGLCanvas::decreaseZoomLevel()
{
  qreal zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);

  if (zoomFactor > MM::ZOOM_MIN) {
    _zoomLevel--;
    if (_deltaLevel)
      _deltaLevel++;
    zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
  }
  zoomFactor = qMax(zoomFactor, MM::ZOOM_MIN);

  // Reset adaptation
  _shapeIsAdapted = false;

  // Apply to view
  applyZoomToView();
}

void MapperGLCanvas::resetZoomLevel()
{
  // Reset zoom level to zero
  _zoomLevel = 0;

  // Reset adaptation
  _shapeIsAdapted = false;

  // Apply to view
  applyZoomToView();
}

void MapperGLCanvas::fitShapeInView()
{
  // Reset zoom level before fit it
  //resetZoomLevel();
  // Get first of the list of all the views
  QGraphicsView* view = scene()->views().first();
  // Scales the view matrix
  view->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
  // Get the horizontal scaling factor
  _scalingFactor = view->matrix().m11();

  // Adapt shape
  _shapeIsAdapted = true;
}

void MapperGLCanvas::showZoomToolBar(bool visible)
{
  if (visible)
    _zoomToolBox->show();
  else
    _zoomToolBox->hide();
}

void MapperGLCanvas::enableZoomToolButtons(bool enabled)
{
  // Enable/Disable all button
  _zoomInButton->setEnabled(enabled);
  _zoomOutButton->setEnabled(enabled);
  _resetZoomButton->setEnabled(enabled);
  _fitToViewButton->setEnabled(enabled);
}


void MapperGLCanvas::_glueVertex(QPointF* p)
{
  MappingManager manager = MainWindow::instance()->getMappingManager();
  for (int i = 0; i < manager.nMappings(); i++)
  {
    MShape *shape = manager.getMapping(i)->getShape().data();
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
