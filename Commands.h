/*
 * Commands.h
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2014 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <QUndoCommand>
#include "MainWindow.h"
#include "MapperGLCanvas.h"

class AddShapesCommand : public QUndoCommand
{
public:
  AddShapesCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent = 0);
  void undo();
  void redo();

private:
  MainWindow *m_mainWindow;
  Mapping::ptr m_mappingPtr;
  uid m_mappingId;

};

class MoveVertexCommand : public QUndoCommand
{
public:
  MoveVertexCommand(MapperGLCanvas *mapperGLCanvas, Shape *shape, int activeVertex, const QPointF &point, QUndoCommand *parent = 0);
  void undo();
  void redo();

private:
  MapperGLCanvas *m_mapperGLCanvas;
  Shape *m_shape;
  int m_activeVertex;
  QPointF newPosition, oldPosition;

};

class MoveShapesCommand : public QUndoCommand
{

};

class DeleteMappingCommand : public QUndoCommand
{
public:
  DeleteMappingCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent = 0);
  void undo();
  void redo();

private:
  MainWindow *m_mainWindow;
  Mapping::ptr m_mappingPtr;
  uid m_mappingId;
};

#endif /* COMMANDS_H_ */
