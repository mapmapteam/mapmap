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
: MapperGLCanvas(mainWindow, parent, shareWidget, scene)
{
}

MShape* DestinationGLCanvas::getShapeFromMappingId(uid mappingId) const
{
  if (mappingId == NULL_UID)
    return NULL;
  else
    return getMainWindow()->getMappingManager().getMappingById(mappingId)->getShape().data();
}

ShapeGraphicsItem* DestinationGLCanvas::getShapeGraphicsItemFromMappingId(uid mappingId) const
{
  if (mappingId == NULL_UID)
    return NULL;

  else
  {
    return MainWindow::instance()->getMapperByMappingId(mappingId)->getGraphicsItem();
  }
}
