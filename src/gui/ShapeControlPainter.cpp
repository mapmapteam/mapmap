/*
 * ShapeControlPainter.cpp
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

#include "ShapeControlPainter.h"

#include "MainWindow.h"

#include <cmath>

#include <QtMath>

namespace mmp {

namespace {

// Pictograms of the handle, in fractions of its radius.
const qreal GLYPH_HALF_LENGTH = 0.65;
const qreal ARROW_HEAD_LENGTH = 0.25;
const qreal ARROW_HEAD_HALF_WIDTH = 0.18;
const qreal ROTATE_GLYPH_RADIUS = 0.5;

// Arc of the rotation pictogram, in degrees counterclockwise from the right.
const qreal ROTATE_GLYPH_START_DEGREES = 60;
const qreal ROTATE_GLYPH_SPAN_DEGREES = 270;

// QPainter::drawArc() counts angles in sixteenths of a degree.
const int QT_ANGLE_UNITS_PER_DEGREE = 16;

/// Draws the two sides of an arrow head whose tip points along direction (of length 1), in radii.
void _drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& direction)
{
  const QPointF back = tip - ARROW_HEAD_LENGTH * direction;
  const QPointF side = ARROW_HEAD_HALF_WIDTH * QPointF(-direction.y(), direction.x());
  painter->drawLine(tip, back + side);
  painter->drawLine(tip, back - side);
}

// Pictogram of the free mode: half a turn on the side of the shape, and an
// arrow leaving the center toward the top right of the image.
const qreal FREE_GLYPH_ARC_RADIUS = 0.5;
const qreal FREE_GLYPH_ARC_START_DEGREES = 135;
const qreal FREE_GLYPH_ARC_SPAN_DEGREES = 180;
const QPointF FREE_GLYPH_ARROW_TAIL(-0.04, 0.04);
const QPointF FREE_GLYPH_ARROW_TIP(0.46, -0.46);

/// Returns the point of a circle of the given radius at an angle counted
/// counterclockwise on screen (y points down), with the direction it moves to
/// when the angle grows.
QPointF _pointOnCircle(qreal radius, qreal degrees, QPointF* direction)
{
  const qreal angle = qDegreesToRadians(degrees);
  *direction = QPointF(-std::sin(angle), -std::cos(angle));
  return radius * QPointF(std::cos(angle), -std::sin(angle));
}

/// Draws an arc of a circle centered on the origin, with an arrow head at its end.
void _drawArcArrow(QPainter* painter, qreal radius, qreal startDegrees, qreal spanDegrees)
{
  const QRectF bounds(-radius, -radius, 2 * radius, 2 * radius);
  painter->drawArc(bounds, qRound(startDegrees * QT_ANGLE_UNITS_PER_DEGREE),
                   qRound(spanDegrees * QT_ANGLE_UNITS_PER_DEGREE));
  QPointF direction;
  const QPointF tip = _pointOnCircle(radius, startDegrees + spanDegrees, &direction);
  _drawArrowHead(painter, tip, direction);
}

}


ShapeControlPainter::ShapeControlPainter(ShapeGraphicsItem* shapeItem)
  : _shapeItem(shapeItem)
{}

MShape::ptr ShapeControlPainter::getShape() const { return _shapeItem->getShape(); }

void ShapeControlPainter::paint(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices)
{
  paintShape(painter, canvas);
  // Handles first, so that their lines pass under the vertices.
  _paintHandles(painter, canvas);
  _paintVertices(painter, canvas, selectedVertices);
}

void ShapeControlPainter::_paintHandles(QPainter *painter, MapperGLCanvas* canvas)
{
  const MShape::ptr shape = getShape();
  if (shape->isLocked())
    return;

  const qreal zoomFactor = canvas->getZoomFactor();
  const qreal radius = MM::HANDLE_RADIUS / zoomFactor;
  const HandleAnchor anchor = handleAnchor(*shape);
  const QPointF center = handlePosition(*shape, MM::HANDLE_DISTANCE / zoomFactor);

  painter->save();

  // A thin line from the shape to the icon.
  painter->setPen(QPen(MM::CONTROL_COLOR, MM::SHAPE_STROKE_WIDTH / zoomFactor));
  painter->setBrush(Qt::NoBrush);
  painter->drawLine(anchor.point, center - radius * anchor.outward);

  // A disc...
  painter->setPen(Qt::NoPen);
  painter->setBrush(MM::CONTROL_COLOR);
  painter->drawEllipse(center, radius, radius);

  // ...with the pictogram of the mode, drawn in the frame of the image so that
  // it turns over when the image is seen in mirror: x to the right of the
  // image, away from the shape, y to its bottom, in radii of the icon.
  const QPointF right = anchor.outward;
  QPointF down(-right.y(), right.x());
  if (anchor.isMirrored())
    down = -down;
  painter->setTransform(QTransform(right.x() * radius, right.y() * radius,
                                   down.x() * radius, down.y() * radius,
                                   center.x(), center.y()), true);
  painter->setPen(QPen(MM::HANDLE_GLYPH_COLOR, MM::HANDLE_GLYPH_STROKE_WIDTH / MM::HANDLE_RADIUS,
                       Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter->setBrush(Qt::NoBrush);
  switch (MainWindow::window()->handleModeKeys()->mode())
  {
  case HandleMode::Rotate: _drawRotateGlyph(painter); break;
  case HandleMode::Scale:  _drawScaleGlyph(painter);  break;
  case HandleMode::Free:   _drawFreeGlyph(painter);   break;
  }

  painter->restore();
}

void ShapeControlPainter::_paintVertices(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices)
{
  qreal zoomFactor = canvas->getZoomFactor();
  qreal selectRadius = (getShape()->isLocked() ? MM::VERTEX_LOCKED_RADIUS : MM::VERTEX_SELECT_RADIUS) / zoomFactor;
  qreal strokeWidth  = MM::VERTEX_SELECT_STROKE_WIDTH / zoomFactor;

  for (int i=0; i<getShape()->nVertices(); i++)
    Util::drawControlsVertex(painter, getShape()->getVertex(i), selectedVertices.contains(i), getShape()->isLocked(), selectRadius, strokeWidth);
}

QPen ShapeControlPainter::getRescaledShapeStroke(MapperGLCanvas* canvas, bool selected, bool innerStroke)
{
  const QBrush& brush =
  (selected ?
                          (getShape()->isLocked() ? QBrush(MM::CONTROL_LOCKED_COLOR) : QBrush(MM::CONTROL_COLOR)) :
                          QBrush(MM::CONTROL_COLOR_NON_SELECTED));
  return QPen(brush,
              (innerStroke ? MM::SHAPE_INNER_STROKE_WIDTH : MM::SHAPE_STROKE_WIDTH) / canvas->getZoomFactor());
}

void PolygonControlPainter::paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected)
{
  Polygon* poly = static_cast<Polygon*>(getShape().data());
  Q_ASSERT(poly);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas, selected));
  painter->setBrush(Qt::NoBrush);

  // Draw inner quads.
  painter->drawPolygon(poly->toPolygon());
}


void EllipseControlPainter::paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected)
{
  Ellipse* ellipse = static_cast<Ellipse*>(getShape().data());
  Q_ASSERT(ellipse);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas, selected));
  painter->setBrush(Qt::NoBrush);

  // Draw ellipse contour.
  painter->drawPolygon(ellipse->toPolygon());
}

void MeshControlPainter::paintShape(QPainter *painter, MapperGLCanvas* canvas, bool selected)
{
  Mesh* mesh = static_cast<Mesh*>(getShape().data());
  Q_ASSERT(mesh);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas, selected, true));
  painter->setBrush(Qt::NoBrush);

  // Draw inner quads.
  QVector<Quad::ptr> quads = mesh->getQuads();
  for (QVector<Quad::ptr>::const_iterator it = quads.begin(); it != quads.end(); ++it)
  {
    painter->drawPolygon((*it)->toPolygon());
  }

  // Draw outer quad.
  painter->setPen(getRescaledShapeStroke(canvas, selected));
  painter->drawPolygon(_shapeItem->mapFromScene(mesh->toPolygon()));
}

void ShapeControlPainter::_drawScaleGlyph(QPainter* painter)
{
  // Four arrows in a cross, along the diagonals.
  for (const QPointF& diagonal : { QPointF(1, 1), QPointF(1, -1) })
  {
    const QPointF direction = diagonal / M_SQRT2;
    const QPointF tip = GLYPH_HALF_LENGTH * direction;
    painter->drawLine(-tip, tip);
    _drawArrowHead(painter, tip, direction);
    _drawArrowHead(painter, -tip, -direction);
  }
}

void ShapeControlPainter::_drawRotateGlyph(QPainter* painter)
{
  // Three quarters of a circle, with an arrow head at its end.
  _drawArcArrow(painter, ROTATE_GLYPH_RADIUS, ROTATE_GLYPH_START_DEGREES, ROTATE_GLYPH_SPAN_DEGREES);
}

void ShapeControlPainter::_drawFreeGlyph(QPainter* painter)
{
  _drawArcArrow(painter, FREE_GLYPH_ARC_RADIUS, FREE_GLYPH_ARC_START_DEGREES, FREE_GLYPH_ARC_SPAN_DEGREES);
  painter->drawLine(FREE_GLYPH_ARROW_TAIL, FREE_GLYPH_ARROW_TIP);
  const QPointF direction = FREE_GLYPH_ARROW_TIP - FREE_GLYPH_ARROW_TAIL;
  _drawArrowHead(painter, FREE_GLYPH_ARROW_TIP, direction / QLineF(QPointF(), direction).length());
}

}
