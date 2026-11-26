/*
 * MapperGLCanvas.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef MAPPERGLCANVAS_H_
#define MAPPERGLCANVAS_H_

#include <QOpenGLWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QUndoStack>

#include <QtMath>

#include <iostream>
#include <optional>

#include "MM.h"
#include "FrameStats.h"
#include "UidAllocator.h"
#include "Shape.h"
#include "ShapeHandles.h"

#include "LayerGui.h"

namespace mmp {

class MainWindow;
class ShapeGraphicsItem;

/**
 * Mother class for OpenGL canvases that allow the display and controls of shapes and vertices.
 * Provides common functionality to both main sublasses: SourceGLCanvas and DestinationGLCanvas.
 */
class MapperGLCanvas: public QGraphicsView
{
  Q_OBJECT
public:
  /// Constructor.
  MapperGLCanvas(MainWindow* mainWindow, bool isOutput, QWidget* parent = 0, QOpenGLWidget* shareWidget = 0, QGraphicsScene* scene = 0);
  virtual ~MapperGLCanvas() {}

  /// Returns shape associated with mapping id.
  virtual bool isOutput() const { return _isOutput; }

  MShape::ptr getShapeFromMapping(Layer::ptr mapping);
  MShape::ptr getCurrentShape();
  QSharedPointer<ShapeGraphicsItem> getShapeGraphicsItemFromMapping(Layer::ptr mapping);
  QSharedPointer<ShapeGraphicsItem> getCurrentShapeGraphicsItem();

//  QSize sizeHint() const;
//  QSize minimumSizeHint() const;

  // Draws foreground (displays crosshair if needed).
  void drawForeground(QPainter *painter , const QRectF &rect) override;

  /**
   * Stick vertex p of Shape orig to another Shape's vertex, if the 2 vertices are
   * close enough. The distance per coordinate is currently set in dist_stick
   * variable.
   */
  /// Returns pointer to main window.
  MainWindow* getMainWindow() const { return _mainWindow; }

  /// Returns true iff one of the vertices is currently active.
  bool hasActiveVertex() const { return _activeVertex != NO_VERTEX; }

  /// Returns the currently active (ie. selected) vertex, or NO_VERTEX if none is currently active.
  int getActiveVertexIndex() const { return _activeVertex; }

  /// Set the currently active vertex
  void setActiveVertexIndex(int activeVertex) { _activeVertex = activeVertex; }

  bool shapeGrabbed() const { return _shapeGrabbed; }
  bool vertexGrabbed() const { return _vertexGrabbed; }

  /**
   * Returns the scale from scene coordinates to screen pixels, used to draw
   * the controls at a constant size on screen.
   */
  virtual qreal getZoomFactor() const { return _fitScale() * _zoom; }

  /**
   * Returns the zoom shown to the user: 1 when the content rectangle (see
   * setContentRect()) fills the view.
   */
  qreal getRelativeZoom() const { return _zoom; }

  /**
   * Sets the rectangle that this canvas frames at 100% zoom: the image of the
   * current source in the input editor, the output frame in the output editor.
   * The view is centered on it when it changes.
   */
  void setContentRect(const QRectF& rect);

  /// Returns the rectangle framed at 100% zoom.
  QRectF getContentRect() const { return _contentRect; }

  /// Sets the name shown above the outline of the content rectangle.
  void setContentLabel(const QString& label);

  // Apply zoom to view
  void applyZoomToView();

protected:
//  void initializeGL();
//  void resizeGL(int width, int height);
//  void paintGL();
//
//  void keyPressEvent(QKeyEvent* event);
//  void mousePressEvent(QMouseEvent* event);
//  void mouseMoveEvent(QMouseEvent* event);
//  void mouseReleaseEvent(QMouseEvent* event);
//  void paintEvent(QPaintEvent* event);
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

  void resizeEvent(QResizeEvent* event) override;
  void leaveEvent(QEvent* event) override;

  /// Draws the outline of the content rectangle under everything else.
  void drawBackground(QPainter* painter, const QRectF& rect) override;

protected:
//  /**
//   * Draws the shapes and controls over the canvas. This method calls:
//   * <code>
//   * enterDraw(painter);
//   * doDraw(painter);
//   * exitDraw(painter);
//   * </code>
//   */
//  void draw(QPainter* painter);
//
//  /// Performs initalizations before drawing.
//  void enterDraw(QPainter* painter);
//
//  /// Performs the drawing (implemented by subclasses).
//  virtual void doDraw(QPainter* painter) = 0;
//
//  /// Performs last drawing actions before exiting draw(QPainter*).
//  void exitDraw(QPainter* painter);

private:
  // Pointer to main window.
  MainWindow* _mainWindow;

  // Is this a destination (output) or source (input) canvas.
  bool _isOutput;

  // Last point pressed (in mouse/window coordinates).
  QPoint _mousePressedPosition;

  // Last position of the mouse (in mouse/window coordinates).
  QPoint _lastMousePosition;

  // Start position of last object grabbed (in scene coordinates).
  QPointF _grabbedObjectStartScenePosition;

  // Center point of currently selected shape (in scene coordinates).
  QPointF _grabbedShapeStartCenterScenePosition;
  MShape::ptr _grabbedShapeCopy;

  // Mouse currently pressed inside a vertex.
  bool _vertexGrabbed;
  
  // If current vertex has been moved.
  bool _vertexMoved;

  // Index of currently active vertex.
  int _activeVertex;

  // True iff current shape is grabbed.
  bool _shapeGrabbed;
  // If current shape has been moved.
  bool _shapeMoved;

  // True iff current shape is grabbed (first step).
  bool _shapeFirstGrab;

  // Zoom relative to the content rectangle filling the view (1 = 100%).
  qreal _zoom;

  // Rectangle framed at 100% zoom (see setContentRect()).
  QRectF _contentRect;

  // Name shown above the outline of the content rectangle.
  QString _contentLabel;

  // Drag in progress on a scale or rotation handle of the current shape.
  std::optional<HandleDrag> _handleDrag;
  // The mode when the handle drag started, which names the undo step.
  HandleMode _pressMode;
  // The shape whose handle is dragged.
  QWeakPointer<MShape> _handleShape;
  // If the current handle drag has moved the shape.
  bool _handleMoved;

  // Wheel movement not yet turned into a zoom step (in 1/8 of a degree,
  // 120 per notch); see wheelEvent().
  int _wheelZoomRemainder;


  // Pointer to MainWindow UndoStack
  QUndoStack *undoStack;

signals:
  void imageChanged();
  void shapeContextMenuRequested(const QPoint &pos);
  void zoomFactorChanged(qreal value);

public slots:
  void updateCanvas();
  void deselectVertices();
  void deselectAll();

  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;

  // Event Filter
  bool eventFilter(QObject *target, QEvent *event) override;

  // Zoom
  void increaseZoomLevel(int steps=1);
  void decreaseZoomLevel(int steps=1);
  /// Shows one pixel of the content per screen pixel.
  void resetZoomLevel();
  /// Makes the content rectangle fill the view (100%).
  void fitShapeToView();
  // Set zoom factor with drowmenu data
  void setZoomFromMenu(const QString& text);

protected:
  /// Paints the view, timing it when FrameStats is on.
  void paintEvent(QPaintEvent* event) override;

  /// Timing of the paintings of this view (see FrameStats).
  FrameStats _frameStats;
  /// Start of the previous painting (FrameStats::now()), 0 if none.
  qint64 _lastPaintStartNs;

  // TODO: Perhaps the sticky-sensitivity should be configurable through GUI
  void _snapVertex(QPointF* p);

  // Draws the outline of the content rectangle.
  void _drawContentOutline(QPainter* painter);

  // Makes the scene rectangle cover the content rectangle and all shapes,
  // so that the scroll bars reach all of them, at the given view scale.
  virtual void _updateSceneRect(qreal scale);

  // Content outline and its label, in screen pixels.
  static constexpr qreal CONTENT_OUTLINE_WIDTH  = 2;
  static constexpr qreal CONTENT_OUTLINE_GAP    = 3;
  static constexpr qreal CONTENT_OUTLINE_RADIUS = 8;
  static constexpr int   CONTENT_LABEL_FONT_SIZE = 11;
  static constexpr qreal CONTENT_LABEL_SPACING  = 3;

  // Space left at 100% zoom between the view and the handle of a shape that
  // covers all the content, in screen pixels.
  static constexpr qreal FIT_MARGIN_SPACING = 4;

private:
  // Scale at which the content rectangle fills the view (1 without content).
  qreal _fitScale() const;
  // The same for a view of the given size, in screen pixels.
  qreal _fitScale(const QSizeF& viewSize) const;
  // Shows the scroll bars when they have something to scroll, or hides them.
  void _setScrollBarsShown(bool shown);

  // Sets the zoom, bounded, and applies it keeping the scene point under
  // viewAnchor (in viewport coordinates) in place.
  void _setZoom(qreal zoom, const QPointF& viewAnchor);

  // Returns the viewport position of the mouse if it is over the view, or
  // the center of the view.
  QPointF _zoomAnchor() const;

  // Scrolls the view (in pixels).
  void _scrollBy(const QPoint& distance);

  /// Returns true iff a point of the view is on the handle of the current shape
  /// (never when the shape is locked or controls are hidden).
  bool _isOnHandle(const QPoint& viewPosition);

  /// Returns true iff the shape whose handle is dragged is still the current,
  /// unlocked shape (a shortcut, OSC or MCP may change it during the drag).
  bool _handleShapeIsCurrent();

  /// Applies the handle drag to its shape, with steps when stepped is true.
  void _updateHandleDrag(bool stepped);

  /// Ends the handle drag; when commit is true and the shape moved, makes the
  /// drag a single undo step.
  void _endHandleDrag(bool commit);
  // The editors of the other side: the output ones for the input editor, and
  // the other way round.
  QList<MapperGLCanvas*> _otherSideCanvases() const;
  // Deselects the vertex selected on the other side, when one gets selected here.
  void _deselectVerticesOnOtherSide();
  // Redraws the other side, which shows the vertex selected here.
  void _updateOtherSide();

public:
  static const int NO_VERTEX = -1;
};

}

#endif /* MAPPERGLCANVAS_H_ */
