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
#include <QKeyEvent>
#include <QPaintEvent>

#include <iostream>

#include "UidAllocator.h"
#include "Shape.h"

class MainWindow;

/**
 * Qt GUI widget that draws a mapper, which is a shape with some paint.
 */
class MapperGLCanvas: public QGLWidget
{
  Q_OBJECT

public:
  MapperGLCanvas(MainWindow* mainWindow, QWidget* parent = 0, const QGLWidget* shareWidget = 0);
  virtual ~MapperGLCanvas() {}

  virtual Shape* getShapeFromMappingId(uid mappingId) = 0;

  void switchImage(int imageId);
//  QSize sizeHint() const;
//  QSize minimumSizeHint() const;
  Shape* getCurrentShape();
  void glueVertex(Shape *, QPointF *);

  MainWindow* getMainWindow() const { return _mainWindow; }
  bool displayControls() const { return _displayControls; }

protected:
  void initializeGL();
  void resizeGL(int width, int height);
  void paintGL();

  void keyPressEvent(QKeyEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void paintEvent(QPaintEvent* event);

private:
  void draw(QPainter* painter);
  void enterDraw(QPainter* painter);
  virtual void doDraw(QPainter* painter) = 0;
  void exitDraw(QPainter* painter);

  MainWindow* _mainWindow;
  bool _mousepressed;
  int _activeVertex;
  bool _shapegrabbed;
  bool _shapefirstgrab;
  bool _displayControls;

signals:
  void shapeChanged(Shape*);
  void imageChanged();

public slots:
  void updateCanvas();
  void enableDisplayControls(bool display);
  void deselectAll();

public:
  static const int NO_VERTEX = -1;
};

#endif /* MAPPERGLCANVAS_H_ */
