/*
 * Commands.cpp
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

#include "MainWindow.h"
#include "MapperGLCanvas.h"

#include "Commands.h"

namespace mmp {

AddSourceCommand::AddSourceCommand(MainWindow *mainWindow, uid sourceId, const QIcon &icon, const QString &name, QUndoCommand *parent) :
  QUndoCommand(parent),
  _mainWindow(mainWindow),
  _sourceId(sourceId),
  _icon(icon),
  _name(name)
{
  setText(QObject::tr("Add source"));
}

void AddSourceCommand::undo()
{
  _source = _mainWindow->getMappingManager().getSourceById(_sourceId);
  _mainWindow->removeSourceItem(_sourceId);
}

void AddSourceCommand::redo()
{
  if (!_source.isNull())
  {
    uid lastId = _mainWindow->getMappingManager().addSource(_source);
    _mainWindow->addSourceItem(lastId, _icon, _name);
  }
  else {
    _mainWindow->addSourceItem(_sourceId, _icon, _name);
  }
}

AddLayerCommand::AddLayerCommand(MainWindow *mainWindow, uid layerId, QUndoCommand *parent):
  QUndoCommand(parent),
  _mainWindow(mainWindow),
  _layerId(layerId)
{
  setText(QObject::tr("Add layer"));
}

void AddLayerCommand::undo()
{
  _layer = _mainWindow->getMappingManager().getLayerById(_layerId);
  _mainWindow->deleteLayer(_layerId);
}

void AddLayerCommand::redo()
{
  if (!_layer.isNull())
  {
    uid storedId = _mainWindow->getMappingManager().addLayer(_layer);
    _mainWindow->addLayerItem(storedId);
  }
  else
  {
    _mainWindow->addLayerItem(_layerId);
  }
}

DuplicateLayerCommand::DuplicateLayerCommand(MainWindow *mainWindow, uid cloneId, QUndoCommand *parent):
  AddLayerCommand(mainWindow, cloneId, parent)
{
  setText(QObject::tr("Duplicate layer"));
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


ScaleRotateShapeCommand::ScaleRotateShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, int activeVertex, const QPointF &point, const QPointF& initialPositionPoint, const MShape::ptr& initialShape, MShape::ShapeMode mode, QUndoCommand *parent)
  : TransformShapeCommand(canvas, option, parent),
    _movedVertex(activeVertex),
    _initialShape(initialShape)
{
	// Initial vector from center.
	QPointF center = initialShape->getCenter();
	QLineF initialVector(center, initialPositionPoint);
	QLineF currentVector(center, point);

	// Compute scale.
	qreal scale = currentVector.length() / initialVector.length();

	// Compute rotation.
	qreal rotation = currentVector.angleTo(initialVector);

	// Create transform object.
	_transform.translate(+center.x(), +center.y());
	if (mode == MShape::RotateMode) {
		 setText(QObject::tr("Rotate shape"));
		_transform.rotate(rotation);
	}
	if (mode == MShape::ScaleMode) {
		setText(QObject::tr("Scale shape"));
		_transform.scale(scale, scale);
	}
	_transform.translate(-center.x(), -center.y());
}

int ScaleRotateShapeCommand::id() const { return (_option == STEP ? CMD_KEY_SCALE_ROTATE_SHAPE : CMD_MOUSE_SCALE_ROTATE_SHAPE); }

void ScaleRotateShapeCommand::_doTransform(MShape::ptr shape)
{
	// Apply to shape.
	shape->copyFrom(*_initialShape);
	shape->applyTransform(_transform);
}

bool ScaleRotateShapeCommand::mergeWith(const QUndoCommand* other)
{
  if (!TransformShapeCommand::mergeWith(other))
    return false;

  const ScaleRotateShapeCommand* cmd = static_cast<const ScaleRotateShapeCommand*>(other);

  // Needs to be the same vertex.
  if (cmd->_movedVertex != _movedVertex)
    return false;

	_option = cmd->_option;
	_transform = cmd->_transform;

	return true;
}


TranslateShapeCommand::TranslateShapeCommand(MapperGLCanvas *canvas, TransformShapeOption option, const QPointF &translation, QUndoCommand *parent)
  : TransformShapeCommand(canvas, option, parent),
    _translation(translation)
{
  setText(QObject::tr("Move shape"));
}

int TranslateShapeCommand::id() const { return (_option == STEP ? CMD_KEY_TRANSLATE_SHAPE : CMD_MOUSE_TRANSLATE_SHAPE); }

bool TranslateShapeCommand::mergeWith(const QUndoCommand* other)
{
  if (!TransformShapeCommand::mergeWith(other))
    return false;

  const TranslateShapeCommand* cmd = static_cast<const TranslateShapeCommand*>(other);

  // Update translation.
  _translation += cmd->_translation;
  _option = cmd->_option;
  return true;
}

void TranslateShapeCommand::_doTransform(MShape::ptr shape)
{
  // Apply translation.
  shape->translate(_translation);
}

RemoveSourceCommand::RemoveSourceCommand(MainWindow *mainWindow, uid sourceId, QUndoCommand *parent):
  QUndoCommand(parent),
  _mainWindow(mainWindow),
  _sourceId(sourceId),
  _sourceLayers()
{
  setText(QObject::tr("Remove media"));
}

void RemoveSourceCommand::undo()
{
  if (!_source.isNull())
  {
    MappingManager& manager = _mainWindow->getMappingManager();
    uid lastId = manager.addSource(_source);
    _mainWindow->addSourceItem(lastId, _source->getIcon(), _source->getName());

    // Add all mappings associated with source.
    QMap<uid, Layer::ptr> sourceLayers = manager.getSourceLayers(_source);
    for (QMap<uid, Layer::ptr>::const_iterator it = _sourceLayers.constBegin();
         it != _sourceLayers.constEnd(); ++it) {
      uid mid = manager.addLayer( it.value() );
      Q_ASSERT( mid == it.key() );
      _mainWindow->addLayerItem(mid);
    }
  }
}

void RemoveSourceCommand::redo()
{
   MappingManager& manager = _mainWindow->getMappingManager();
   bool deleteWithoutPrompt = (!_source.isNull()); // to avoid the pop-up window when redoing after undoing
  _source = manager.getSourceById(_sourceId);
  _sourceLayers = manager.getSourceLayers(_source);
  _mainWindow->deleteSource(_sourceId, deleteWithoutPrompt);
}



DeleteLayerCommand::DeleteLayerCommand(MainWindow *mainWindow, uid layerId, QUndoCommand *parent) :
  QUndoCommand(parent),
  _mainWindow(mainWindow),
  _layerId(layerId)
{
  setText(QObject::tr("Delete layer"));
}

void DeleteLayerCommand::undo()
{
  if (!_layer.isNull())
  {
    uid storedId = _mainWindow->getMappingManager().addLayer(_layer);
    _mainWindow->addLayerItem(storedId);
  }
}

void DeleteLayerCommand::redo()
{
  // Store mapping pointer before delete it
  _layer = _mainWindow->getMappingManager().getLayerById(_layerId);
  _mainWindow->deleteLayer(_layerId);
}

MoveLayerCommand::MoveLayerCommand(MainWindow *mainWindow, uid layerId,  MM::MoveElement moveType, QUndoCommand *parent) :
  QUndoCommand(parent),
  _mainWindow(mainWindow),
  _layerId(layerId),
  _moveType(moveType)
{
  switch (moveType) {
    case MM::Raise:  setText(QObject::tr("Raise layer")); break;
    case MM::Lower:  setText(QObject::tr("Lower layer")); break;
    case MM::Top:    setText(QObject::tr("Raise layer to top")); break;
    case MM::Bottom: setText(QObject::tr("Lower layer to bottom")); break;
    default:; // should not happen
  }
}

void MoveLayerCommand::undo()
{
  if (!_layer.isNull())
  {
    // Do the inverse move.
    _mainWindow->moveLayer(_layerId, _fromIdx);
  }
}

void MoveLayerCommand::redo()
{
  // Store mapping pointer before delete it
  _layer = _mainWindow->getMappingManager().getLayerById(_layerId);
  _fromIdx = _mainWindow->getMappingManager().getLayerIndex(_layer);
  int maxLayerIdx = _mainWindow->getMappingManager().nLayers()-1;

  switch (_moveType) {
    case MM::Raise:  _toIdx = qMax(_fromIdx - 1, 0); break;
    case MM::Lower:  _toIdx = qMin(_fromIdx + 1, maxLayerIdx); break;
    case MM::Top:    _toIdx = 0; break;
    case MM::Bottom: _toIdx = maxLayerIdx; break;
    default:; // should not happen
  }

  // Do the move.
  _mainWindow->moveLayer(_layerId, _toIdx);
}

FlipShapeCommand::FlipShapeCommand(MapperGLCanvas *canvas, TransformShapeCommand::TransformShapeOption option, const MShape::ptr &initialShape, MShape::FlipDirection direction, QUndoCommand *parent)
  : TransformShapeCommand (canvas, option, parent),
    _initialShape(initialShape)
{
	// Initial vector from center.
	QPointF center = initialShape->getCenter();

	// Create transform object.
	_transform.translate(+center.x(), +center.y());

	if (direction == MShape::Horizontal) {
		setText(QObject::tr("Flip Horizontally"));
		_transform.scale(-1, 1);
	}

	else if (direction == MShape::Vertical) {
		setText(QObject::tr("Flip Vertically"));
		_transform.scale(1, -1);
	}
	_transform.translate(-center.x(), -center.y());
}

int FlipShapeCommand::id() const { return (_option == STEP ? CMD_KEY_FLIP_SHAPE : CMD_MOUSE_FLIP_SHAPE); }

void FlipShapeCommand::_doTransform(MShape::ptr shape)
{
	// Apply to shape.
	shape->copyFrom(*_initialShape);
	shape->applyTransform(_transform);
}

bool FlipShapeCommand::mergeWith(const QUndoCommand* other)
{
  if (!TransformShapeCommand::mergeWith(other))
    return false;

  const FlipShapeCommand* cmd = static_cast<const FlipShapeCommand*>(other);

	_option = cmd->_option;
	_transform = cmd->_transform;

	return true;
}

RotateShapeCommand::RotateShapeCommand(MapperGLCanvas *canvas, TransformShapeCommand::TransformShapeOption option, const MShape::ptr &initialShape, MShape::Rotation rotation, QUndoCommand *parent)
  : TransformShapeCommand (canvas, option, parent),
    _initialShape(initialShape)
{
	// Initial vector from center.
	QPointF center = initialShape->getCenter();

	// Create transform object.
	_transform.translate(+center.x(), +center.y());

	if (rotation == MShape::Rotate90CW) {
		setText(QObject::tr("Rotate 90° CW"));
    _transform.rotate(90);
	}

  else if (rotation == MShape::Rotate90CCW) {
		setText(QObject::tr("Rotate 90° CCW"));
    _transform.rotate(-90);
	}

  else if (rotation == MShape::Rotate180) {
		setText(QObject::tr("Rotate 180°"));
    _transform.rotate(180);
	}

	_transform.translate(-center.x(), -center.y());
}

int RotateShapeCommand::id() const { return (_option == STEP ? CMD_KEY_ROTATE_SHAPE : CMD_MOUSE_ROTATE_SHAPE); }

void RotateShapeCommand::_doTransform(MShape::ptr shape)
{
	// Apply to shape.
	shape->copyFrom(*_initialShape);
	shape->applyTransform(_transform);
}

bool RotateShapeCommand::mergeWith(const QUndoCommand* other)
{
  if (!TransformShapeCommand::mergeWith(other))
    return false;

  const RotateShapeCommand* cmd = static_cast<const RotateShapeCommand*>(other);

	_option = cmd->_option;
	_transform = cmd->_transform;

	return true;
}

}
