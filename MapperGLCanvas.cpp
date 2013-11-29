/*
 * MapperGLCanvas.cpp
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

#include "MapperGLCanvas.h"

MapperGLCanvas::MapperGLCanvas(QWidget* parent, const QGLWidget * shareWidget)
  : QGLWidget(parent, shareWidget), _mousepressed(false)
{
}

void MapperGLCanvas::initializeGL()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  //qglClearColor(Qt::black);
  //glShadeModel(GL_FLAT);
  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);
}

void MapperGLCanvas::resizeGL(int /* width */, int /* height */)
{
//  glClearColor(0.0, 0.0, 0.0, 0.0);
//
//  glViewport(0, 0, width, height);
//  glMatrixMode(GL_PROJECTION);
//  glLoadIdentity();
//  glMatrixMode (GL_PROJECTION);
//  glLoadIdentity ();
//  glOrtho (
//    0.0f, (GLfloat) width, // left, right
//    (GLfloat) height, 0.0f, // bottom, top
//    -1.0, 1.0f);
//  glMatrixMode (GL_MODELVIEW);
}

void MapperGLCanvas::paintGL()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw();
}

void MapperGLCanvas::draw()
{
  enterDraw();
  doDraw();
  exitDraw();
}

void MapperGLCanvas::enterDraw()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width(), height());
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (
    0.0f, (GLfloat) width(), // left, right
    (GLfloat) height(), 0.0f, // bottom, top
    -1.0, 1.0f);
  glMatrixMode (GL_MODELVIEW);

//  glClearColor(0.0, 0.0, 0.0, 0.0);
//  glClear(GL_COLOR_BUFFER_BIT);
//  glViewport(0, 0, (GLint) GetSize().x, (GLint) GetSize().y);

//  glMatrixMode (GL_PROJECTION);
//  glLoadIdentity ();
//  glOrtho (
//    0.0f, (float) GetSize().x, // left, right
//    (float) GetSize().y, 0.0f, // bottom, top
//    -1.0, 1.0f);
//  glMatrixMode (GL_MODELVIEW);

////  glLoadIdentity (); // FIXME? is this needed here?
}

void MapperGLCanvas::mousePressEvent(QMouseEvent* event)
{
  int i, dist, maxdist, mindist;
  int xmouse = event->x();
  int ymouse = event->y();
  maxdist = mindist = 50;
  if (event->buttons() & Qt::LeftButton)
  {    
    // std::cout << "Left Mouse key pressed" << std::endl;
    for (i = 0; i < 4; i++)
    {
      Quad& quad = getQuad();
      Point p = quad.getVertex(i);
      dist = abs(xmouse - p.x) + abs(ymouse - p.y); 
      if (dist < maxdist && dist < mindist)
      {
        quad.setActiveVertex(i);
        mindist = dist;
      }
    }
    _mousepressed = true;
  }
}

void MapperGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
  // std::cout << "Mouse Release event " << std::endl;
  _mousepressed = false;
}

void MapperGLCanvas::mouseMoveEvent(QMouseEvent* event)
{

  if (_mousepressed)
  {
    // std::cout << "Move event " << std::endl;
    Quad& quad = getQuad();
    Point p = quad.getVertex(quad.getActiveVertex());
    p.x = event->x();
    p.y = event->y();

    quad.setVertex(quad.getActiveVertex(), p);
    update();
    emit quadChanged();
  }
}

void MapperGLCanvas::keyPressEvent(QKeyEvent* event)
{
  std::cout << "Key pressed" << std::endl;
  int xMove = 0;
  int yMove = 0;
  switch (event->key()) {
  case Qt::Key_Tab:
    if (event->modifiers() & Qt::ControlModifier)
      switchImage( (Common::getCurrentSourceId() + 1) % Common::nImages());
    else
    {
      Quad& quad = getQuad();
      quad.setActiveVertex((quad.getActiveVertex() + 1) % 4);
    }
    break;
  case Qt::Key_Up:
    yMove = -1;
    break;
  case Qt::Key_Down:
    yMove = +1;
    break;
  case Qt::Key_Left:
    xMove = -1;
    break;
  case Qt::Key_Right:
    xMove = +1;
    break;
  default:
    std::cerr << "Unhandled key" << std::endl;
    QWidget::keyPressEvent(event);
    break;
  }

  Quad& quad = getQuad();
  Point p = quad.getVertex(quad.getActiveVertex());
  p.x += xMove;
  p.y += yMove;
  quad.setVertex(quad.getActiveVertex(), p);

  update();

  emit quadChanged();
}

void MapperGLCanvas::paintEvent(QPaintEvent* /* event */)
{
  std::cout << "Paint event" << std::endl;
  updateGL();
}

void MapperGLCanvas::switchImage(int imageId)
{
  Common::switchImage(imageId);
  emit quadSwitched();
}

void MapperGLCanvas::exitDraw()
{
  glFlush();
  swapBuffers();
}

void MapperGLCanvas::updateCanvas()
{
  std::cout << "Update me!" << std::endl;
  update();
}
