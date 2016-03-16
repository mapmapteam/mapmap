/*
 * ShapeControlPainter.h
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

#ifndef SHAPE_CONTROL_PAINTER_H_
#define SHAPE_CONTROL_PAINTER_H_

#include <QtGlobal>

#include "MM.h"
#include "MapperGLCanvas.h"
#include "ShapeGraphicsItem.h"

MM_BEGIN_NAMESPACE

class ShapeGraphicsItem;
class MapperGLCanvas;

/**
 * An object responsible to paint control points for a specific ShapeGraphicsItem.
 */
class ShapeControlPainter
{
public:
  typedef QSharedPointer<ShapeControlPainter> ptr;

  ShapeControlPainter(ShapeGraphicsItem* shapeItem);
  virtual ~ShapeControlPainter() {}

  MShape::ptr getShape() const;

  virtual void paint(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices = QList<int>());

protected:
  virtual void _paintShape(QPainter *painter, MapperGLCanvas* canvas) = 0;
  virtual void _paintVertices(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices = QList<int>());

  QPen getRescaledShapeStroke(MapperGLCanvas* canvas, bool innerStroke=false);

  ShapeGraphicsItem* _shapeItem;
};

/// Control painter for polygons.
class PolygonControlPainter : public ShapeControlPainter
{
public:
  PolygonControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~PolygonControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter, MapperGLCanvas* canvas);
};

/// Control painter for ellipses.
class EllipseControlPainter : public ShapeControlPainter
{
public:
  EllipseControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~EllipseControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter, MapperGLCanvas* canvas);
};

/// Control painter for meshes.
class MeshControlPainter : public ShapeControlPainter
{
public:
  MeshControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~MeshControlPainter() {}

protected:
  virtual void _paintShape(QPainter *painter, MapperGLCanvas* canvas);
};

MM_END_NAMESPACE

#endif
