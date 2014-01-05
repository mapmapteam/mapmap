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

#include <QtGlobal>
#include <QPointF>
#include <vector>
#include <map>

#include <tr1/memory>
#include <QObject>
#include <QString>
#include <QPointF>
#include <QMetaType>
#include <iostream>
/**
 * Point (or vertex) on the 2-D canvas.
 */


Q_DECLARE_METATYPE (qreal)

class Point: public QObject, public QPointF
{
  Q_OBJECT

public:
  Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
  Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
  Q_INVOKABLE Point(qreal x, qreal y): QPointF(x,y) {}
  Q_INVOKABLE Point(): QPointF(0,0) {}

signals:
  void xChanged();
  void yChanged();

public slots:
  void setX(qreal x)
  {
    QPointF::setX(x);
    emit xChanged();
  }
  void setY(qreal y)
  {
    QPointF::setY(y);
    emit yChanged();
  }
  void setValue(const QPointF& p)
  {
    setX(p.x());
    setY(p.y());
  }

};
/**
 * Series of vertices. (points)
 */
class Shape
{
public:
  typedef std::tr1::shared_ptr<Shape> ptr;

  Shape() {}
  Shape(std::vector<Point*> vertices_) :
    vertices(vertices_)
  {}
  virtual ~Shape() {}

  virtual void build() {}

  int nVertices() const { return vertices.size(); }

  Point* getVertex(int i) const
  {
    return vertices[i];
  }

  void setVertex(int i, QPointF v)
  {
    vertices[i]->setValue(v);
  }

  void setVertex(int i, double x, double y)
  {
    vertices[i]->setX(x);
    vertices[i]->setY(y);
  }

  virtual const char * getShapeType() = 0;

  /** Return true if Shape includes point (x,y), false otherwise
   *  Algorithm should work for all polygons, including non-convex
   *  Found at http://www.cs.tufts.edu/comp/163/notes05/point_inclusion_handout.pdf
   */
  bool includesPoint(int x, int y);

  /* Translate all vertices of shape by the vector (x,y) */
  void translate(int x, int y);

  int nVertices()
  {
    return vertices.size();
  }

protected:
  std::vector<Point*> vertices;

  void _addVertex(const QPointF& vertex)
  {
    Point* v = new Point();
    v->setValue(vertex);

    vertices.push_back(v);
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

  virtual const char * getShapeType() { return "quad"; }
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
  virtual const char * getShapeType() { return "triangle"; }
};

class Mesh : public Quad {
public:
  Mesh() : _nColumns(0), _nRows(0) {
    init(1, 1);
  }
  Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4, int nColumns=2, int nRows=2);
  virtual ~Mesh() {}

  void resizeVertices2d(std::vector< std::vector<int> >& vertices2d, int nColumns, int nRows);

  void init(int nColumns, int nRows);

  /**
   * This is what _vertices2d looks like.
   *
   * 0----4----6----1
   * |    |    |    |
   * 8----9---10---11
   * |    |    |    |
   * 3----5----7----2
   */
  // vertices 0..3 = 4 corners
  //
  void addColumn();
  void addRow();

  void resize(int nColumns_, int nRows_);

//  void removeColumn(int columnId);

  std::vector<Quad> getQuads() const;
  std::vector< std::vector<Quad> > getQuads2d() const;

  int nColumns() const { return _nColumns; }
  int nRows() const  { return _nRows; }

  int nHorizontalQuads() const { return _nColumns-1; }
  int nVerticalQuads() const { return _nRows-1; }

protected:
  int _nColumns;
  int _nRows;
  // _vertices[i][j] contains vertex id of vertex at position (i,j) where i = 0..nColumns and j = 0..nRows
  std::vector< std::vector<int> > _vertices2d;
  // Maps a vertex id to the pair of vertex ids it "splits".
  std::map<int, std::pair<int, int> > _splitVertices;
};


#endif /* SHAPE_H_ */
