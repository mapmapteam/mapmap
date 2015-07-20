
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


TransformShapeCommand::TransformShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, QUndoCommand* parent)
  : QUndoCommand(parent),
    _canvas(canvas), _option(option) {
  // Copy shape.
  _shape = canvas->getCurrentShape();
  _option = option;

  // Clone shape before applying transform.
  _originalShape.reset(_shape.toStrongRef()->clone());
}

void TransformShapeCommand::undo() {
  // Copy back shape.
  _shape.toStrongRef()->copyFrom(*_originalShape);

  // Update everything.
  _canvas->currentShapeWasChanged();
  _canvas->update();
}

void TransformShapeCommand::redo() {
  // Call transformation.
  _doTransform(_shape);

  // Update everything.
  _canvas->currentShapeWasChanged();
  _canvas->update();
}

bool TransformShapeCommand::mergeWith(const QUndoCommand* other) {
  // Make sure other is of the same type (id).
  if (other->id() != id())
    return false;

  const TransformShapeCommand* cmd = static_cast<const TransformShapeCommand*>(other);

  // Don't merge a new transform with a dropped tranform move (ie. each drag'n'drop is considered
  // as a single separate command).
  if (_option == RELEASE && cmd->_option == FREE)
    return false;

  // Don't merge transforms
  if (cmd->_canvas != _canvas ||
      cmd->_shape != _shape)
    return false;

  return true;
}

MoveVertexCommand::MoveVertexCommand(MapperGLCanvas* canvas, TransformShapeOption option, int activeVertex, const QPointF &point, QUndoCommand *parent)
  : TransformShapeCommand(canvas, option, parent),
    _movedVertex(activeVertex),
    _vertexPosition(point)
{
  setText(QObject::tr("Move vertex"));
}

int MoveVertexCommand::id() const { return (_option == STEP ? CMD_KEY_MOVE_VERTEX : CMD_MOUSE_MOVE_VERTEX); }

void MoveVertexCommand::_doTransform(MShape::ptr shape)
{
  shape->setVertex(_movedVertex, _vertexPosition);
}

bool MoveVertexCommand::mergeWith(const QUndoCommand* other)
{
  if (!TransformShapeCommand::mergeWith(other))
    return false;

  const MoveVertexCommand* cmd = static_cast<const MoveVertexCommand*>(other);

  // Needs to be the same vertex.
  if (cmd->_movedVertex != _movedVertex)
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
