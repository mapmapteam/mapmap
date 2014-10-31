/*
 * MapperGLCanvas.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QPaintEvent>

#include <iostream>

#include "UidAllocator.h"
#include "Shape.h"

class MainWindow;

/**
 * Mother class for OpenGL canvases that allow the display and controls of shapes and vertices.
 * Provides common functionality to both main sublasses: SourceGLCanvas and DestinationGLCanvas.
 */
class MapperGLCanvas: public QGraphicsView
{
  Q_OBJECT

public:
  /// Constructor.
  MapperGLCanvas(MainWindow* mainWindow, QWidget* parent = 0, const QGLWidget* shareWidget = 0);
  virtual ~MapperGLCanvas() {}

  /// Returns shape associated with mapping id.
  virtual MShape* getShapeFromMappingId(uid mappingId) = 0;

//  QSize sizeHint() const;
//  QSize minimumSizeHint() const;

  /// Returns current shape.
//  Shape* getCurrentShape();

  /**
   * Stick vertex p of Shape orig to another Shape's vertex, if the 2 vertices are
   * close enough. The distance per coordinate is currently set in dist_stick
   * variable.
   */
  // TODO: Perhaps the sticky-sensitivity should be configurable through GUI
  void glueVertex(MShape *, QPointF *);

  /// Returns pointer to main window.
  MainWindow* getMainWindow() const { return _mainWindow; }

//  QGLWidget* getViewport() const { return _viewport; }

  /// Returns true iff we should display the controls.
  bool displayControls() const { return _displayControls; }

  /// Returns true iff we want vertices to stick to each other.
  bool stickyVertices() const { return _stickyVertices; }

  /// Returns true iff one of the vertices is currently active.
  bool hasActiveVertex() const { return _activeVertex != NO_VERTEX; }

  /// Returns the currently active (ie. selected) vertex, or NO_VERTEX if none is currently active.
  int getActiveVertexIndex() const { return _activeVertex; }

protected:
//  void initializeGL();
//  void resizeGL(int width, int height);
//  void paintGL();

//  void keyPressEvent(QKeyEvent* event);
//  void mousePressEvent(QMouseEvent* event);
//  void mouseMoveEvent(QMouseEvent* event);
//  void mouseReleaseEvent(QMouseEvent* event);
//  void paintEvent(QPaintEvent* event);

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

//  QGraphicsScene* _scene;

//  QGLWidget* _viewport;

  // Last point pressed.
  QPoint _mousePressedPosition;

  // Mouse currently pressed inside a vertex.
  bool _mousePressedOnVertex;

  // Index of currently active vertex.
  int _activeVertex;

  // True iff current shape is grabbed.
  bool _shapeGrabbed;

  // True iff current shape is grabbed (first step).
  bool _shapeFirstGrab;

  // True iff we are displaying the controls.
  bool _displayControls;

  // True iff we want vertices to stick to each other.
  bool _stickyVertices;

signals:
  void shapeChanged(MShape*);
  void imageChanged();

public slots:
  void updateCanvas();
  void enableDisplayControls(bool display);
  void enableStickyVertices(bool display);
  void deselectVertices();
  void deselectAll();

public:
  static const int NO_VERTEX = -1;
};

#endif /* MAPPERGLCANVAS_H_ */
