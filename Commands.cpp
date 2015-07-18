
#include "Commands.h"

AddShapesCommand::AddShapesCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent):
  QUndoCommand(parent)
{
  setText(QObject::tr("Add mapping"));
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


MoveVertexCommand::MoveVertexCommand(MapperGLCanvas* canvas, int activeVertex, const QPointF &point, MoveVertexOption option, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  setText(QObject::tr("Move vertex"));
  _canvas = canvas;
  _shape = canvas->getCurrentShape();
  _movedVertex = activeVertex;
  _vertexPosition = point;
  _option = option;
  _originalShape.reset(_shape.toStrongRef()->clone());
}

int MoveVertexCommand::id() const { return (_option == KEY_MOVE ? CMD_KEY_MOVE_VERTEX : CMD_MOUSE_MOVE_VERTEX); }

void MoveVertexCommand::undo()
{
  _shape.toStrongRef()->copyFrom(*_originalShape);
  _canvas->update();
  _canvas->currentShapeWasChanged();
}

void MoveVertexCommand::redo()
{
  _shape.toStrongRef()->setVertex(_movedVertex, _vertexPosition);
  _canvas->update();
  _canvas->currentShapeWasChanged();
}

bool MoveVertexCommand::mergeWith(const QUndoCommand* other)
{
  if (other->id() != id()) // make sure other is also an AppendText command
    return false;

  const MoveVertexCommand* cmd = static_cast<const MoveVertexCommand*>(other);

  // Don't merge a new move with a dropped vertex move (ie. each drag'n'drop is considered
  // as a single separate command).
  if (_option == MOUSE_RELEASE && cmd->_option == MOUSE_MOVE)
    return false;

  if (cmd->_canvas != _canvas ||
      cmd->_shape != _shape ||
      cmd->_movedVertex != _movedVertex)
    return false;

  _vertexPosition = cmd->_vertexPosition;
  _option = cmd->_option;
  return true;
}


MoveShapesCommand::MoveShapesCommand(MapperGLCanvas *mapperGLCanvas, QMouseEvent *event, const QPointF &point, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  setText(QObject::tr("Move shape"));
  m_mapperGLCanvas = mapperGLCanvas;
  m_shape = m_mapperGLCanvas->getCurrentShape().data();
  m_event = event;
  newPosition = point;
}

void MoveShapesCommand::undo()
{
  m_shape->translate(oldPosition.x(), oldPosition.y());
  m_mapperGLCanvas->update();
//  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());
}

void MoveShapesCommand::redo()
{
  m_shape->translate(m_event->x() - newPosition.x(), m_event->y() - newPosition.y());
  m_mapperGLCanvas->update();
//  emit m_mapperGLCanvas->shapeChanged(m_mapperGLCanvas->getCurrentShape());

  oldPosition.setX(newPosition.x() - m_event->x());
  oldPosition.setY(newPosition.y() - m_event->y());
}


DeleteMappingCommand::DeleteMappingCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  setText(QObject::tr("Delete mapping"));
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
