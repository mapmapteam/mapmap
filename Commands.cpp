
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
