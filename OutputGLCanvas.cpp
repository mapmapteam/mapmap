/*
 * OutputGLCanvas.cpp
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

#include "OutputGLCanvas.h"
#include "MainWindow.h"

OutputGLCanvas::OutputGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget* shareWidget, QGraphicsScene* scene)
: DestinationGLCanvas(mainWindow, parent, shareWidget, scene),
  _displayCrosshair(false),
  _svg_test_signal(":/test-signal"),
  _brush_test_signal(_svg_test_signal)
{
  // Disable scrollbars.
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void OutputGLCanvas::drawForeground(QPainter *painter , const QRectF &rect)
{
  if (getMainWindow()->displayTestSignal())
  {
    glPushMatrix();
    painter->save();
    _drawTestSignal(painter);
    painter->restore();
    glPopMatrix();
    return;
  }

  // Display crosshair cursor.
  if (_displayCrosshair)
  {
    QPointF cursorPosition = mapToScene(cursor().pos());// - rect.topLeft();//(QCursor::pos());///*this->mapFromGlobal(*/QCursor::pos()/*)*/;
    if (rect.contains(cursorPosition))
    {
      painter->setPen(MM::CONTROL_COLOR);
      painter->drawLine(cursorPosition.x(), rect.y(), cursorPosition.x(), rect.height());
      painter->drawLine(rect.x(), cursorPosition.y(), rect.width(), cursorPosition.y());
    }
  }
}

void OutputGLCanvas::_drawTestSignal(QPainter* painter)
{
  const QRect& geo = geometry();
  painter->setPen(MM::CONTROL_COLOR);
  int height = geo.height();
  int width = geo.width();
  int rect_size = 10;
  QColor color_0(191, 191, 191);
  QColor color_1(128, 128, 128);
  QBrush brush_0(color_0);
  QBrush brush_1(color_1);

  painter->setPen(Qt::NoPen);

  for (int x = 0; x < width; x += rect_size)
  {
    for (int y = 0; y < height; y += rect_size)
    {
      if (((x + y) % 20) == 0)
      {
        painter->setBrush(brush_0);
      } else {
        painter->setBrush(brush_1);
      }
      painter->drawRect(x, y, rect_size, rect_size);
    }
  }

  painter->fillRect(geo, this->_brush_test_signal);
}

void OutputGLCanvas::resizeGL(int width, int height)
{
  int side_length = width;
  if (height < width)
  {
    side_length = height;
  }

  (void) side_length; // to get rid of warnings
  // TODO: reload SVG with the new size
  // TODO: _svg_test_signal.load(":/test-signal");
  // TODO: _brush_test_signal(_svg_test_signal)
}

void OutputGLCanvas::wheelEvent(QWheelEvent *event)
{
  event->ignore();
}

void OutputGLCanvas::mouseMoveEvent(QMouseEvent *event)
{
  // Click-and-drag translate view.
  if (event->buttons() & Qt::MiddleButton)
  {
    event->ignore();
  }
  else
  {
    DestinationGLCanvas::mouseMoveEvent(event);
  }
}
