/*
 * Shape.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef SHAPE_H_
#define SHAPE_H_

#include <tr1/memory>
#include <iostream>
#include <cmath>

#include <QtGlobal>

#include <QObject>
#include <QTransform>
#include <QPointF>
#include <QVector2D>
#include <QPolygonF>

#include <QVector>
#include <QMap>
#include <QString>
#include <QMetaType>

#include "Maths.h"

/**
 * Shape represented by a series of control points.
 */
class Shape
{
public:
  typedef std::tr1::shared_ptr<Shape> ptr;

  Shape() {}
  Shape(QVector<QPointF> vertices_) :
    vertices(vertices_)
  {}
  virtual ~Shape() {}

  virtual void build() {}

  int nVertices() const { return vertices.size(); }

  QPointF getVertex(int i) const
  {
    return vertices[i];
  }

  virtual void setVertex(int i, const QPointF& v)
  {
    _rawSetVertex(i, v);
  }

  virtual void setVertex(int i, qreal x, qreal y)
  {
    setVertex(i, QPointF(x, y));
  }

  virtual QString getType() const = 0;

  /** Return true if Shape includes point (x,y), false otherwise
   *  Algorithm should work for all polygons, including non-convex
   *  Found at http://www.cs.tufts.edu/comp/163/notes05/point_inclusion_handout.pdf
   */
  virtual bool includesPoint(qreal x, qreal y) {
    return includesPoint(QPoint(x, y));
  }

  virtual bool includesPoint(const QPointF& p) = 0;

  /// Translate all vertices of shape by the vector (x,y).
  virtual void translate(int x, int y);

protected:
  QVector<QPointF> vertices;

  void _addVertex(const QPointF& vertex)
  {
    vertices.push_back(vertex);
  }

  void _rawSetVertex(int i, const QPointF& v)
  {
    vertices[i] = v;
  }

};

/**
 * This class represents a simple polygon (ie. the control points are vertices).
 */
class Polygon : public Shape {
public:
  Polygon() {}
  Polygon(QVector<QPointF> vertices_) : Shape(vertices_) {}
  virtual ~Polygon() {}

  virtual QPolygonF toPolygon() const;

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
};

/**
 * Four-vertex shape.
 */
class Quad : public Polygon
{
public:
  Quad() {}
  Quad(QPointF p1, QPointF p2, QPointF p3, QPointF p4)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
  }
  virtual ~Quad() {}

  virtual QString getType() const { return "quad"; }
};

/**
 * Triangle shape.
 */
class Triangle : public Polygon
{
public:
  Triangle() {}
  Triangle(QPointF p1, QPointF p2, QPointF p3)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
  }
  virtual ~Triangle() {}
  virtual QString getType() const { return "triangle"; }
};

class Mesh : public Quad
{
  typedef QVector<QVector<int> > IndexVector2d;

public:
  Mesh();

  // This constructor creates a quad mesh (four corners) using the same order as for the quad
  // constructor (ie. clockwise).
  Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4);

  // Standard mesh constructor.
  Mesh(const QVector<QPointF>& points, int nColumns, int nRows);

  virtual ~Mesh() {}

  // Performs the actual adding of points (used for loading).
  void init(const QVector<QPointF>& points, int nColumns, int nRows);

  virtual QString getType() const { return "mesh"; }

  /// Returns a polygon that is formed by all the contour points of the mesh.
  virtual QPolygonF toPolygon() const;

  // Override the parent, checking to make sure the vertices are displaced correctly.
  virtual void setVertex(int i, const QPointF& v);

  QPointF getVertex2d(int i, int j) const
  {
    return vertices[_vertices2d[i][j]];
  }

  void setVertex2d(int i, int j, const QPointF& v)
  {
    vertices[_vertices2d[i][j]] = v; // copy
  }

  void setVertex2d(int i, int j, double x, double y)
  {
    vertices[_vertices2d[i][j]] = QPointF(x, y);
  }

  void resizeVertices2d(IndexVector2d& vertices2d, int nColumns, int nRows);

  //
  void addColumn();
  void addRow();

  void resize(int nColumns_, int nRows_);

//  void removeColumn(int columnId);

  QVector<Quad> getQuads() const;
  QVector<QVector<Quad> > getQuads2d() const;

  int nColumns() const { return _nColumns; }
  int nRows() const  { return _nRows; }

  int nHorizontalQuads() const { return _nColumns-1; }
  int nVerticalQuads() const { return _nRows-1; }

protected:
  int _nColumns;
  int _nRows;
  // _vertices[i][j] contains vertex id of vertex at position (i,j) where i = 0..nColumns and j = 0..nRows
  IndexVector2d _vertices2d;

  /**
   * Reorder vertices in a standard order:
   *
   * 0----1----2----3
   * |    |    |    |
   * 4----5----6----7
   * |    |    |    |
   * 8----9---10----11
   */
  void _reorderVertices();
};

class Ellipse : public Shape {
public:
  Ellipse() {}
  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, QPointF p5)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    _addVertex(p5);
    sanitize();
  }

  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4, bool hasCenterControl=true)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    if (hasCenterControl)
      _addVertex(getCenter());
    sanitize();
  }

  /// Remaps points so as to make sure this is a correct ellipse, keeping vertices 0 and 2 as
  /// reference for the horizzontal axis.
  void sanitize()
  {
    // Get horizontal axis rotated 90 degrees CW
    QVector2D hAxis = getHorizontalAxis();
    const QVector2D center(getCenter());
    QVector2D hAxisRotated(hAxis.y(), -hAxis.x());

    // Project vertex 1 onto it.
    QVector2D vAxisNormalized = hAxisRotated.normalized();

    QVector2D vFromCenter = QVector2D(getVertex(1)) - center;
    const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;
    Shape::setVertex(1, (center + projection).toPointF());
    Shape::setVertex(3, (center - projection).toPointF());

    if (hasCenterControl())
    {
      // Clip control point.
      Shape::setVertex(4, clipInside(getVertex(4)));
    }
  }

  virtual ~Ellipse() {}

  virtual QString getType() const { return "ellipse"; }

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
  QPointF clipInside(const QPointF& v) const
  {
    // Map point as vector on a unit circle.
    QVector2D vector(toUnitCircle().map(v));

    // Clip control point.
    return (vector.length() <= 1 ?
                               v :
                               fromUnitCircle().map(vector.normalized().toPointF()));

  }

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

//protected:
//  virtual void _vertexChanged(int i, Point* p=NULL) {
//    // Get horizontal and vertical axis length.
//    qreal hAxisLength = Point::dist(getVertex(0)->toPoint(), getVertex(2)->toPoint());
//    qreal vAxisLength = Point::dist(getVertex(1)->toPoint(), getVertex(3)->toPoint());
//  }
};

#endif /* SHAPE_H_ */
