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

MM_BEGIN_NAMESPACE

OutputGLCanvas::OutputGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget* shareWidget, QGraphicsScene* scene)
: MapperGLCanvas(mainWindow, true, parent, shareWidget, scene),
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
    painter->translate(rect.x(), rect.y());
    painter->save();
    _drawTestSignal(painter);
    painter->restore();
    glPopMatrix();
  }
  else
  {
    MapperGLCanvas::drawForeground(painter, rect);

    // Display crosshair cursor.
    if (_displayCrosshair)
    {
      QPointF cursorPosition = mapToScene(mapFromGlobal(cursor().pos()));// - rect.topLeft();//(QCursor::pos());///*this->mapFromGlobal(*/QCursor::pos()/*)*/;
      if (rect.contains(cursorPosition))
      {
        painter->setPen(MM::CONTROL_COLOR);
        painter->drawLine(cursorPosition.x(), rect.y(), cursorPosition.x(), rect.height());
        painter->drawLine(rect.x(), cursorPosition.y(), rect.width(), cursorPosition.y());
      }
    }
  }

}

void OutputGLCanvas::_drawTestSignal(QPainter* painter)
{
  const QRect& geo = geometry();
  painter->setPen(MM::CONTROL_COLOR);
  int rect_size = 10;
  QColor color_0(191, 191, 191);
  QColor color_1(128, 128, 128);
  QBrush brush_0(color_0);
  QBrush brush_1(color_1);

  painter->setPen(Qt::NoPen);

  // Draw checkerboard pattern.
  for (int x = geo.x(); x < geo.width(); x += rect_size)
  {
    for (int y = geo.y(); y < geo.height(); y += rect_size)
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

  // Create responsive image
  QImage test_signal = _svg_test_signal.scaled(geo.height(), geo.height(), Qt::KeepAspectRatio);
  // Set new brush
  this->_brush_test_signal = QBrush(test_signal);
  // Center the brush
  painter->translate((geo.width() - test_signal.width()) / 2, (geo.height() - test_signal.height()) / 2);
  // Draw the actual brush.
  painter->fillRect(0, 0, test_signal.width(), test_signal.height(), this->_brush_test_signal);
}

void OutputGLCanvas::resizeGL(int width, int height)
{
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
    MapperGLCanvas::mouseMoveEvent(event);
  }
}

MM_END_NAMESPACE
