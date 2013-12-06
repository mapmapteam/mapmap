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

//#include "Common.h"
#include "Shape.h"

//#include "MainWindow.h"

/**
 * Qt GUI widget that draws a mapper, which is a shape with some paint.
 */
class MapperGLCanvas: public QGLWidget
{
  Q_OBJECT

public:
  MapperGLCanvas(QWidget* parent = 0, const QGLWidget* shareWidget = 0);
  virtual ~MapperGLCanvas() {}

  virtual Shape* getShapeFromMappingId(int mappingId) = 0;

  void switchImage(int imageId);
//  QSize sizeHint() const;
//  QSize minimumSizeHint() const;
  Shape* getCurrentShape();
  void glueVertex(Shape *, Point *);

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
  void draw();
  void enterDraw();
  virtual void doDraw() = 0;
  void exitDraw();
  bool _mousepressed;
  int _active_vertex;
  bool _shapegrabbed;
  bool _shapefirstgrab;

signals:
  void quadChanged();
  void imageChanged();

public slots:
  void updateCanvas();
};

#endif /* MAPPERGLCANVAS_H_ */
