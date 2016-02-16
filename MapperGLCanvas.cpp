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

  // Create zoom tools layout
  createZoomToolsLayout();
  // Disable zoom tool buttons
  enableZoomToolBar(false);

  // TODO: do we need to delete scene (or call new QGraphicsScene(this)?)
  setScene(scene ? scene : new QGraphicsScene);

  // Black background.
  this->scene()->setBackgroundBrush(Qt::black);
}

MShape::ptr MapperGLCanvas::getCurrentShape()
{
  return getShapeFromMapping(MainWindow::instance()->getCurrentMapping());
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getCurrentShapeGraphicsItem()
{
  return getShapeGraphicsItemFromMapping(MainWindow::instance()->getCurrentMapping());
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
  // Update dropdown menu
  updateDropdownMenu();
}

void MapperGLCanvas::createZoomToolsLayout()
{
  // Create zoom tool bar
  _zoomToolBar = new QWidget(this);
  _zoomToolBar->setObjectName("zoom-toolbox");

  // Create vertical layout for widgets
  QHBoxLayout* buttonsLayout = new QHBoxLayout;
  buttonsLayout->setContentsMargins(0, 0, 5, 0);
  // Create buttons
  // Zoom In button
  _zoomInButton = new QPushButton;
  _zoomInButton->setIcon(QIcon(":/zoom-in"));
  _zoomInButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _zoomInButton->setToolTip(tr("Enlarge the shape"));
  _zoomInButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _zoomInButton->setObjectName("zoom-in");
  connect(_zoomInButton, SIGNAL(clicked()), this, SLOT(increaseZoomLevel()));
  // Zoom Out button
  _zoomOutButton = new QPushButton;
  _zoomOutButton->setIcon(QIcon(":/zoom-out"));
  _zoomOutButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _zoomOutButton->setToolTip(tr("Shrink the shape"));
  _zoomOutButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _zoomOutButton->setObjectName("zoom-out");
  connect(_zoomOutButton, SIGNAL(clicked()), this, SLOT(decreaseZoomLevel()));
  // Reset to normal size button.
  _resetZoomButton = new QPushButton;
  _resetZoomButton->setIcon(QIcon(":/reset-zoom"));
  _resetZoomButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _resetZoomButton->setToolTip(tr("Reset the shape to the normal size"));
  _resetZoomButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _resetZoomButton->setObjectName("reset-zoom");
  connect(_resetZoomButton, SIGNAL(clicked()), this, SLOT(resetZoomLevel()));
  // Fit to view button
  _fitToViewButton = new QPushButton;
  _fitToViewButton->setIcon(QIcon(":/zoom-fit"));
  _fitToViewButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _fitToViewButton->setToolTip(tr("Fit the shape to content view"));
  _fitToViewButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _fitToViewButton->setObjectName("zoom-fit");
  connect(_fitToViewButton, SIGNAL(clicked()), this, SLOT(fitShapeInView()));

  // Create separator
  QFrame *separator = new QFrame(_zoomToolBar);
  separator->setFixedSize(5, 30);
  separator->setFrameShape(QFrame::VLine);

  // Create the dropdowm menu
  _dropdownMenu = new QComboBox;
  // make some settings
  _dropdownMenu->setObjectName("dropdown-menu");
  // Create if empty or update list
  updateDropdownMenu();
  // And listen
  connect(_dropdownMenu, SIGNAL(activated(QString)), this, SLOT(setZoomFromMenu(QString)));

  // Add widgets into layout
  buttonsLayout->addWidget(_zoomInButton);
  buttonsLayout->addWidget(_zoomOutButton);
  buttonsLayout->addWidget(_resetZoomButton);
  buttonsLayout->addWidget(_fitToViewButton);
  buttonsLayout->addWidget(separator);
  buttonsLayout->addWidget(_dropdownMenu);

  // Insert layout in widget
  _zoomToolBar->setLayout(buttonsLayout);
}

void MapperGLCanvas::updateZoomToolbar()
{
  _zoomToolBar->move(this->viewport()->width() - _zoomToolBar->width(),
                     this->viewport()->height() - _zoomToolBar->height());
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
      // Shape is locked either if either itself or its mapping is locked.
      bool shapeIsLocked = _mainWindow->getCurrentMapping()->isLocked() || shape->isLocked();

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

          // Vertex can be grabbed only if the mapping is not locked
          _vertexGrabbed = !shapeIsLocked;
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
      MShape::ptr shape = getShapeFromMapping(*it);

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
        // Shape can be grabbed only if it is not locked
        _shapeGrabbed = selectedShape->isLocked() ? false : true;
        _shapeFirstGrab = true;

        _grabbedObjectStartScenePosition = pos;
      }
    }
    // Show the shape/mapping context menu
    if (event->button() & Qt::RightButton)
    {
      if (selectedShape && selectedShape->includesPoint(pos))
      {
        emit shapeContextMenuRequested(event->pos());
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

    if (event->modifiers() & Qt::ShiftModifier) {

      // SHIFT + directional keys allow move with large steps
      if (event->key() == Qt::Key_Up)
        pos.ry() -= MM::VERTEX_MOVES_STEP;
      else if (event->key() == Qt::Key_Down)
        pos.ry() += MM::VERTEX_MOVES_STEP;
      else if (event->key() == Qt::Key_Right)
        pos.rx() += MM::VERTEX_MOVES_STEP;
      else if (event->key() == Qt::Key_Left)
        pos.rx() -= MM::VERTEX_MOVES_STEP;

      // SHIFT+Space to switch between vertex
      else if (event->key() == Qt::Key_Space) {
        if (shape)
          _activeVertex = (_activeVertex + 1) % shape->nVertices();
        pos = shape->getVertex(_activeVertex).toPoint(); // reset to new vertex
      }

      else
        handledKey = false;
    }
    else
    {
      switch (event->key()) {
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
    }

    if (handledKey)
      // Enable to Undo and Redo when arrow keys move the position of vertices
      undoStack->push(new MoveVertexCommand(this, TransformShapeCommand::STEP, _activeVertex, mapToScene(pos)));
  }

  else {
    // Take scroll bar current coordinate
    int scrollX = this->horizontalScrollBar()->value();
    int scrollY = this->verticalScrollBar()->value();

    handledKey = true;
    if (event->matches(QKeySequence::Undo))
      undoStack->undo();
    else if (event->matches(QKeySequence::Redo))
      undoStack->redo();
    // Case 1: zoom in with CTRL++
    else if (event->matches(QKeySequence::ZoomIn))
      increaseZoomLevel();
    else if (event->matches(QKeySequence::ZoomOut))
      decreaseZoomLevel();
    else if (event->modifiers() & Qt::ControlModifier) {
      if(event->key() == Qt::Key_0)
        resetZoomLevel();
      // Case 2: zoom in with CTRL+=
      else if (event->key() == Qt::Key_Equal ||
               // Case 3: zoom in with CTRL+SHIFT++
               (event->modifiers() & Qt::ShiftModifier && event->key() == Qt::Key_Plus))
          increaseZoomLevel();
    }
    else if(event->key() == Qt::Key_Up)
      scrollY -= 50;
    else if(event->key() == Qt::Key_Down)
      scrollY += 50;
    else if(event->key() == Qt::Key_Right)
      scrollX += 50;
    else if(event->key() == Qt::Key_Left)
      scrollX -= 50;
    else
      handledKey = false;

    // Set scroll bar new value
    this->verticalScrollBar()->setValue(scrollY);
    this->horizontalScrollBar()->setValue(scrollX);
  }

  // Defer unhandled keys to parent.
  if (!handledKey)
  {
    QWidget::keyPressEvent(event);
  }
}

void MapperGLCanvas::updateCanvas()
{
  update();
  scene()->update();
}

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
  // See: http://doc.qt.io/qt-5/qwheelevent.html#angleDelta
#if QT_VERSION >= 0x050500
  int deltaLevel = event->angleDelta().y() / 120;
#else
  int deltaLevel = event->delta() / 120;
#endif

  if (deltaLevel > 0)
  {
    // Increase zoom level
    increaseZoomLevel(deltaLevel);
  }
  else
  {
    // Decrease zoom level
    decreaseZoomLevel(-deltaLevel);
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

void MapperGLCanvas::increaseZoomLevel(int steps)
{
  qreal zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);

  while (steps > 0 && zoomFactor < MM::ZOOM_MAX) {
    _zoomLevel++;
    zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
    steps--;
  }
  zoomFactor = qMin(zoomFactor, MM::ZOOM_MAX);

  // Reset adaptation
  _shapeIsAdapted = false;

  // Apply to view
  applyZoomToView();
}

void MapperGLCanvas::decreaseZoomLevel(int steps)
{
  qreal zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);

  while (steps > 0 && zoomFactor > MM::ZOOM_MIN) {
    _zoomLevel--;
    zoomFactor = qPow(MM::ZOOM_FACTOR, _zoomLevel);
    steps--;
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
  // Get first of the list of all the views
  QGraphicsView* view = scene()->views().first();
  // Scales the view matrix
  view->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
  // Get the horizontal scaling factor
  _scalingFactor = view->matrix().m11();

  // Adapt shape
  _shapeIsAdapted = true;

  // Update zoom menu list
  updateDropdownMenu();
}

void MapperGLCanvas::showZoomToolBar(bool visible)
{
  if (visible)
    _zoomToolBar->show();
  else
    _zoomToolBar->hide();
}

void MapperGLCanvas::enableZoomToolBar(bool enabled)
{
  // Enable/Disable all button
  _zoomInButton->setEnabled(enabled);
  _zoomOutButton->setEnabled(enabled);
  _resetZoomButton->setEnabled(enabled);
  _fitToViewButton->setEnabled(enabled);
  _dropdownMenu->setEnabled(enabled);
}

void MapperGLCanvas::setZoomFromMenu(const QString &text)
{
  // Get text choosen by user and convert it to double
  qreal zoomFactor = text.mid(0, text.length() - 1).toDouble();
  // Set zoom factor
  _scalingFactor = zoomFactor / 100;

  // Adapt shape
  _shapeIsAdapted = true;

  // Apply to view
  applyZoomToView();
}

void MapperGLCanvas::updateDropdownMenu()
{
  // Get current zoom factor percentage
  QString zoomFactor = QString::number(int(getZoomFactor() * 100)).append(QChar('%'));
  //Create list
  QStringList zoomFactorList;
  zoomFactorList << "400%" << "300%" << "200%" << "150%" << "125%" <<
                    "100%" << "75%" << "50%" << "25%" << "12.5%";
  // Avoid duplicate
  if (!zoomFactorList.contains(zoomFactor))
    zoomFactorList.append(zoomFactor);
  // Clear if is not empty
  _dropdownMenu->clear();
  // Add list item
  _dropdownMenu->addItems(zoomFactorList);
  // Select 100% by default
  _dropdownMenu->setCurrentText(zoomFactor);
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
