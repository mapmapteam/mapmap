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
#include "MM.h"

namespace mmp {

enum CommandId {
  CMD_KEY_MOVE_VERTEX,
  CMD_MOUSE_MOVE_VERTEX,
  CMD_KEY_TRANSLATE_SHAPE,
  CMD_MOUSE_TRANSLATE_SHAPE,
  CMD_KEY_SCALE_ROTATE_SHAPE,
  CMD_MOUSE_SCALE_ROTATE_SHAPE,
  CMD_KEY_FLIP_SHAPE,
  CMD_MOUSE_FLIP_SHAPE,
  CMD_KEY_ROTATE_SHAPE,
  CMD_MOUSE_ROTATE_SHAPE,
};

class MainWindow;
class MapperGLCanvas;

class AddPaintCommand : public QUndoCommand
{
public:
  explicit AddPaintCommand(MainWindow *mainWindow, uid paintId, const QIcon &icon, const QString &name, QUndoCommand *parent = 0);

  void undo() Q_DECL_OVERRIDE;
  void redo() Q_DECL_OVERRIDE;

private:
  MainWindow *_mainWindow;
  Paint::ptr _paint;
  uid _paintId;
  QIcon _icon;
  QString _name;
};

class AddMappingCommand : public QUndoCommand
{
public:
  explicit AddMappingCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent = 0);

  void undo() Q_DECL_OVERRIDE;
  void redo() Q_DECL_OVERRIDE;

private:
  MainWindow *_mainWindow;
  Mapping::ptr _mapping;
  uid _mappingId;
};

class DuplicateMappingCommand : public AddMappingCommand
{
public:
  explicit DuplicateMappingCommand(MainWindow *mainWindow, uid cloneId, QUndoCommand *parent = 0);

};

class TransformShapeCommand : public QUndoCommand
{
public:
  enum TransformShapeOption {
    STEP,
    FREE,
    RELEASE,
  };
  TransformShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, QUndoCommand *parent = 0);

  virtual void undo();
  virtual void redo();
  virtual bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape) = 0;

  // Information pertaining to the shape context.
  MapperGLCanvas* _canvas;
  QWeakPointer<MShape> _shape;

  // Did we use keys to move that vertex?
  int _option;

  // Clone of the original shape used for undoing purposes.
  MShape::ptr _originalShape;
};

class MoveVertexCommand : public TransformShapeCommand
{
public:
  MoveVertexCommand(MapperGLCanvas* canvas, TransformShapeOption option, int activeVertex, const QPointF &point, QUndoCommand *parent = 0);

  int id() const;
  bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape);

  // Information pertaining to the moving of vertex.
  int _movedVertex;
  QPointF _vertexPosition;
};

class ScaleRotateShapeCommand : public TransformShapeCommand
{
public:
  ScaleRotateShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, int activeVertex, const QPointF &point, const QPointF& initialPositionPoint, const MShape::ptr& initialShape, MShape::ShapeMode mode, QUndoCommand *parent = 0);

  int id() const;
  bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape);

  // Information pertaining to the moving of vertex.
  int _movedVertex;
  QTransform _transform;
  MShape::ptr _initialShape;
};

class TranslateShapeCommand : public TransformShapeCommand
{
public:
  TranslateShapeCommand(MapperGLCanvas *canvas, TransformShapeOption option, const QPointF &translation, QUndoCommand *parent = 0);

  int id() const;
  bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape);

private:
  QPointF _translation;
};

class FlipShapeCommand : public TransformShapeCommand
{
public:
  FlipShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, const MShape::ptr& initialShape, MShape::FlipDirection direction, QUndoCommand *parent = 0);

  int id() const;
  bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape);

  // Information pertaining to flip transformation.
  QTransform _transform;
  MShape::ptr _initialShape;
};

class RotateShapeCommand : public TransformShapeCommand
{
public:
  RotateShapeCommand(MapperGLCanvas* canvas, TransformShapeOption option, const MShape::ptr& initialShape, MShape::Rotation rotation, QUndoCommand *parent = 0);

  int id() const;
  bool mergeWith(const QUndoCommand* other);

protected:
  // Perform the actual transformation on the shape.
  virtual void _doTransform(MShape::ptr shape);

  // Information pertaining to rotate transformation.
  QTransform _transform;
  MShape::ptr _initialShape;
};

class RemovePaintCommand : public QUndoCommand
{
public:
  explicit RemovePaintCommand(MainWindow *mainWindow, uid paintId, QUndoCommand *parent = 0);

  void undo() Q_DECL_OVERRIDE;
  void redo() Q_DECL_OVERRIDE;

private:
  MainWindow *_mainWindow;
  Paint::ptr _paint;
  uid _paintId;
  QMap<uid, Mapping::ptr> _paintMappings; // saves mappings associated with paint
};

class DeleteMappingCommand : public QUndoCommand
{
public:
  explicit DeleteMappingCommand(MainWindow *mainWindow, uid mappingId, QUndoCommand *parent = 0);

  void undo() Q_DECL_OVERRIDE;
  void redo() Q_DECL_OVERRIDE;

private:
  MainWindow *_mainWindow;
  Mapping::ptr _mapping;
  uid _mappingId;
};

class MoveMappingCommand : public QUndoCommand
{
public:
  explicit MoveMappingCommand(MainWindow *mainWindow, uid mappingId, MM::MoveElement moveType, QUndoCommand *parent = 0);

  void undo() Q_DECL_OVERRIDE;
  void redo() Q_DECL_OVERRIDE;

private:
  MainWindow *_mainWindow;
  Mapping::ptr _mapping;
  uid _mappingId;
  MM::MoveElement _moveType;
  int _fromIdx;
  int _toIdx;
};

}

#endif /* COMMANDS_H_ */
