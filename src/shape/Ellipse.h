/*
 * Ellipse.h
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


#ifndef ELLIPSE_H_
#define ELLIPSE_H_

#include "Shape.h"

namespace mmp {

class Ellipse : public MShape
{
  Q_OBJECT
public:
  Q_INVOKABLE Ellipse() {}
  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, QPointF p5)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    _addVertex(p5);
    build();
  }

  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, bool hasCenterControl=true)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    if (hasCenterControl)
      _addVertex(getCenter());
    build();
  }

  virtual ~Ellipse() {}

  /// Remaps points so as to make sure this is a correct ellipse, keeping vertices 0 and 2 as
  /// reference for the horizzontal axis.
  void sanitize();

  virtual void build() {
    sanitize();
  }

  virtual ShapeType getType() const { return ShapeType::Ellipse; }

  qreal getRotationRadians() const
  {
    QVector2D hAxis = getHorizontalAxis();
    return atan2( hAxis.y(), hAxis.x() );
  }

  qreal getRotation() const
  {
    return radiansToDegrees( getRotationRadians() );
  }

  bool hasCenterControl() const
  {
    return (nVertices() == 5);
  }

  /// If v is outside boundaries, remap it to the border.
  QPointF clipInside(const QPointF& v) const;

//  QRect getBoundingRect() const {
//    return QRect(0,                                     getVerticalAxis().manhattanLength(),
//                 getHorizontalAxis().manhattanLength(), getVerticalAxis().manhattanLength());
//  }
//
  QPointF getCenter() const
  {
    return (QVector2D(getVertex(0)) - (getHorizontalAxis() / 2)).toPointF();
  }

  QVector2D getHorizontalAxis() const
  {
    return QVector2D(getVertex(0)) - QVector2D(getVertex(2));
  }

  QVector2D getVerticalAxis() const
  {
    return QVector2D(getVertex(1)) - QVector2D(getVertex(3));
  }

  qreal getHorizontalRadius() const
  {
    return getHorizontalAxis().length() / 2;
  }

  qreal getVerticalRadius() const
  {
    return getVerticalAxis().length() / 2;
  }

  /// Remaps point from ellipse to a circle with radius 1 set at origin (0,0).
  QTransform toUnitCircle() const;

  /// Remaps point from circle with radius 1 set at origin (0,0) to ellipse coordinates.
  QTransform fromUnitCircle() const;

  /** Return true if Shape includes point (x,y), false otherwise
   *  Algorithm should work for all polygons, including non-convex
   *  Found at http://www.cs.tufts.edu/comp/163/notes05/point_inclusion_handout.pdf
   */
  virtual bool includesPoint(qreal x, qreal y);

  virtual bool includesPoint(const QPointF& p)
  {
    return includesPoint(p.x(), p.y());
  }

  // Override the parent, checking to make sure the vertices are displaced correctly.
  virtual void setVertex(int i, const QPointF& v);

  // Returns true iff vertex index is considered a major (external) control point.
  virtual bool isMajorVertex(int idx) const;

protected:
  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const { return new Ellipse(); }

//protected:
//  virtual void _vertexChanged(int i, Point* p=NULL) {
//    // Get horizontal and vertical axis length.
//    qreal hAxisLength = Point::dist(getVertex(0)->toPoint(), getVertex(2)->toPoint());
//    qreal vAxisLength = Point::dist(getVertex(1)->toPoint(), getVertex(3)->toPoint());
//  }
};

}

#endif /* ELLIPSE_H_ */
