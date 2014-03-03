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

#include "Math.h"

/**
 * Point (or vertex) on the 2-D canvas.
 */

/**
 * Series of vertices. (points)
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
    vertices[i].setX(v.x());
    vertices[i].setY(v.y());
  }

  virtual void setVertex(int i, double x, double y)
  {
    setVertex(i, QPointF(x, y));
  }

  virtual QString getType() const = 0;

  /** Return true if Shape includes point (x,y), false otherwise
   *  Algorithm should work for all polygons, including non-convex
   *  Found at http://www.cs.tufts.edu/comp/163/notes05/point_inclusion_handout.pdf
   */
  virtual bool includesPoint(qreal x, qreal y);

  virtual bool includesPoint(const QPointF& p) {
    return includesPoint(p.x(), p.y());
  }

  /* Translate all vertices of shape by the vector (x,y) */
  virtual void translate(int x, int y);

  virtual QPolygonF toPolygon() const;

protected:
  QVector<QPointF> vertices;

  void _addVertex(const QPointF& vertex)
  {
    vertices.push_back(vertex);
  }

};

/**
 * Four-vertex shape.
 */
class Quad : public Shape
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
class Triangle : public Shape
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

class Mesh : public Quad {

  typedef QVector<QVector<int> > IndexVector2d;

public:
  Mesh() : _nColumns(0), _nRows(0) {
    init(1, 1);
  }
  Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4, int nColumns=2, int nRows=2);
  Mesh(const QVector<QPointF>& points, int nColumns, int nRows);
  virtual ~Mesh() {}

  virtual QString getType() const { return "mesh"; }

  virtual QPolygonF toPolygon() const;

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

  void init(int nColumns, int nRows);

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
  Ellipse(QPointF p1, QPointF p2, QPointF p3, QPointF p4)
  {
    _addVertex(p1);
    _addVertex(p2);
    _addVertex(p3);
    _addVertex(p4);
    _addVertex(getCenter()); // add a point in the center
  }
  virtual ~Ellipse() {}

  virtual QString getType() const { return "ellipse"; }

  qreal getRotationRadians() const {
    QVector2D hAxis = getHorizontalAxis();
    return atan2( hAxis.y(), hAxis.x() );
  }

  qreal getRotation() const {
    return radiansToDegrees( getRotationRadians() );
  }

//  QRect getBoundingRect() const {
//    return QRect(0,                                     getVerticalAxis().manhattanLength(),
//                 getHorizontalAxis().manhattanLength(), getVerticalAxis().manhattanLength());
//  }
//
  QPointF getCenter() const {
    return (QVector2D(getVertex(0)) - (getHorizontalAxis() / 2)).toPointF();
  }

  QVector2D getHorizontalAxis() const {
    return QVector2D(getVertex(0)) - QVector2D(getVertex(2));
  }

  QVector2D getVerticalAxis() const {
    return QVector2D(getVertex(1)) - QVector2D(getVertex(3));
  }

  qreal getHorizontalRadius() const {
    return getHorizontalAxis().length() / 2;
  }

  qreal getVerticalRadius() const {
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

  virtual bool includesPoint(const QPointF& p) {
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
