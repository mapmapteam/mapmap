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

DestinationGLCanvas::DestinationGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget* shareWidget, QGraphicsScene* scene)
: MapperGLCanvas(mainWindow, parent, shareWidget, scene),
  _displayCrosshair(false)
{
}

MShape* DestinationGLCanvas::getShapeFromMappingId(uid mappingId)
{
  if (mappingId == NULL_UID)
    return NULL;
  else
    return getMainWindow()->getMappingManager().getMappingById(mappingId)->getShape().get();
}

void DestinationGLCanvas::drawForeground(QPainter *painter , const QRectF &rect) {
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

//void DestinationGLCanvas::doDraw(QPainter* painter)
//{
//  glPushMatrix();
//
//  // Draw the mappings.
//  QVector<Mapping::ptr> mappings = getMainWindow()->getMappingManager().getVisibleMappings();
//  for (QVector<Mapping::ptr>::const_iterator it = mappings.begin(); it != mappings.end(); ++it)
//  {
//    painter->save();
//    getMainWindow()->getMapperByMappingId((*it)->getId())->draw(painter);
//    painter->restore();
//  }
//
//  // Draw the controls of current mapping.
//  if (displayControls() &&
//      getMainWindow()->hasCurrentMapping() &&
//      getCurrentShape() != NULL)
//  {
//    painter->save();
//    const Mapper::ptr& mapper = getMainWindow()->getMapperByMappingId(getMainWindow()->getCurrentMappingId());
//    if (hasActiveVertex()) {
//      QList<int> selectedVertices;
//      selectedVertices.append(getActiveVertexIndex());
//      mapper->drawControls(painter, &selectedVertices);
//    }
//    else
//      mapper->drawControls(painter);
//    painter->restore();
//  }
//
//  glPopMatrix();
//
//  // Display crosshair cursor.
//  if (_displayCrosshair)
//  {
//    const QPoint& cursorPosition = this->mapFromGlobal(QCursor::pos());
//    const QRect& geo = geometry();
//    if (geo.contains(cursorPosition))
//    {
//      painter->setPen(MM::CONTROL_COLOR);
//      painter->drawLine(cursorPosition.x(), 0, cursorPosition.x(), geo.height());
//      painter->drawLine(0, cursorPosition.y(), geo.width(), cursorPosition.y());
//    }
//  }
//
//}

