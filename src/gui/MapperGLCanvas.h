/*
 * MapperGLCanvas.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Dame Diongue -- bamdamd(@)gmail(.)com
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

#include <QtCore/QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QGLWidget>
#else
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtOpenGL/QOpenGLFunctions>
#endif

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QUndoStack>
#include <QtMath>
#include <iostream>

#include "MM.h"
#include "UidAllocator.h"
#include "Shape.h"
#include "MappingGui.h"

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  MapperGLCanvas(MainWindow* mainWindow, bool isOutput, QWidget* parent = nullptr, const QGLWidget* shareWidget = nullptr, QGraphicsScene* scene = nullptr);
#else
  MapperGLCanvas(MainWindow* mainWindow, bool isOutput, QWidget* parent = nullptr, const QOpenGLWidget* shareWidget = nullptr, QGraphicsScene* scene = nullptr);
#endif
  virtual ~MapperGLCanvas() {}

  /// Returns shape associated with mapping id.
  virtual bool isOutput() const { return _isOutput; }

  MShape::ptr getShapeFromMapping(Mapping::ptr mapping);
  MShape::ptr getCurrentShape();
  QSharedPointer<ShapeGraphicsItem> getShapeGraphicsItemFromMapping(Mapping::ptr mapping);
  QSharedPointer<ShapeGraphicsItem> getCurrentShapeGraphicsItem();

  // Draws foreground (displays crosshair if needed).
  void drawForeground(QPainter *painter , const QRectF &rect);

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

  qreal getZoomFactor() const { return _shapeIsAdapted
        ? _scalingFactor
        : qBound(MM::ZOOM_MIN, qPow(MM::ZOOM_FACTOR, _zoomLevel), MM::ZOOM_MAX); }

  /// This function needs to be called after a shape inside the canvas has been changed for appropriate signals to be activated.
  void currentShapeWasChanged();

  // Apply zoom to view
  void applyZoomToView();

protected:
  void dragEnterEvent(QDragEnterEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);

private:
  // Pointer to main window.
  MainWindow* _mainWindow;

  // Is this a destination (output) or source (input) canvas.
  bool _isOutput;

  // Last point pressed (in mouse/window coordinates).
  QPoint _mousePressedPosition;

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

  // The zoom level (in number of steps).
  int _zoomLevel;

  // The scaling factor
  qreal _scalingFactor;

  bool _shapeIsAdapted;

  // Pointer to MainWindow UndoStack
  QUndoStack *undoStack;

signals:
  void shapeChanged(MShape*);
  void imageChanged();
  void shapeContextMenuRequested(const QPoint &pos);
  void zoomFactorChanged(qreal value);

public slots:
  void updateCanvas();
  void deselectVertices();
  void deselectAll();

  void wheelEvent(QWheelEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent* event);

  // Event Filter
  bool eventFilter(QObject *target, QEvent *event);

  // Zoom
  void increaseZoomLevel(int steps=1);
  void decreaseZoomLevel(int steps=1);
  void resetZoomLevel();
  void fitShapeToView();
  // Set zoom factor with drowmenu data
  void setZoomFromMenu(const QString& text);

protected:
  // TODO: Perhaps the sticky-sensitivity should be configurable through GUI
  void _snapVertex(QPointF* p);

public:
  static const int NO_VERTEX = -1;
};

}

#endif /* MAPPERGLCANVAS_H_ */
