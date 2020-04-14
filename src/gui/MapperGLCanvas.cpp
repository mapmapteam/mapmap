/*
 * MapperGLCanvas.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2014 Dame Diongue -- baydamd(@)gmail(.)com
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

#include "ShapeGraphicsItem.h"
#include "MapperGLCanvas.h"

#include "MainWindow.h"
#include "Commands.h"

namespace mmp {

MapperGLCanvas::MapperGLCanvas(MainWindow* mainWindow,
                               bool isOutput, QWidget* parent, const QGLWidget * shareWidget,
                               QGraphicsScene* scene)
  : QGraphicsView(parent),
    _mainWindow(mainWindow),
    _isOutput(isOutput),
    _vertexGrabbed(false),
    _vertexMoved(false),
    _activeVertex(NO_VERTEX),
    _shapeGrabbed(false), // comment out?
    _shapeMoved(false),
    _shapeFirstGrab(false), // comment out?
    _zoomLevel(0),
    _shapeIsAdapted(false)
{
  // For now clicking on the window doesn't do anything.
  setDragMode(QGraphicsView::NoDrag);

  setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                 QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform);
  // Dont need to always see scroll bar
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  this->horizontalScrollBar()->setRange(0, 1000);
  this->verticalScrollBar()->setRange(0, 1000);

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

  // Enable dragndrop
  setAcceptDrops(true);

  // Prepare to store commands
  undoStack = mainWindow->getUndoStack();
}

MShape::ptr MapperGLCanvas::getShapeFromMapping(Mapping::ptr mapping)
{
  if (mapping.isNull())
  {
    return MShape::ptr();
  }
  else
  {
    return (isOutput() ? mapping->getShape() : mapping->getInputShape());
  }
}

MShape::ptr MapperGLCanvas::getCurrentShape()
{
  return getShapeFromMapping(MainWindow::window()->getCurrentMapping());
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getShapeGraphicsItemFromMapping(Mapping::ptr mapping)
{
  if (mapping.isNull())
  {
    return QSharedPointer<ShapeGraphicsItem>();
  }
  else
  {
    MappingGui::ptr mappingGui = MainWindow::window()->getMappingGuiByMappingId(mapping->getId());
    return (isOutput() ? mappingGui->getGraphicsItem() : mappingGui->getInputGraphicsItem());
  }
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getCurrentShapeGraphicsItem()
{
  return getShapeGraphicsItemFromMapping(_mainWindow->getCurrentMapping());
}

// Draws foreground (displays crosshair if needed).
void MapperGLCanvas::drawForeground(QPainter *painter , const QRectF &rect)
{
  Q_UNUSED(rect);
  if (MainWindow::window()->displayControls())
  {
    uid mid = MainWindow::window()->getCurrentMappingId();
    if (mid != NULL_UID)
    {

      // Display other controls.
      if (_mainWindow->displayPaintControls())
      {
        QMap<uid, Mapping::ptr> paintMappings = _mainWindow->getMappingManager().getPaintMappings( _mainWindow->getCurrentPaint() );
        for (QMap<uid, Mapping::ptr>::const_iterator it = paintMappings.constBegin();
             it != paintMappings.constEnd(); ++it)
        {
          if (it.key() != mid)
          {
            ShapeGraphicsItem::ptr paintMappingItem = getShapeGraphicsItemFromMapping(it.value());
            if (paintMappingItem)
              paintMappingItem->getControlPainter()->paintShape(painter, this, false);
          }
        }
      }

      // Use current shape graphics item to draw controls.
      ShapeGraphicsItem::ptr item = getCurrentShapeGraphicsItem();
      if (item)
      {
        QList<int> selected;
        if (hasActiveVertex())
        {
          selected.push_back(getActiveVertexIndex());
        }
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
  // Resets the view transformation matrix
  resetMatrix();
  // Scale the current view
  scale(zoomFactor, zoomFactor);
  // And update
  update();

  emit zoomFactorChanged(zoomFactor);
}

void MapperGLCanvas::dragEnterEvent(QDragEnterEvent *event)
{
  const QMimeData *mimeData = event->mimeData();
  bool allowDrag = true;

  if (mimeData->hasUrls()) {
    foreach (QUrl url, mimeData->urls()) {
      QString fileName = url.toLocalFile();
      // Don't allow drag if file is not supported
      if (!MainWindow::window()->fileSupported(fileName, MM::FILE_EXTENSION) &&
          !MainWindow::window()->fileSupported(fileName, MM::IMAGE_FILES_FILTER) &&
          !MainWindow::window()->fileSupported(fileName, MM::VIDEO_FILES_FILTER)) {
        allowDrag = false;
      }
    }
  }

  if (allowDrag)
    event->acceptProposedAction();
}

void MapperGLCanvas::dragMoveEvent(QDragMoveEvent *event)
{
  event->acceptProposedAction();
}

void MapperGLCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
}

void MapperGLCanvas::dropEvent(QDropEvent *event)
{
  const QMimeData *mimeData = event->mimeData();

  if (mimeData->hasUrls()) {
    // In case that dragged many files
    foreach (QUrl url, mimeData->urls()) {
      QString fileName = url.toLocalFile();

      if (!fileName.isEmpty()) {
        // Test if is mmp file and exit loop
        if (MainWindow::window()->fileSupported(fileName, MM::FILE_EXTENSION)) {
          MainWindow::window()->loadFile(fileName);
          // Exit for prevent drag to many project files
          break;
        }
        // Allow to drag too many videos or images
        else {
          // Check if file is image or not
          // according to file extension
          if (MainWindow::window()->fileSupported(fileName, MM::IMAGE_FILES_FILTER))
            MainWindow::window()->importMediaFile(fileName, true);
          else
            MainWindow::window()->importMediaFile(fileName, false);
        }
      }
    }
  }
  event->acceptProposedAction();
}


//
void MapperGLCanvas::mousePressEvent(QMouseEvent* event)
{
  bool mousePressedOnSomething = false;

  _mousePressedPosition = event->pos();
  QPointF pos = mapToScene(event->pos());

  // Drag the scene with middle button.
  //  if (event->buttons() & Qt::MiddleButton)
  //  {
  //    // NOTE: This is a trick code to implement scroll hand drag using the middle button.
  //    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->pos(),
  //            event->globalPos(), Qt::LeftButton, 0, event->modifiers());
  //    QGraphicsView::mouseReleaseEvent(&releaseEvent);
  //    setDragMode(QGraphicsView::ScrollHandDrag);

  //    // We need to pretend it is actually the left button that was pressed!
  //    QMouseEvent fakeEvent(event->type(), event->pos(), event->globalPos(),
  //            Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
  //    QGraphicsView::mousePressEvent(&fakeEvent);
  //  }

  // Check for shape selection.
  if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
  {
    MShape::ptr selectedShape = getCurrentShape();
    bool shapeSelectionChange = false;

    // Check for vertex selection first.
    if (event->buttons() & Qt::LeftButton)
    {
      if (selectedShape)
      {
        // Note: we compare with the square value for fastest computation of the distance
        int minDistance = sq(MM::VERTEX_SELECT_RADIUS);

        _grabbedShapeStartCenterScenePosition = selectedShape->getCenter();
        _grabbedShapeCopy.reset(selectedShape->clone());

        // Find the ID of the nearest vertex (from currently selected shape)
        for (int i = 0; i < selectedShape->nVertices(); i++)
        {
          int dist = distSq(_mousePressedPosition, mapFromScene(selectedShape->getVertex(i))); // squared distance
          if (dist < minDistance)
          {
            _activeVertex = i;
            minDistance = dist;

            // Vertex can be grabbed only if the mapping is not locked
            _vertexGrabbed = !selectedShape->isLocked();
            _vertexMoved = false; // Active vertex may not moved
            mousePressedOnSomething = true;

            _grabbedObjectStartScenePosition = selectedShape->getVertex(i);
          }
        }
      }
    }

    // Possibility of changing shape in output by clicking on it.
    MappingManager manager = getMainWindow()->getMappingManager();
    QVector<Mapping::ptr> mappings = manager.getVisibleMappings();
    for (QVector<Mapping::ptr>::const_iterator it = mappings.begin();
         it != mappings.end(); ++it)
    {
      MShape::ptr shape = getShapeFromMapping(*it);

      // Check if mouse was pressed on that shape.
      if (shape && !_vertexGrabbed && shape->includesPoint(pos))
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
          shapeSelectionChange = true;
        }

        break; // Exit loop
      }
    }

    if (selectedShape && !_vertexGrabbed && selectedShape->includesPoint(pos)) {
      if (event->buttons() & Qt::LeftButton) {
        // Shape can be grabbed only if it is not locked
        _shapeGrabbed = !selectedShape->isLocked();
//        _shapeMoved = false;
        _shapeFirstGrab = true;

        _grabbedObjectStartScenePosition = pos;
        //				_grabbedShapeStartCenterScenePosition = selectedShape->getCenter();

        if (shapeSelectionChange) { // if fresh selected shape
          // Reset shape Mode
          selectedShape->setShapeMode(MShape::DefaultMode);
        } else {
          // Move to the next mode
          selectedShape->setShapeMode(selectedShape->shapeModeState(), true);
        }
      }

      // Add Right click for context menu
      if (event->buttons() & Qt::RightButton) {
        emit shapeContextMenuRequested(event->pos()); // Show the shape/mapping context menu
      }
    }
  }

  if (mousePressedOnSomething)
  {
    return;
  }

  // Deactivate.
  deselectAll();
}

void MapperGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
  Q_UNUSED(event);

  // Wrap-up dragging the scene with middle button.
  if (event->buttons() & Qt::MiddleButton)
  {
    QMouseEvent fakeEvent(
          event->type(), event->pos(), event->globalPos(),
          Qt::LeftButton, event->buttons() & ~Qt::LeftButton,
          event->modifiers());
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
    if (xOr(_mainWindow->isStickyVertices(), (event->modifiers() & Qt::ShiftModifier)))
    {
      _snapVertex(&p);
    }

    switch (getCurrentShape()->shapeModeState()) {
      case MShape::RotateMode:
        undoStack->push(new ScaleRotateShapeCommand(this,
                                                    TransformShapeCommand::RELEASE, _activeVertex, p, _grabbedObjectStartScenePosition, _grabbedShapeCopy, MShape::RotateMode));
        break;
      case MShape::ScaleMode:
        undoStack->push(new ScaleRotateShapeCommand(this,
                                                    TransformShapeCommand::RELEASE, _activeVertex, p, _grabbedObjectStartScenePosition, _grabbedShapeCopy, MShape::ScaleMode));
        break;
      default:
        if (_vertexMoved)
          undoStack->push(new MoveVertexCommand(this,
                                                TransformShapeCommand::RELEASE, _activeVertex, p));
        break;
    }


  }
  else if (_shapeGrabbed)
  {
    if (_shapeMoved)
      undoStack->push(new TranslateShapeCommand(this,
                                                TransformShapeCommand::RELEASE, QPointF()));
  }

  _vertexGrabbed = false;
  _vertexMoved = false;
  _shapeGrabbed = false;
  _shapeMoved = false;
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
  static QPoint lastMousePos;

  QPointF scenePos = mapToScene(event->pos());

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
      _vertexMoved = true; // The active vertex is actually moved

      // Stick to vertices.
      if (xOr(_mainWindow->isStickyVertices(), (event->modifiers() & Qt::ShiftModifier)))
      {
        _snapVertex(&p);
      }

      switch (getCurrentShape()->shapeModeState()) {
        case MShape::RotateMode:
          undoStack->push(new ScaleRotateShapeCommand(this,
                                                      TransformShapeCommand::FREE, _activeVertex, p, _grabbedObjectStartScenePosition, _grabbedShapeCopy, MShape::RotateMode));
          break;
        case MShape::ScaleMode:
          undoStack->push(new ScaleRotateShapeCommand(this,
                                                      TransformShapeCommand::FREE, _activeVertex, p, _grabbedObjectStartScenePosition, _grabbedShapeCopy, MShape::ScaleMode));
          break;
        default:
          undoStack->push(new MoveVertexCommand(this,
                                                TransformShapeCommand::FREE, _activeVertex, p));
          break;
      }
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
        // Reset the mode after moved shape
        getCurrentShape()->setShapeMode(MShape::DefaultMode);
      }

      _shapeMoved = true; // The active vertex is actually moved
    }
    QPointF diff = scenePos - mapToScene(lastMousePos);
    undoStack->push(new TranslateShapeCommand(this, TransformShapeCommand::FREE, diff));
  }

  // Window translation action
  else if ((event->buttons() & Qt::MiddleButton) ||
           ((event->modifiers() & Qt::ShiftModifier) && (event->buttons() & Qt::LeftButton)))
  {
    QPointF diff = event->pos() - lastMousePos;
    QGraphicsView* view = scene()->views().first();
    view->translate(diff.x(), diff.y());
  }

  // Reset last mouse position.
  lastMousePos = event->pos();
}


void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
{
  // Checks if the key has been handled by this function or needs to be deferred to superclass.
  bool handledKey = false;

  // Active vertex selected.
  if (hasActiveVertex())
  {
    MShape::ptr shape = getCurrentShape();
    QPoint pos = mapFromScene(shape->getVertex(_activeVertex));
    handledKey = true;

    if (event->modifiers() & Qt::ShiftModifier)
    {
      // SHIFT + directional keys allow move with large steps
      switch (event->key())
      {
        case Qt::Key_Up:
          pos.ry() -= MM::BigStep;
          break;
        case Qt::Key_Down:
          pos.ry() += MM::BigStep;
          break;
        case Qt::Key_Right:
          pos.rx() += MM::BigStep;
          break;
        case Qt::Key_Left:
          pos.rx() -= MM::BigStep;
          break;
          // SHIFT+Space to switch between vertex
        case Qt::Key_Space:
          _activeVertex = (_activeVertex + 1) % shape->nVertices();
          pos = mapFromScene(shape->getVertex(_activeVertex)); // reset to new vertex
          break;
        default:
          handledKey = false;
          break;
      }
    }
    else if (event->modifiers() & Qt::AltModifier)
    {
      switch (event->key())
      {
        case Qt::Key_Up:
          pos.ry() -= MM::SmallStep;
          break;
        case Qt::Key_Down:
          pos.ry() += MM::SmallStep;
          break;
        case Qt::Key_Right:
          pos.rx() += MM::SmallStep;
          break;
        case Qt::Key_Left:
          pos.rx() -= MM::SmallStep;
          break;
        default:
          handledKey = false;
          break;
      }
    }
    else
    {
      switch (event->key())
      {
        case Qt::Key_Up:
          pos.ry() -= MM::MediumStep;
          break;
        case Qt::Key_Down:
          pos.ry() += MM::MediumStep;
          break;
        case Qt::Key_Right:
          pos.rx() += MM::MediumStep;
          break;
        case Qt::Key_Left:
          pos.rx() -= MM::MediumStep;
          break;
        default:
          handledKey = false;
          break;
      }
    }

    if (handledKey)
    {
      // Enable to Undo and Redo when arrow keys move the position of vertices
      undoStack->push(new MoveVertexCommand(this,
                                            TransformShapeCommand::STEP, _activeVertex, mapToScene(pos)));
    }
  }
  else
  {
    // Take scroll bar current coordinate
    int scrollX = this->horizontalScrollBar()->value();
    int scrollY = this->verticalScrollBar()->value();

    handledKey = true;
    if (event->matches(QKeySequence::Undo))
    {
      undoStack->undo();
    }
    else if (event->matches(QKeySequence::Redo))
    {
      undoStack->redo();
    }
    // Case 1: zoom in with CTRL++
    else if (event->matches(QKeySequence::ZoomIn))
    {
      increaseZoomLevel();
    }
    else if (event->matches(QKeySequence::ZoomOut))
    {
      decreaseZoomLevel();
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
      if (event->key() == Qt::Key_0)
      {
        resetZoomLevel();
      }
      // Case 2: zoom in with CTRL+=
      else if (event->key() == Qt::Key_Equal ||
               // Case 3: zoom in with CTRL+SHIFT++
               ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Plus))
      {
        increaseZoomLevel();
      }
    }
    else if(event->key() == Qt::Key_Up)
    {
      scrollY -= 50;
    }
    else if(event->key() == Qt::Key_Down)
    {
      scrollY += 50;
    }
    else if(event->key() == Qt::Key_Right)
    {
      scrollX += 50;
    }
    else if(event->key() == Qt::Key_Left)
    {
      scrollX -= 50;
    }
    else
    {
      handledKey = false;
    }

    // Set scroll bar new value
    this->verticalScrollBar()->setValue(scrollY);
    this->horizontalScrollBar()->setValue(scrollX);
  }

  if (getCurrentShape()) {
    switch (event->key()) {
      case Qt::Key_M:
        getCurrentShape()->setShapeMode(MShape::DefaultMode);
        break;
      case Qt::Key_S:
        getCurrentShape()->setShapeMode(MShape::ScaleMode);
        break;
      case Qt::Key_R:
        getCurrentShape()->setShapeMode(MShape::RotateMode);
        break;
      default:
        break;
    }
  }

  // Defer unhandled keys to parent.
  if (! handledKey)
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
  _vertexMoved = false;
}

void MapperGLCanvas::deselectAll()
{
  deselectVertices();
  _shapeGrabbed = false;
  _shapeMoved = false;
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
  bool control_is_pressed = event->modifiers().testFlag(Qt::ControlModifier);
  bool shift_is_pressed = event->modifiers().testFlag(Qt::ShiftModifier);

  if (control_is_pressed) { // control is pressed: zoom
    // zoom in or out:
    if (deltaLevel > 0) {
      // Increase zoom level
      increaseZoomLevel(deltaLevel);
    } else {
      // Decrease zoom level
      decreaseZoomLevel(-deltaLevel);
    }
    // Accept wheel scrolling event.
    event->accept();
  } else { // control is not pressed: scroll
    QScrollBar* scrollbar;
    if (shift_is_pressed) { // shift is pressed: pans horizontally
      scrollbar = this->horizontalScrollBar();
    } else { // shift is not pressed: scrolls vertically
      scrollbar = this->verticalScrollBar();
    }
    // FIXME: scrolling with the mouse doesn't currently work
    int scroll = scrollbar->value();
    if (deltaLevel > 0) {
      scrollbar->setValue(scroll + 50);
    } else {
      scrollbar->setValue(scroll - 50);
    }
    event->accept();
  }
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

void MapperGLCanvas::fitShapeToView()
{
  if (getCurrentShape()) { // Test if current shape exists
    // Get first of the list of all the views
    // Scales the view matrix
    fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    // Center all shapes
    setSceneRect(scene()->itemsBoundingRect());
    centerOn(this->scene()->itemsBoundingRect().center());
    // Get the horizontal scaling factor
    _scalingFactor = matrix().m11();

    // Adapt shape
    _shapeIsAdapted = true;

    emit zoomFactorChanged(getZoomFactor());
  }
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

void MapperGLCanvas::_snapVertex(QPointF* p)
{
  QSettings settings;
  int vertexStickRadius = settings.value("vertexStickRadius", MM::VERTEX_STICK_RADIUS).toInt();
  MappingManager manager = MainWindow::window()->getMappingManager();
  MShape::ptr currentShape = getCurrentShape();
  for (int i = 0; i < manager.nMappings(); i++)
  {
    MShape::ptr shape = getShapeFromMapping(manager.getMapping(i));
    if (shape && shape != currentShape)
    {
      for (int vertex = 0; vertex < shape->nVertices(); vertex++)
      {
        const QPointF& v = shape->getVertex(vertex);
        if (distIsInside(v, *p, vertexStickRadius))
        {
          p->setX(v.x());
          p->setY(v.y());
        }
      }
    }
  }
}

}
