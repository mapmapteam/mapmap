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

MShape::ptr DestinationGLCanvas::getShapeFromMapping(const Mapping::ptr& mapping) const
{
  return (mapping.isNull() ? MShape::ptr() : mapping->getShape());
}

QSharedPointer<ShapeGraphicsItem> DestinationGLCanvas::getShapeGraphicsItemFromMapping(const Mapping::ptr& mapping) const
{
  return (mapping.isNull() ? QSharedPointer<ShapeGraphicsItem>() : MainWindow::instance()->getMappingGuiByMappingId(mapping->getId())->getGraphicsItem());
}
