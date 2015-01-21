
#include "Commands.h"

AddShapesCommand::AddShapesCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent):
  QUndoCommand(parent)
{
  m_mainWindow = mainWindow;
  m_mappingId = mappingId;
}

void AddShapesCommand::undo()
{
  m_mappingPtr = m_mainWindow->getMappingManager().getMappingById(m_mappingId);
  m_mainWindow->deleteMapping(m_mappingId);
}

void AddShapesCommand::redo()
{
  if(m_mappingPtr != NULL)
    {
      uint currentId = m_mainWindow->getMappingManager().addMapping(m_mappingPtr);
      m_mainWindow->addMappingItem(currentId);
    }
}


MoveVertexCommand::MoveVertexCommand(MapperGLCanvas *mapperGLCanvas, Shape *shape, int activeVertex, const QPointF &point, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  m_mapperGLCanvas = mapperGLCanvas;
  m_shape = shape;
  m_activeVertex = activeVertex;
  newPosition = point;

  oldPosition = m_shape->getVertex(m_activeVertex);
}

void MoveVertexCommand::undo()
{
  m_shape->setVertex(m_activeVertex, oldPosition);
  m_mapperGLCanvas->update();
  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());
}

void MoveVertexCommand::redo()
{
  m_shape->setVertex(m_activeVertex, newPosition);
  m_mapperGLCanvas->update();
  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());
}


MoveShapesCommand::MoveShapesCommand(MapperGLCanvas *mapperGLCanvas, Shape *shape, QMouseEvent *event, const QPointF &point, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  m_mapperGLCanvas = mapperGLCanvas;
  m_shape = shape;
  m_event = event;
  newPosition = point;
}

void MoveShapesCommand::undo()
{
  m_shape->translate(oldPosition.x(), oldPosition.y());
  m_mapperGLCanvas->update();
  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());
}

void MoveShapesCommand::redo()
{
  m_shape->translate(m_event->x() - newPosition.x(), m_event->y() - newPosition.y());
  m_mapperGLCanvas->update();
  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());

  oldPosition.setX(newPosition.x() - m_event->x());
  oldPosition.setY(newPosition.y() - m_event->y());
}


DeleteMappingCommand::DeleteMappingCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  m_mainWindow = mainWindow;
  m_mappingId = mappingId;
}

void DeleteMappingCommand::undo()
{
  if(m_mappingPtr != NULL)
    {
      uint currentId = m_mainWindow->getMappingManager().addMapping(m_mappingPtr);
      m_mainWindow->addMappingItem(currentId);
    }
}

void DeleteMappingCommand::redo()
{
  m_mappingPtr = m_mainWindow->getMappingManager().getMappingById(m_mappingId);
  m_mainWindow->deleteMapping(m_mappingId);
}
