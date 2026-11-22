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

/**
 * Ellipse defined as the projective image of a unit circle.
 *
 * Vertices 0-3 are the images of the circle's quarter points (-1,0), (0,-1), (1,0), (0,1)
 * and fully define a homography from the unit circle to the ellipse. Vertex 4 (optional)
 * is the image of the circle's center, ie. the perspective-correct center of the ellipse:
 * it is derived from the other vertices and dragging it moves the whole shape.
 *
 * In perspective mode (default) vertices 0-3 can be moved freely, as long as the circle's
 * image remains an ellipse. Otherwise the shape is constrained to an axis-aligned ellipse,
 * with vertices 1-3 kept perpendicular to vertices 0-2.
 */
class Ellipse : public MShape
{
  Q_OBJECT
public:
  /// Maximum perspective strength allowed (0 = none, 1 = degenerate).
  static constexpr qreal MAX_PERSPECTIVE = 0.95;

  /// Default number of segments used to approximate the contour.
  static const int N_POLYGON_SEGMENTS = 100;

  Q_INVOKABLE Ellipse() : _perspective(true) {}
  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, QPointF p5)
    : _perspective(true)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    _addVertex(p5);
    build();
  }

  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, bool hasCenterControl=true)
    : _perspective(true)
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

  /// Makes sure this is a correct ellipse and updates the center control point.
  void sanitize();

  virtual void build() {
    sanitize();
  }

  virtual ShapeType getType() const { return ShapeType::Ellipse; }

  /// Returns true iff vertices 0-3 can be moved freely (perspective ellipse).
  bool isPerspective() const { return _perspective; }

  /// Enables or disables perspective mode (disabling it remaps the shape to an axis-aligned ellipse).
  void setPerspective(bool perspective);

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

  /// Returns the perspective-correct center (image of the unit circle's center).
  QPointF getCenter() const;

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

  /// Homography mapping the unit circle (centered at origin) to this ellipse.
  QTransform fromUnitCircle() const;

  /// Homography mapping this ellipse to the unit circle (centered at origin).
  QTransform toUnitCircle() const;

  /// Returns true iff vertices 0-3 define a valid ellipse.
  bool isValid() const;

  /// Returns the contour of the ellipse as a polygon.
  QPolygonF toPolygon(int nSegments=N_POLYGON_SEGMENTS) const;

  /// Return true if the ellipse includes point (x,y), false otherwise.
  virtual bool includesPoint(qreal x, qreal y);

  virtual bool includesPoint(const QPointF& p)
  {
    return includesPoint(p.x(), p.y());
  }

  // Override the parent, checking to make sure the vertices are displaced correctly.
  virtual void setVertex(int i, const QPointF& v);

  // Returns true iff vertex index is considered a major (external) control point.
  virtual bool isMajorVertex(int idx) const;

  virtual void copyFrom(const MShape& shape);

protected:
  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const { return new Ellipse(); }

  /// Computes the homography unit circle -> ellipse; returns false if degenerate.
  bool _computeFromUnitCircle(QTransform& transform) const;

  /// Returns true iff transform maps the unit circle to a (non-degenerate) ellipse.
  static bool _isValidTransform(const QTransform& transform);

  /// Center of the ellipse according to the horizontal axis (vertices 0 and 2).
  QPointF _axisCenter() const
  {
    return (QVector2D(getVertex(0)) - (getHorizontalAxis() / 2)).toPointF();
  }

  /// Affine version of toUnitCircle() based on the axes (used in non-perspective mode).
  QTransform _axisToUnitCircle() const;

  /// Remaps vertices 1 and 3 so as to make an axis-aligned ellipse.
  void _sanitizeAxes();

  /// Sets vertex in non-perspective mode.
  void _setAxisVertex(int i, const QPointF& v);

  /// Sets vertex in perspective mode, clamping it to remain a valid ellipse.
  void _setPerspectiveVertex(int i, const QPointF& v);

  bool _perspective;
};

}

#endif /* ELLIPSE_H_ */
