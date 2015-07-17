/*
 * SourceGLCanvas.cpp
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

#include "SourceGLCanvas.h"

#include "MainWindow.h"


SourceGLCanvas::SourceGLCanvas(MainWindow* mainWindow, QWidget* parent)
  : MapperGLCanvas(mainWindow, parent)
{
}

MShape* SourceGLCanvas::getShapeFromMappingId(uid mappingId) const
{
  if (mappingId == NULL_UID)
    return NULL;

  else
  {
    Mapping::ptr mapping = getMainWindow()->getMappingManager().getMappingById(mappingId);
    Q_CHECK_PTR(mapping);
    return mapping->getInputShape().data();
  }
}

ShapeGraphicsItem* SourceGLCanvas::getShapeGraphicsItemFromMappingId(uid mappingId) const
{
  if (mappingId == NULL_UID)
    return NULL;

  else
  {
    return MainWindow::instance()->getMapperByMappingId(mappingId)->getInputGraphicsItem();
  }
}

//

//void SourceGLCanvas::doDraw(QPainter* painter)
//{
//  if (getMainWindow()->hasCurrentMapping())
//  {
//    uint mappingId = getMainWindow()->getCurrentMappingId();
//    const Mapper::ptr& mapper = getMainWindow()->getMapperByMappingId(mappingId);
//    painter->save();
//    mapper->drawInput(painter);
//    painter->restore();
//    if (displayControls())
//    {
//      painter->save();
//      if (hasActiveVertex()) {
//        QList<int> selectedVertices;
//        selectedVertices.append(getActiveVertexIndex());
//        mapper->drawInputControls(painter, &selectedVertices);
//      }
//      else
//        mapper->drawInputControls(painter);
//      painter->restore();
//    }
//  }
//}

