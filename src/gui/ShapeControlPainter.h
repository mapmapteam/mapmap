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
#include "ShapeHandles.h"
#include "MapperGLCanvas.h"
#include "ShapeGraphicsItem.h"

namespace mmp {

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

  // Just paints the shape (selected=false is for painting shapes in the "background" when the option to display all controls is chosen).
  virtual void paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected=true) = 0;
  /**
   * Paints the shape and its controls. linkedVertex is the vertex that goes
   * with the one selected in the editor of the other side (input or output),
   * or a negative number.
   */
  virtual void paint(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices = QList<int>(),
                     int linkedVertex = -1);

protected:
  virtual void _paintVertices(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices = QList<int>(),
                              int linkedVertex = -1);

  /**
   * Draws the handle of the shape, unless it is locked: a disc outside the
   * right of the image with the pictogram of the current mode (free, rotate or
   * scale), drawn the way the image is set in the shape.
   */
  void _paintHandles(QPainter *painter, MapperGLCanvas* canvas);

  /// Pictograms of the handle, drawn around the origin in radii of the icon
  /// (x to the right of the image, y to its bottom).
  static void _drawScaleGlyph(QPainter* painter);  ///< Four arrows in a cross.
  static void _drawRotateGlyph(QPainter* painter); ///< A circular arrow.
  static void _drawFreeGlyph(QPainter* painter);   ///< Half a turn and an arrow leaving the center.

  QPen getRescaledShapeStroke(MapperGLCanvas* canvas, bool selected=true, bool innerStroke=false);

  ShapeGraphicsItem* _shapeItem;
};

/// Control painter for polygons.
class PolygonControlPainter : public ShapeControlPainter
{
public:
  PolygonControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~PolygonControlPainter() {}

  virtual void paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected=true);
};

/// Control painter for ellipses.
class EllipseControlPainter : public ShapeControlPainter
{
public:
  EllipseControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~EllipseControlPainter() {}

  virtual void paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected=true);
};

/// Control painter for meshes.
class MeshControlPainter : public ShapeControlPainter
{
public:
  MeshControlPainter(ShapeGraphicsItem* shapeItem) : ShapeControlPainter(shapeItem) {}
  virtual ~MeshControlPainter() {}

  virtual void paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected=true);
};

}

#endif
