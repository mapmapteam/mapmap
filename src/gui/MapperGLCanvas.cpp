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

#include <QApplication>
#include <QScopeGuard>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QResizeEvent>
#include <QCursor>
#include <QSurfaceFormat>

namespace mmp {

MapperGLCanvas::MapperGLCanvas(MainWindow* mainWindow,
                               bool isOutput, QWidget* parent, QOpenGLWidget* shareWidget,
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
    _zoom(1.0),
    _pressMode(HandleMode::Free),
    _handleMoved(false),
    _wheelZoomRemainder(0),
    _frameStats(isOutput ? "output editor" : "input editor"),
    _lastPaintStartNs(0)
{
  Q_UNUSED(shareWidget);
  // For now clicking on the window doesn't do anything.
  setDragMode(QGraphicsView::NoDrag);

  setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                 QPainter::SmoothPixmapTransform);
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
  // The point under the mouse is kept in place by _setZoom().
  setTransformationAnchor(QGraphicsView::NoAnchor);

  resetTransform();
  // setAcceptDrops(true);

  // Render with OpenGL.
  QOpenGLWidget* glw = new QOpenGLWidget(this);
  QSurfaceFormat fmt;
  fmt.setSamples(4); // replaces QGL::SampleBuffers
  glw->setFormat(fmt);
  setViewport(glw);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

  // Show the position of the mouse in the output even when no button is pressed.
  viewport()->setMouseTracking(isOutput);

  // Draw the handle again when R or S changes its mode.
  // During a drag, the new mode applies from the state reached.
  connect(_mainWindow->handleModeKeys(), &HandleModeKeys::modeChanged, this, [this]() {
    if (_handleDrag)
      _handleDrag->setMode(_mainWindow->handleModeKeys()->mode());
    viewport()->update();
  });

  // TODO: do we need to delete scene (or call new QGraphicsScene(this)?)
  setScene(scene ? scene : new QGraphicsScene);

  // Black background.
  this->scene()->setBackgroundBrush(Qt::black);

  // Enable dragndrop
  setAcceptDrops(true);

  // Prepare to store commands
  undoStack = mainWindow->getUndoStack();
}

MShape::ptr MapperGLCanvas::getShapeFromMapping(Layer::ptr mapping)
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
  return getShapeFromMapping(MainWindow::window()->getCurrentLayer());
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getShapeGraphicsItemFromMapping(Layer::ptr mapping)
{
  if (mapping.isNull())
  {
    return QSharedPointer<ShapeGraphicsItem>();
  }
  else
  {
    LayerGui::ptr layerGui = MainWindow::window()->getLayerGuiByLayerId(mapping->getId());
    return (isOutput() ? layerGui->getGraphicsItem() : layerGui->getInputGraphicsItem());
  }
}

QSharedPointer<ShapeGraphicsItem> MapperGLCanvas::getCurrentShapeGraphicsItem()
{
  return getShapeGraphicsItemFromMapping(_mainWindow->getCurrentLayer());
}

void MapperGLCanvas::paintEvent(QPaintEvent* event)
{
  if (!FrameStats::isEnabled())
  {
    QGraphicsView::paintEvent(event);
    return;
  }

  const qint64 startNs = FrameStats::now();
  if (_lastPaintStartNs > 0)
    _frameStats.record(FrameStats::INTERVAL, startNs - _lastPaintStartNs, startNs);
  _lastPaintStartNs = startNs;

  QGraphicsView::paintEvent(event);

  // OpenGL works asynchronously: without glFinish() this only times the
  // sending of the commands. The context of the viewport is still current.
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (FrameStats::isSynchronous() && context)
    context->functions()->glFinish();
  const qint64 endNs = FrameStats::now();
  _frameStats.record(FrameStats::DRAW, endNs - startNs, endNs);
}

// Draws foreground (displays crosshair if needed).
void MapperGLCanvas::drawForeground(QPainter *painter , const QRectF &rect)
{
  Q_UNUSED(rect);
  if (MainWindow::window()->displayControls())
  {
    uid mid = MainWindow::window()->getCurrentLayerId();
    if (mid != NULL_UID)
    {

      // Display other controls.
      if (_mainWindow->displaySourceControls())
      {
        QMap<uid, Layer::ptr> sourceLayers = _mainWindow->getMappingManager().getSourceLayers( _mainWindow->getCurrentSource() );
        for (QMap<uid, Layer::ptr>::const_iterator it = sourceLayers.constBegin();
             it != sourceLayers.constEnd(); ++it)
        {
          if (it.key() != mid)
          {
            ShapeGraphicsItem::ptr sourceLayerItem = getShapeGraphicsItemFromMapping(it.value());
            if (sourceLayerItem)
              sourceLayerItem->getControlPainter()->paintShape(painter, this, false);
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
        // With the vertex that goes with the one selected on the other side.
        item->getControlPainter()->paint(painter, this, selected, _linkedVertex());
      }

    }
  }
}

void MapperGLCanvas::applyZoomToView()
{
  _setZoom(_zoom, _zoomAnchor());
}

void MapperGLCanvas::setContentRect(const QRectF& rect)
{
  if (rect == _contentRect)
    return;
  _contentRect = rect;

  // Frame the new content at the current zoom.
  _setZoom(_zoom, viewport()->rect().center());
  if (!_contentRect.isEmpty())
    centerOn(_contentRect.center());
}

qreal MapperGLCanvas::_fitScale() const
{
  return _fitScale(viewport()->size());
}

qreal MapperGLCanvas::_fitScale(const QSizeF& viewSize) const
{
  // Leave room around the content for the label above its outline, for the
  // controls of vertices placed on its edges and, most of all, for the handle
  // of a shape that covers it all.
  const qreal margin = MM::HANDLE_DISTANCE + MM::HANDLE_RADIUS + FIT_MARGIN_SPACING;
  const QSizeF area = viewSize - QSizeF(2 * margin, 2 * margin);
  return Util::fitScale(_contentRect.size(), area.isEmpty() ? viewSize : area);
}

QPointF MapperGLCanvas::_zoomAnchor() const
{
  const QPoint mouse = viewport()->mapFromGlobal(QCursor::pos());
  return viewport()->rect().contains(mouse) ? QPointF(mouse) : QPointF(viewport()->rect().center());
}

void MapperGLCanvas::_setZoom(qreal zoom, const QPointF& viewAnchor)
{
  // Zoom out to ZOOM_MIN and in to ZOOM_MAX, both relative to the content
  // and in screen pixels, so that 100% and 1:1 are always reachable.
  const qreal fit = _fitScale();
  const qreal minZoom = qMin(MM::ZOOM_MIN, MM::ZOOM_MIN / fit);
  const qreal maxZoom = qMax(MM::ZOOM_MAX, MM::ZOOM_MAX / fit);
  _zoom = qBound(minZoom, zoom, maxZoom);

  // Keep the scene point under the anchor in place.
  const QPointF sceneAnchor = mapToScene(viewAnchor.toPoint());
  const qreal scale = fit * _zoom;
  _updateSceneRect(scale);
  resetTransform();
  this->scale(scale, scale);
  const QPointF viewCenter = QRectF(viewport()->rect()).center();
  centerOn(sceneAnchor + (viewCenter - viewAnchor) / scale);

  update();
  emit zoomFactorChanged(_zoom);
}

void MapperGLCanvas::_updateSceneRect(qreal scale)
{
  QRectF rect = _contentRect | scene()->itemsBoundingRect();
  if (rect.isEmpty())
  {
    // Nothing to show: no need to scroll.
    setSceneRect(QRectF(QPointF(0, 0), QSizeF(viewport()->size()) / scale));
    _setScrollBarsShown(false);
    return;
  }

  // The scroll bars are only shown when the content and the shapes do not all
  // fit in the view. The view can still be moved without them. This is
  // measured on the view without its scroll bars: showing or hiding them
  // changes the size of the view, and must not change the answer.
  const QSizeF fullView = maximumViewportSize();
  const QSizeF needed = rect.size() * (_fitScale(fullView) * _zoom);
  _setScrollBarsShown(needed.width() > fullView.width() || needed.height() > fullView.height());

  // Half a view of margin on each side, so that any point of the content or
  // shapes can be brought to the middle of the view.
  const QSizeF halfView = QSizeF(viewport()->size()) / (2 * scale);
  setSceneRect(rect.adjusted(-halfView.width(), -halfView.height(), halfView.width(), halfView.height()));
}

void MapperGLCanvas::_setScrollBarsShown(bool shown)
{
  const Qt::ScrollBarPolicy policy = shown ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
  setHorizontalScrollBarPolicy(policy);
  setVerticalScrollBarPolicy(policy);
}

void MapperGLCanvas::resizeEvent(QResizeEvent* event)
{
  // Keep the same scene point in the middle, at the same relative zoom.
  const QPointF sceneCenter = mapToScene(QRect(QPoint(0, 0), event->oldSize()).center());
  QGraphicsView::resizeEvent(event);
  if (event->oldSize().isValid())
  {
    _setZoom(_zoom, viewport()->rect().center());
    centerOn(sceneCenter);
  }
  else
    fitShapeToView();
}

void MapperGLCanvas::drawBackground(QPainter* painter, const QRectF& rect)
{
  QGraphicsView::drawBackground(painter, rect);
  // Under the shapes and their controls, so that it never hides them.
  _drawContentOutline(painter);
}

void MapperGLCanvas::setContentLabel(const QString& label)
{
  _contentLabel = label;
  viewport()->update();
}

void MapperGLCanvas::_drawContentOutline(QPainter* painter)
{
  if (_contentRect.isEmpty())
    return;

  // Drawn in screen pixels, so that it looks the same at any zoom.
  const QRectF frame = viewportTransform().mapRect(_contentRect);
  painter->save();
  painter->resetTransform();
  painter->setRenderHint(QPainter::Antialiasing);

  // A rounded outline around the frame, a little away from it so that it
  // does not hide the corners of the image.
  QPen pen(MM::CONTENT_OUTLINE_COLOR, CONTENT_OUTLINE_WIDTH);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  const qreal gap = CONTENT_OUTLINE_GAP + CONTENT_OUTLINE_WIDTH / 2;
  painter->drawRoundedRect(frame.adjusted(-gap, -gap, gap, gap),
                           CONTENT_OUTLINE_RADIUS, CONTENT_OUTLINE_RADIUS);

  // Its name above it: plain text, which reads as a caption rather than as
  // something to click or drag.
  if (!_contentLabel.isEmpty())
  {
    QFont font = painter->font();
    font.setPixelSize(CONTENT_LABEL_FONT_SIZE);
    painter->setFont(font);
    const QFontMetricsF metrics(font);
    const QPointF baseline(frame.left() - gap + CONTENT_OUTLINE_RADIUS,
                           frame.top() - gap - CONTENT_LABEL_SPACING - metrics.descent());
    painter->setPen(MM::CONTENT_LABEL_COLOR);
    painter->drawText(baseline, _contentLabel);
  }

  painter->restore();
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
  // The other side shows which vertex is selected here.
  const auto updateOtherSide = qScopeGuard([this] { _updateOtherSide(); });

  bool mousePressedOnSomething = false;

  _mousePressedPosition = event->pos();
  _lastMousePosition = event->pos();
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

  // Other buttons do nothing while a handle is dragged. A left press during a
  // drag means its release was lost (the input view does not follow the mouse
  // without a button): end that drag, then handle the press as usual.
  if (_handleDrag)
  {
    if (event->button() != Qt::LeftButton)
      return;
    _endHandleDrag(true);
  }

  // The handles of the current shape come before its vertices and the other
  // shapes, which they may cover.
  if (event->button() == Qt::LeftButton)
  {
    if (_isOnHandle(event->pos()))
    {
      MShape::ptr shape = getCurrentShape();
      _grabbedShapeCopy.reset(shape->clone());
      _handleDrag.emplace(shape->getCenter(), pos, _mainWindow->handleModeKeys()->mode());
      _pressMode = _mainWindow->handleModeKeys()->mode();
      _handleShape = shape;
      _handleMoved = false;
      _handleDrag->setStepped(event->modifiers() & Qt::ShiftModifier);
      _mainWindow->handleModeKeys()->setDragActive(true);
      return;
    }
  }

  // Check for shape selection.
  if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
  {
    MShape::ptr selectedShape = getCurrentShape();

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

        // One vertex is selected at a time: the one the arrow keys move.
        if (mousePressedOnSomething)
          _deselectVerticesOnOtherSide();
      }
    }

    // Possibility of changing shape in output by clicking on it.
    MappingManager manager = getMainWindow()->getMappingManager();
    QVector<Layer::ptr> mappings = manager.getVisibleLayers();
    for (QVector<Layer::ptr>::const_iterator it = mappings.begin();
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
          getMainWindow()->setCurrentLayer((*it)->getId());

          // Reset selected shape to new one.
          selectedShape = getCurrentShape();
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

void MapperGLCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
  // Qt turns the second of two quick presses into a double click: on the
  // handle, it is a press like any other (a second click goes to the next
  // mode, a press followed by a move drags).
  if (event->button() == Qt::LeftButton && (_handleDrag || _isOnHandle(event->pos())))
    mousePressEvent(event);
  else
    QGraphicsView::mouseDoubleClickEvent(event);
}

void MapperGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
  Q_UNUSED(event);

  // Wrap-up dragging the scene with middle button.
  if (event->buttons() & Qt::MiddleButton)
  {
    QMouseEvent fakeEvent(
          event->type(), event->pos(), event->globalPosition().toPoint(),
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
  if (_handleDrag)
  {
    // Only the left button, which started the drag, ends it.
    if (event->button() == Qt::LeftButton)
    {
      // A click without a drag goes to the next mode.
      const bool clicked = !_handleMoved;
      _endHandleDrag(true);
      if (clicked)
        _mainWindow->handleModeKeys()->setMode(nextHandleMode(_mainWindow->handleModeKeys()->mode()));
    }
    return;
  }

  if (_vertexGrabbed)
  {
    // TODO : code repetition here!!!
    QPointF p = mapToScene(event->pos());

    // Stick to vertices.
    if (xOr(_mainWindow->isStickyVertices(), (event->modifiers() & Qt::ShiftModifier)))
    {
      _snapVertex(&p);
    }

    if (_vertexMoved)
      undoStack->push(new MoveVertexCommand(this,
                                            TransformShapeCommand::RELEASE, _activeVertex, p,
                                            _grabbedShapeCopy));


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

  // Shapes may have moved past the scroll bars' reach.
  _updateSceneRect(getZoomFactor());
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
  QPointF scenePos = mapToScene(event->pos());

  // Handle drag: Ctrl for fine motion, Shift for steps.
  if (_handleDrag)
  {
    // The release was lost (to a menu, for instance): end the drag here.
    if (!(event->buttons() & Qt::LeftButton))
      _endHandleDrag(true);
    // Until the pointer goes farther than the drag distance, it is a click:
    // nothing moves (the first move then starts from the press point).
    else if (_handleMoved
             || (event->pos() - _mousePressedPosition).manhattanLength() >= QApplication::startDragDistance())
    {
      _handleDrag->moveTo(scenePos, event->modifiers() & Qt::ControlModifier);
      _handleMoved = true;
      _updateHandleDrag(event->modifiers() & Qt::ShiftModifier);
    }
  }

  // Vertex grab.
  else if (_vertexGrabbed)
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

      undoStack->push(new MoveVertexCommand(this,
                                            TransformShapeCommand::FREE, _activeVertex, p,
                                            _grabbedShapeCopy));
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
        _lastMousePosition = _mousePressedPosition;
        _shapeFirstGrab = false;
      }

      _shapeMoved = true; // The active vertex is actually moved
    }
    QPointF diff = scenePos - mapToScene(_lastMousePosition);
    undoStack->push(new TranslateShapeCommand(this, TransformShapeCommand::FREE, diff));
  }

  // Window translation action
  else if ((event->buttons() & Qt::MiddleButton) ||
           ((event->modifiers() & Qt::ShiftModifier) && (event->buttons() & Qt::LeftButton)))
  {
    _scrollBy(_lastMousePosition - event->pos());
  }

  // Reset last mouse position.
  _lastMousePosition = event->pos();

  if (isOutput())
    _mainWindow->updateMouseCoordinates();
}

bool MapperGLCanvas::_isOnHandle(const QPoint& viewPosition)
{
  MShape::ptr shape = getCurrentShape();
  if (!shape || shape->isLocked() || !MainWindow::window()->displayControls())
    return false;

  const QPointF handleInView = mapFromScene(handlePosition(*shape, MM::HANDLE_DISTANCE / getZoomFactor()));
  return distSq(QPointF(viewPosition), handleInView) < sq(MM::VERTEX_SELECT_RADIUS);
}

bool MapperGLCanvas::_handleShapeIsCurrent()
{
  MShape::ptr shape = _handleShape.toStrongRef();
  return shape && shape == getCurrentShape() && !shape->isLocked();
}

void MapperGLCanvas::_updateHandleDrag(bool stepped)
{
  if (!_handleShapeIsCurrent())
  {
    _endHandleDrag(false);
    return;
  }
  _handleDrag->setStepped(stepped);
  undoStack->push(new ScaleRotateShapeCommand(this, TransformShapeCommand::FREE, _pressMode, _grabbedShapeCopy,
                                              _handleDrag->transform()));
}

void MapperGLCanvas::_endHandleDrag(bool commit)
{
  // Commit what is shown: the steps of the last update, not the keys held now.
  if (commit && _handleMoved && _handleShapeIsCurrent())
    undoStack->push(new ScaleRotateShapeCommand(this, TransformShapeCommand::RELEASE, _pressMode, _grabbedShapeCopy,
                                                _handleDrag->transform()));
  _handleDrag.reset();
  _handleShape.clear();
  _mainWindow->handleModeKeys()->setDragActive(false);
  _handleMoved = false;
}

void MapperGLCanvas::leaveEvent(QEvent* event)
{
  QGraphicsView::leaveEvent(event);
  if (isOutput())
    _mainWindow->updateMouseCoordinates();
}

void MapperGLCanvas::_scrollBy(const QPoint& distance)
{
  horizontalScrollBar()->setValue(horizontalScrollBar()->value() + distance.x());
  verticalScrollBar()->setValue(verticalScrollBar()->value() + distance.y());
}


void MapperGLCanvas::focusOutEvent(QFocusEvent* event)
{
  // A drag cannot go on without the focus (a dialog or another window opened).
  if (_handleDrag)
    _endHandleDrag(true);
  QGraphicsView::focusOutEvent(event);
}

void MapperGLCanvas::keyReleaseEvent(QKeyEvent* event)
{
  // Releasing Shift during a handle drag removes the steps at once.
  if (_handleDrag && _handleMoved && event->key() == Qt::Key_Shift)
    _updateHandleDrag(false);
  QGraphicsView::keyReleaseEvent(event);
}

void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
{
  // The other side shows which vertex is selected here.
  const auto updateOtherSide = qScopeGuard([this] { _updateOtherSide(); });

  // Pressing Shift during a handle drag shows the steps at once.
  if (_handleDrag && _handleMoved && event->key() == Qt::Key_Shift)
  {
    // (The modifiers of a modifier key's own event differ between systems.)
    _updateHandleDrag(true);
    return;
  }

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

QList<MapperGLCanvas*> MapperGLCanvas::_otherSideCanvases() const
{
  // The output window shows the same shapes as the output editor: they are on
  // the same side.
  QList<MapperGLCanvas*> others;
  const QList<MapperGLCanvas*> canvases = { _mainWindow->getSourceCanvas(), _mainWindow->getDestinationCanvas(),
                                            _mainWindow->getOutputWindow()->getCanvas() };
  for (MapperGLCanvas* canvas : canvases)
  {
    if (canvas && canvas->isOutput() != isOutput())
      others << canvas;
  }
  return others;
}

void MapperGLCanvas::_deselectVerticesOnOtherSide()
{
  for (MapperGLCanvas* canvas : _otherSideCanvases())
    canvas->deselectVertices();
}

void MapperGLCanvas::_updateOtherSide()
{
  for (MapperGLCanvas* canvas : _otherSideCanvases())
    canvas->viewport()->update();
}

int MapperGLCanvas::_linkedVertex() const
{
  for (MapperGLCanvas* canvas : _otherSideCanvases())
  {
    if (canvas->hasActiveVertex())
      return canvas->getActiveVertexIndex();
  }
  return NO_VERTEX;
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
  // On macOS, Qt reports the Command key as ControlModifier and the physical
  // Control key as MetaModifier: accept both, so Cmd+scroll and Ctrl+scroll
  // both zoom. Elsewhere MetaModifier is the Windows/Super key, which the
  // window manager usually keeps for itself.
  bool control_is_pressed = event->modifiers().testFlag(Qt::ControlModifier)
#ifdef Q_OS_MAC
      || event->modifiers().testFlag(Qt::MetaModifier)
#endif
      ;
  bool shift_is_pressed = event->modifiers().testFlag(Qt::ShiftModifier);

  if (control_is_pressed) { // control is pressed: zoom
    // Trackpads and high-resolution wheels send many small deltas (less than
    // one 120-unit notch), which the integer division above rounds to zero.
    // Accumulate them so that they zoom too.
    _wheelZoomRemainder += event->angleDelta().y();
    deltaLevel = _wheelZoomRemainder / 120;
    _wheelZoomRemainder -= deltaLevel * 120;

    // zoom in or out:
    if (deltaLevel > 0) {
      // Increase zoom level
      increaseZoomLevel(deltaLevel);
    } else if (deltaLevel < 0) {
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
  _setZoom(_zoom * qPow(MM::ZOOM_FACTOR, steps), _zoomAnchor());
}

void MapperGLCanvas::decreaseZoomLevel(int steps)
{
  _setZoom(_zoom / qPow(MM::ZOOM_FACTOR, steps), _zoomAnchor());
}

void MapperGLCanvas::resetZoomLevel()
{
  _setZoom(1.0 / _fitScale(), _zoomAnchor());
}

void MapperGLCanvas::fitShapeToView()
{
  _setZoom(1.0, viewport()->rect().center());
  if (!_contentRect.isEmpty())
    centerOn(_contentRect.center());
}

void MapperGLCanvas::setZoomFromMenu(const QString &text)
{
  // Get text choosen by user and convert it to a factor ("50%" -> 0.5).
  bool ok;
  const qreal percent = QString(text).remove(QChar('%')).trimmed().toDouble(&ok);
  if (ok && percent > 0)
    _setZoom(percent / 100, viewport()->rect().center());
}

void MapperGLCanvas::_snapVertex(QPointF* p)
{
  QSettings settings;
  int vertexStickRadius = settings.value("vertexStickRadius", MM::VERTEX_STICK_RADIUS).toInt();
  MappingManager manager = MainWindow::window()->getMappingManager();
  MShape::ptr currentShape = getCurrentShape();
  for (int i = 0; i < manager.nLayers(); i++)
  {
    MShape::ptr shape = getShapeFromMapping(manager.getLayer(i));
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
