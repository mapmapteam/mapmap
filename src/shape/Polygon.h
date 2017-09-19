/*
 * Polygon.h
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
#ifndef POLYGON_H_
#define POLYGON_H_

#include "Shape.h"

namespace mmp {

/**
 * This class represents a simple polygon (ie. the control points are vertices).
 */
class Polygon : public MShape
{
  Q_OBJECT
public:
  Polygon() {}
  Polygon(QVector<QPointF> vertices_) : MShape(vertices_) {}
  virtual ~Polygon() {}

  virtual QPolygonF toPolygon() const;
	virtual void fromPolygon(const QPolygonF& p);

  virtual bool includesPoint(const QPointF& p) {
    return toPolygon().containsPoint(p, Qt::OddEvenFill);
  }

  // Override the parent, checking to make sure the vertices are displaced correctly.
  virtual void setVertex(int i, const QPointF& v);

protected:
  /// Returns all line segments of the polygon.
  QVector<QLineF> _getSegments() const;

  /// Returns all line segments of a polygon.
  static QVector<QLineF> _getSegments(const QPolygonF& polygon);

  /// Makes sure vertex v as the i-th point of polygon stays inside the polygon.
  static void _constrainVertex(const QPolygonF& polygon, int i, QPointF& v);

  // Parameters used to constrain vertex.
  static qreal _CONSTRAIN_VERTEX_SEGMENT_ELONGATION;
  static qreal _CONSTRAIN_VERTEX_INTERSECTION_PULLAWAY;
};

}

#endif /* POLYGON_H_ */
