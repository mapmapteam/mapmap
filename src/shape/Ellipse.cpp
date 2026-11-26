/*
 * Ellipse.cpp
 *
 * (c) 2016 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "Ellipse.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

#include <cmath>

namespace mmp {

namespace {

// Number of bisection steps used to clamp a vertex to a valid position.
const int N_CLAMP_STEPS = 20;

// Images of vertices 0-3 on the unit circle.
QPolygonF _unitQuarterPoints()
{
  QPolygonF points;
  points << QPointF(-1, 0) << QPointF(0, -1) << QPointF(1, 0) << QPointF(0, 1);
  return points;
}

// Returns the homography that keeps the unit circle as it is and moves its center to point p
// (inside the circle), without turning the circle around it. In homogeneous coordinates,
// with u the direction of p and g = 1 / sqrt(1 - |p|^2):
//   (x, y) -> (x, y) + (g - 1) ((x, y).u) u + g p w
//   w      -> g (w + (x, y).p)
// which leaves x^2 + y^2 - w^2, hence the circle, unchanged.
QTransform _shiftCircleCenter(const QPointF& p)
{
  const qreal distance = std::hypot(p.x(), p.y());
  if (qFuzzyIsNull(distance))
    return QTransform();

  const QPointF direction = p / distance;
  const qreal gain = 1 / std::sqrt(1 - distance*distance);
  const qreal stretch = gain - 1;
  return QTransform(1 + stretch * direction.x() * direction.x(), stretch * direction.x() * direction.y(), gain * p.x(),
                    stretch * direction.x() * direction.y(), 1 + stretch * direction.y() * direction.y(), gain * p.y(),
                    gain * p.x(), gain * p.y(), gain);
}

}

void Ellipse::setPerspective(bool perspective)
{
  _perspective = perspective;
  sanitize();
}

void Ellipse::sanitize()
{
  const QVector<QPointF> before = vertices;
  _sanitize();
  _verticesMoved(before);
}

void Ellipse::_sanitize()
{
  if (nVertices() < 4)
    return;

  // Fall back on an axis-aligned ellipse if needed.
  if (!_perspective || !isValid())
    _sanitizeAxes();

  // Center control point is always the perspective-correct center.
  if (hasCenterControl())
    _rawSetVertex(4, getCenter());
}

void Ellipse::_sanitizeAxes()
{
  // Get horizontal axis rotated 90 degrees CW
  QVector2D hAxis = getHorizontalAxis();
  const QVector2D center(_axisCenter());
  QVector2D hAxisRotated(hAxis.y(), -hAxis.x());

  // Project vertex 1 onto it.
  QVector2D vAxisNormalized = hAxisRotated.normalized();

  QVector2D vFromCenter = QVector2D(getVertex(1)) - center;
  const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;
  _rawSetVertex(1, (center + projection).toPointF());
  _rawSetVertex(3, (center - projection).toPointF());
}

QPointF Ellipse::getCenter() const
{
  QTransform transform;
  if (nVertices() >= 4 && _computeFromUnitCircle(transform))
    return transform.map(QPointF(0, 0));
  else
    return _axisCenter();
}

bool Ellipse::_computeFromUnitCircle(QTransform& transform) const
{
  QPolygonF quad;
  for (int i=0; i<4; i++)
    quad << getVertex(i);

  if (!QTransform::quadToQuad(_unitQuarterPoints(), quad, transform))
    return false;

  // Normalize sign so that homogeneous coordinate w is positive inside the circle
  // (QTransform clips points with w < 0).
  if (transform.m33() < 0)
    transform = QTransform(-transform.m11(), -transform.m12(), -transform.m13(),
                           -transform.m21(), -transform.m22(), -transform.m23(),
                           -transform.m31(), -transform.m32(), -transform.m33());
  return true;
}

bool Ellipse::_isValidTransform(const QTransform& transform)
{
  // Points of the unit circle are mapped with w = m13*x + m23*y + m33. The image of the circle
  // is an ellipse iff w > 0 on the whole circle, ie. iff the preimage of the line at infinity
  // does not intersect the circle. Ratio |(m13, m23)| / m33 is the strength of the perspective.
  if (transform.m33() <= 0 || qFuzzyIsNull(transform.determinant()))
    return false;
  return std::hypot(transform.m13(), transform.m23()) <= MAX_PERSPECTIVE * transform.m33();
}

bool Ellipse::isValid() const
{
  QTransform transform;
  return _computeFromUnitCircle(transform) && _isValidTransform(transform);
}

QTransform Ellipse::fromUnitCircle() const
{
  QTransform transform;
  _computeFromUnitCircle(transform);
  return transform;
}

QTransform Ellipse::toUnitCircle() const
{
  return fromUnitCircle().inverted();
}

QTransform Ellipse::_axisToUnitCircle() const
{
  const QPointF& center = _axisCenter();
  return QTransform().scale(1.0/getHorizontalRadius(), 1.0/getVerticalRadius())
                     .rotateRadians(-getRotationRadians())
                     .translate(-center.x(), -center.y());
}

QPolygonF Ellipse::toPolygon(int nSegments) const
{
  const QTransform transform = fromUnitCircle();
  QPolygonF polygon;
  for (int i=0; i<nSegments; i++)
  {
    qreal angle = 2 * M_PI * i / nSegments;
    polygon << transform.map(QPointF(cos(angle), sin(angle)));
  }
  return polygon;
}

bool Ellipse::includesPoint(qreal x, qreal y)
{
  return (QVector2D(toUnitCircle().map(QPointF(x, y))).length() <= 1);
}

void Ellipse::setVertex(int i, const QPointF& v)
{
  const QVector<QPointF> before = vertices;

  // Center control point: in perspective, move it inside the contour, which tilts the
  // ellipse; otherwise it stays in the middle of the axes, so move the whole shape.
  if (i == CENTER_VERTEX)
  {
    if (_perspective)
      _setPerspectiveCenter(v);
    else
    {
      const QPointF offset = v - getCenter();
      for (int j=0; j<N_DEFINING_VERTICES; j++)
        _rawSetVertex(j, getVertex(j) + offset);
    }
  }

  else if (_perspective)
    _setPerspectiveVertex(i, v);

  else
    _setAxisVertex(i, v);

  // Just to be sure.
  _sanitize();

  _verticesMoved(before);
}

void Ellipse::_setPerspectiveVertex(int i, const QPointF& v)
{
  const QPointF previous = getVertex(i);
  _rawSetVertex(i, v);
  if (isValid())
    return;

  // Find the furthest valid position on the segment previous -> v.
  qreal valid = 0;
  qreal invalid = 1;
  for (int k=0; k<N_CLAMP_STEPS; k++)
  {
    qreal t = (valid + invalid) / 2;
    _rawSetVertex(i, previous + t*(v - previous));
    if (isValid())
      valid = t;
    else
      invalid = t;
  }
  _rawSetVertex(i, previous + valid*(v - previous));
}

bool Ellipse::_computeWithCenter(const QTransform& fromCircle, const QPointF& center, QTransform& moved) const
{
  bool invertible = false;
  const QTransform toCircle = fromCircle.inverted(&invertible);
  if (!invertible)
    return false;

  // Where the center is asked to go, on the unit circle: it must stay inside. Its
  // homogeneous coordinate must keep the sign it has at the current center, which is
  // checked by hand as QTransform::map() hides it.
  auto homogeneous = [&toCircle](const QPointF& point) {
    return toCircle.m13() * point.x() + toCircle.m23() * point.y() + toCircle.m33();
  };
  if (homogeneous(center) * homogeneous(fromCircle.map(QPointF(0, 0))) <= 0)
    return false;
  const QPointF onCircle = toCircle.map(center);
  if (std::hypot(onCircle.x(), onCircle.y()) >= 1)
    return false;

  moved = _shiftCircleCenter(onCircle) * fromCircle;
  return _isValidTransform(moved);
}

void Ellipse::_setPerspectiveCenter(const QPointF& v)
{
  QTransform fromCircle;
  if (!_computeFromUnitCircle(fromCircle) || !_isValidTransform(fromCircle))
    return;
  const QPointF previous = fromCircle.map(QPointF(0, 0));

  // Find the furthest valid position on the segment previous -> v.
  QTransform moved;
  if (!_computeWithCenter(fromCircle, v, moved))
  {
    moved = fromCircle;
    qreal valid = 0;
    qreal invalid = 1;
    for (int k=0; k<N_CLAMP_STEPS; k++)
    {
      const qreal t = (valid + invalid) / 2;
      QTransform candidate;
      if (_computeWithCenter(fromCircle, previous + t*(v - previous), candidate))
      {
        valid = t;
        moved = candidate;
      }
      else
        invalid = t;
    }
  }

  // The contour stays; the four vertices slide along it.
  const QPolygonF quarterPoints = _unitQuarterPoints();
  for (int j=0; j<N_DEFINING_VERTICES; j++)
    _rawSetVertex(j, moved.map(quarterPoints.at(j)));
}

void Ellipse::_setAxisVertex(int i, const QPointF& v)
{
  // Save vertical axis vector.
  const QVector2D& vAxis  = getVerticalAxis();

  // If changed one of the two rotation-controlling points, adjust the other two points.
  if (i == 0 || i == 2)
  {
    // Transformation ellipse_t --> circle.
    QTransform transform = _axisToUnitCircle();

    // Change the vertex.
    _rawSetVertex(i, v);

    // Combine with transformation circle -> ellipse_{t+1}.
    transform *= _axisToUnitCircle().inverted();

    // Set vertices.
    _rawSetVertex(1, transform.map( getVertex(1) ));
    _rawSetVertex(3, transform.map( getVertex(3) ));
  }

  // If changed one of the two other points, just change the vertical axis.
  else if (i == 1 || i == 3)
  {
    // Retrieve the new horizontal axis vector and center.
    const QVector2D center(_axisCenter());

    QVector2D vFromCenter = QVector2D(v) - center;

    // Find projection of v onto vAxis / 2.
    QVector2D vAxisNormalized = vAxis.normalized();
    const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;

    // Assign vertical control points.
    if (i == 1)
    {
      _rawSetVertex(1, (center + projection).toPointF());
      _rawSetVertex(3, (center - projection).toPointF());
    }
    else
    {
      _rawSetVertex(1, (center - projection).toPointF());
      _rawSetVertex(3, (center + projection).toPointF());
    }
  }
}

void Ellipse::copyFrom(const MShape& shape)
{
  MShape::copyFrom(shape);
  const Ellipse* ellipse = qobject_cast<const Ellipse*>(&shape);
  if (ellipse)
    _perspective = ellipse->_perspective;
}

void Ellipse::read(const QJsonObject& obj)
{
  MShape::read(obj);

  // Not an ellipse: left as it is.
  if (nVertices() < N_DEFINING_VERTICES)
    return;

  if (nVertices() > N_DEFINING_VERTICES + 1)
    qWarning() << "Ellipse with" << nVertices() << "vertices: only the first"
               << N_DEFINING_VERTICES << "are read";

  // The center control point is derived from the other vertices: the one of the
  // project, if any, is not read.
  vertices.resize(N_DEFINING_VERTICES);
  _addVertex(getCenter());
  _verticesChanged();
}

void Ellipse::write(QJsonObject& obj)
{
  MShape::write(obj);

  // Leave out the center control point.
  QJsonArray verticesArray = obj["vertices"].toArray();
  while (verticesArray.size() > N_DEFINING_VERTICES)
    verticesArray.removeLast();
  obj["vertices"] = verticesArray;
}

// Returns true iff vertex index is considered a major (external) control point.
bool Ellipse::isMajorVertex(int idx) const
{
  return !hasCenterControl() || idx != 4;
}

}
