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

namespace mmp {

void Ellipse::sanitize()
{
  // Get horizontal axis rotated 90 degrees CW
  QVector2D hAxis = getHorizontalAxis();
  const QVector2D center(getCenter());
  QVector2D hAxisRotated(hAxis.y(), -hAxis.x());

  // Project vertex 1 onto it.
  QVector2D vAxisNormalized = hAxisRotated.normalized();

  QVector2D vFromCenter = QVector2D(getVertex(1)) - center;
  const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;
  MShape::setVertex(1, (center + projection).toPointF());
  MShape::setVertex(3, (center - projection).toPointF());

  if (hasCenterControl())
  {
    // Clip control point.
    MShape::setVertex(4, clipInside(getVertex(4)));
  }
}

QPointF Ellipse::clipInside(const QPointF& v) const
{
  // Map point as vector on a unit circle.
  QVector2D vector(toUnitCircle().map(v));

  // Clip control point.
  return (vector.length() <= 1 ?
                             v :
                             fromUnitCircle().map(vector.normalized().toPointF()));

}

QTransform Ellipse::toUnitCircle() const
{
  const QPointF& center = getCenter();
  return QTransform().scale(1.0/getHorizontalRadius(), 1.0/getVerticalRadius())
                     .rotateRadians(-getRotationRadians())
                     .translate(-center.x(), -center.y());
}

QTransform Ellipse::fromUnitCircle() const
{
  return toUnitCircle().inverted();
}

bool Ellipse::includesPoint(qreal x, qreal y)
{
  return (QVector2D(toUnitCircle().map(QPointF(x, y))).length() <= 1);
}

void Ellipse::setVertex(int i, const QPointF& v)
{
  // Save vertical axis vector.
  const QVector2D& vAxis  = getVerticalAxis();

  // If changed one of the two rotation-controlling points, adjust the other two points.
  if (i == 0 || i == 2)
  {
    // Transformation ellipse_t --> circle.
    QTransform transform = toUnitCircle();

    // Change the vertex.
    _rawSetVertex(i, v);

    // Combine with transformation circle -> ellipse_{t+1}.
    transform *= fromUnitCircle();

    // Set vertices.
    MShape::setVertex(1, transform.map( getVertex(1) ));
    MShape::setVertex(3, transform.map( getVertex(3) ));
    if (hasCenterControl())
      MShape::setVertex(4, transform.map( getVertex(4) ));
  }

  // If changed one of the two other points, just change the vertical axis.
  else if (i == 1 || i == 3)
  {
    // Retrieve the new horizontal axis vector and center.
    const QVector2D center(getCenter());

    QVector2D vFromCenter = QVector2D(v) - center;

    // Find projection of v onto vAxis / 2.
    QVector2D vAxisNormalized = vAxis.normalized();
    const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;

    // Assign vertical control points.
    QPointF v1;
    QPointF v3;
    if (i == 1)
    {
      v1 = (center + projection).toPointF();
      v3 = (center - projection).toPointF();
    }
    else
    {
      v1 = (center - projection).toPointF();
      v3 = (center + projection).toPointF();
    }

    // Transformation ellipse_t --> circle.
    QTransform transform = toUnitCircle();

    // Change vertical points.
    _rawSetVertex(1, v1);
    _rawSetVertex(3, v3);

    // Combine with transformation circle -> ellipse_{t+1}.
    transform *= fromUnitCircle();

    // Set vertices.
    if (hasCenterControl())
      _rawSetVertex(4, transform.map( getVertex(4) ));
  }

  // Center control point (make sure it stays inside!).
  else if (hasCenterControl())
  {
    // Clip control point.
    _rawSetVertex(4, clipInside(v));
  }

  // Just to be sure.
  sanitize();
}

// Returns true iff vertex index is considered a major (external) control point.
bool Ellipse::isMajorVertex(int idx) const
{
  return !hasCenterControl() || idx != 4;
}

}
