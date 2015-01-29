/*
 * DestinationGLCanvas.cpp
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

#include "DestinationGLCanvas.h"
#include "MainWindow.h"
#include <QtGui/QOpenGLFunctions>
#include <GLES/gl.h>

DestinationGLCanvas::DestinationGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget * shareWidget)
: MapperGLCanvas(mainWindow, parent, shareWidget),
  _displayCrosshair(false),
  _svg_test_signal(":/test-signal"),
  _brush_test_signal(_svg_test_signal)
{
}

Shape* DestinationGLCanvas::getShapeFromMappingId(uid mappingId)
{
  if (mappingId == NULL_UID)
    return NULL;
  else
    return getMainWindow()->getMappingManager().getMappingById(mappingId)->getShape().get();
}

void DestinationGLCanvas::doDraw(QPainter* painter)
{
  if (this->displayTestSignal())
  {
    glPushMatrix();
    painter->save();
    this->_drawTestSignal(painter);
    painter->restore();
    glPopMatrix();
    return;
  }

  glPushMatrix();

  // Draw the mappings.
  QVector<Mapping::ptr> mappings = getMainWindow()->getMappingManager().getVisibleMappings();
  for (QVector<Mapping::ptr>::const_iterator it = mappings.begin(); it != mappings.end(); ++it)
  {
    painter->save();
    getMainWindow()->getMapperByMappingId((*it)->getId())->draw(painter);
    painter->restore();
  }

  // Draw the controls of current mapping.
  if (displayControls() &&
      getMainWindow()->hasCurrentMapping() &&
      getCurrentShape() != NULL)
  {
    painter->save();
    const Mapper::ptr& mapper = getMainWindow()->getMapperByMappingId(getMainWindow()->getCurrentMappingId());
    if (hasActiveVertex()) {
      QList<int> selectedVertices;
      selectedVertices.append(getActiveVertexIndex());
      mapper->drawControls(painter, &selectedVertices);
    }
    else
    {
      mapper->drawControls(painter);
    }
    painter->restore();
  }

  glPopMatrix();

  // Display crosshair cursor.
  if (_displayCrosshair)
  {
    const QPoint& cursorPosition = this->mapFromGlobal(QCursor::pos());
    const QRect& geo = geometry();
    if (geo.contains(cursorPosition))
    {
      painter->setPen(MM::CONTROL_COLOR);
      painter->drawLine(cursorPosition.x(), 0, cursorPosition.x(), geo.height());
      painter->drawLine(0, cursorPosition.y(), geo.width(), cursorPosition.y());
    }
  }

}

void DestinationGLCanvas::_drawTestSignal(QPainter* painter)
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

void DestinationGLCanvas::resizeGL(int width, int height)
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

bool DestinationGLCanvas::eventFilter(QObject *target, QEvent *event)
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

