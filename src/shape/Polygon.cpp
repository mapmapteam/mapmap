/*
 * Polygon.cpp
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

#include "Polygon.h"

namespace mmp {

void Polygon::setVertex(int i, const QPointF& v)
{
  // Constrain vertex.
  QPointF realV = v;
  _constrainVertex(toPolygon(), i, realV);
  // Really set the vertex.
  _rawSetVertex(i, realV);
}

void Polygon::_constrainVertex(const QPolygonF& polygon, int i, QPointF& v)
{
  // Nothing to do (eg. triangles).
  if (polygon.size() <= 3)
    return;

  // Save original vertex.
  QPointF originalV = polygon.at(i);

  // Line between original and new point (for later use during intersection check).
  QLineF  originalToNew(originalV, v);

  // Look at the two adjunct segments to vertex i and see if they
  // intersect with any non-adjacent segments.

  // Construct the list of segments (with the new candidate vertex).
  QVector<QLineF> segments = _getSegments(polygon);
  int prev = wrapAround(i - 1, segments.size());
  int next = wrapAround(i + 1, segments.size());
  segments[prev] = QLineF(polygon.at(prev), v);
  segments[i]    = QLineF(v, polygon.at(next));

  // We now stretch segments a little bit to cope with approximation errors.
  for (QVector<QLineF>::Iterator it = segments.begin(); it != segments.end(); ++it)
  {
    QLineF&   seg = *it;
    QPointF p1 = seg.p1();
    QPointF p2 = seg.p2();
    // Create small vector pointing in same direction as segment.
    QVector2D vec(p2 - p1);
    vec *= _CONSTRAIN_VERTEX_SEGMENT_ELONGATION / vec.length();
    QPointF diff = vec.toPointF();
    // Use it to elongate segment slightly.
    seg.setP1( p1 - diff);
    seg.setP2( p2 + diff);
  }

  // For each adjunct segment.
  for (int adj=0; adj<2; adj++)
  {
    int idx = wrapAround(i + adj - 1, segments.size());
    for (int j=0; j<segments.size(); j++)
    {
      // If the segment to compare to is valid (ie. if it is not
      // the segment itself nor an adjacent one) then check for
      // intersection.
      if (j != idx &&
          j != wrapAround(idx-1, segments.size()) &&
          j != wrapAround(idx+1, segments.size()))
      {
        QPointF intersection;
        if (segments[idx].intersect(segments[j], &intersection) == QLineF::BoundedIntersection ||
            originalToNew.intersect(segments[j], &intersection) == QLineF::BoundedIntersection)
        {
          // Rearrange segments with new position at intersection point.
          // Create small vector pointing in same direction as segment.
          QVector2D vec(intersection - originalV);
          vec *= _CONSTRAIN_VERTEX_INTERSECTION_PULLAWAY / vec.length();
          QPointF diff = vec.toPointF();
          v = intersection - diff;
          segments[prev] = QLineF(polygon.at(prev), v);
          segments[i]    = QLineF(v, polygon.at(next));
        }
      }
    }
  }
}

qreal Polygon::_CONSTRAIN_VERTEX_SEGMENT_ELONGATION    = 10.0;
qreal Polygon::_CONSTRAIN_VERTEX_INTERSECTION_PULLAWAY = 30.0;

QVector<QLineF> Polygon::_getSegments() const
{
  return _getSegments(toPolygon());
}

QVector<QLineF> Polygon::_getSegments(const QPolygonF& polygon)
{
  QVector<QLineF> segments;
  for (int i=0; i<polygon.size(); i++)
    segments.push_back(QLineF(polygon.at(i), polygon.at( (i+1) % polygon.size() )));
  return segments;
}

QPolygonF Polygon::toPolygon() const
{
  QPolygonF polygon;
  for (QVector<QPointF>::const_iterator it = vertices.begin() ;
      it != vertices.end(); ++it)
  {
    polygon.append(*it);
  }
	// First point and end point match.
//	polygon.append(vertices.first());
  return polygon;
}

void Polygon::fromPolygon(const QPolygonF& p)
{
	Q_ASSERT(p.size() == nVertices());
	for (int i=0; i<nVertices(); i++)
		vertices[i] = p[i];
	build();
}

}
