/*
 * ShapeHandles.cpp
 *
 * (c) 2026 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "ShapeHandles.h"

#include <QLineF>
#include <QPolygonF>
#include <QtMath>

#include <algorithm>
#include <cmath>

#include "Mesh.h"

namespace mmp {

namespace {

/// Returns v scaled to a length of 1, or fallback when v has no length.
QPointF _normalized(const QPointF& v, const QPointF& fallback)
{
  const qreal length = QLineF(QPointF(), v).length();
  return (length > 0) ? v / length : fallback;
}

/**
 * Returns the point halfway along a polyline, with the direction
 * perpendicular to the segment that holds it, on the side away from center.
 */
HandleAnchor _polylineMiddle(const QPolygonF& polyline, const QPointF& center, const QPointF& fallback)
{
  HandleAnchor anchor { polyline.isEmpty() ? center : polyline.first(), fallback, QPointF(1, 0) };
  if (polyline.size() < 2)
    return anchor;

  qreal totalLength = 0;
  for (int i = 1; i < polyline.size(); i++)
    totalLength += QLineF(polyline.at(i - 1), polyline.at(i)).length();

  // Walk the polyline until half of its length.
  qreal remaining = totalLength / 2;
  QLineF segment(polyline.at(0), polyline.at(1));
  for (int i = 1; i < polyline.size(); i++)
  {
    segment = QLineF(polyline.at(i - 1), polyline.at(i));
    if (remaining <= segment.length() || i == polyline.size() - 1)
      break;
    remaining -= segment.length();
  }
  anchor.point = (segment.length() > 0) ? segment.pointAt(remaining / segment.length()) : segment.p1();

  // Perpendicular to the segment, turned away from the center.
  QPointF normal(segment.dy(), -segment.dx());
  if (QPointF::dotProduct(normal, anchor.point - center) < 0)
    normal = -normal;
  if (normal.isNull())
    normal = anchor.point - center;
  anchor.outward = _normalized(normal, fallback);
  anchor.along = _normalized(segment.p2() - segment.p1(), QPointF(1, 0));
  return anchor;
}

}

HandleAnchor handleAnchor(const MShape& shape)
{
  const QPointF center = shape.getCenter();
  // The handle points up when the shape gives no direction.
  const QPointF fallback(0, -1);

  QPolygonF polyline;
  switch (shape.getType())
  {
  case MShape::Quad:
    if (shape.nVertices() >= 3)
      polyline << shape.getVertex(0) << shape.getVertex(1);
    break;

  case MShape::Triangle:
    if (shape.nVertices() >= 3)
      polyline << shape.getVertex(0) << shape.getVertex(2);
    break;

  case MShape::Mesh:
  {
    const Mesh& mesh = static_cast<const Mesh&>(shape);
    for (int column = 0; column < mesh.nColumns(); column++)
      polyline << mesh.getVertex2d(column, 0);
    break;
  }

  case MShape::Ellipse:
    if (shape.nVertices() >= 3)
    {
      const QPointF point = shape.getVertex(1);
      const QPointF right = shape.getVertex(2);
      return HandleAnchor { point, _normalized(point - center, fallback), _normalized(right - center, QPointF(1, 0)) };
    }
    break;
  }

  return _polylineMiddle(polyline, center, fallback);
}

bool HandleAnchor::isMirrored() const
{
  // The right of the image, when it is not seen in mirror.
  const QPointF right(-outward.y(), outward.x());
  return QPointF::dotProduct(right, along) < 0;
}

QPointF handlePosition(const MShape& shape, qreal distance)
{
  const HandleAnchor anchor = handleAnchor(shape);
  return anchor.point + distance * anchor.outward;
}

HandleMode nextHandleMode(HandleMode mode)
{
  switch (mode)
  {
  case HandleMode::Free:   return HandleMode::Rotate;
  case HandleMode::Rotate: return HandleMode::Scale;
  case HandleMode::Scale:  return HandleMode::Free;
  }
  return HandleMode::Free;
}

HandleDrag::HandleDrag(const QPointF& center, const QPointF& pressPoint, HandleMode mode)
  : _center(center),
    _lastPoint(pressPoint),
    _rotation(0),
    _scale(1),
    _mode(mode),
    _stepped(false)
{}

void HandleDrag::setMode(HandleMode mode)
{
  // Freeze what the new mode no longer changes at the value shown.
  if (mode == HandleMode::Scale)
    _rotation = rotation();
  if (mode == HandleMode::Rotate)
    _scale = scale();
  _mode = mode;
}

void HandleDrag::setStepped(bool stepped)
{
  _stepped = stepped;
}

void HandleDrag::moveTo(const QPointF& point, bool fine)
{
  const QPointF previous = _lastPoint - _center;
  const QPointF current = point - _center;
  // Directions and ratios are undefined at the center: ignore such points.
  if (previous.isNull() || current.isNull())
    return;

  const qreal part = fine ? FINE_FACTOR : 1;
  if (_mode != HandleMode::Scale)
  {
    // Signed angle from previous to current; positive is clockwise on screen (y points down).
    const qreal cross = previous.x() * current.y() - previous.y() * current.x();
    const qreal dot = QPointF::dotProduct(previous, current);
    _rotation += part * qRadiansToDegrees(std::atan2(cross, dot));
  }
  if (_mode != HandleMode::Rotate)
  {
    const qreal ratio = QLineF(QPointF(), current).length() / QLineF(QPointF(), previous).length();
    _scale *= std::pow(ratio, part);
  }
  _lastPoint = point;
}

qreal HandleDrag::rotation() const
{
  const bool rounded = _stepped && _mode != HandleMode::Scale;
  return rounded ? qRound(_rotation / ROTATION_STEP_DEGREES) * ROTATION_STEP_DEGREES : _rotation;
}

qreal HandleDrag::scale() const
{
  const bool rounded = _stepped && _mode != HandleMode::Rotate;
  const qreal scale = rounded ? 1 + qRound((_scale - 1) / SCALE_STEP) * SCALE_STEP : _scale;
  return std::max(scale, MIN_SCALE);
}

QTransform HandleDrag::transform() const
{
  QTransform transform;
  transform.translate(_center.x(), _center.y());
  transform.rotate(rotation());
  transform.scale(scale(), scale());
  transform.translate(-_center.x(), -_center.y());
  return transform;
}

}
