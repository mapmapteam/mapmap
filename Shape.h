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

#include <vector>
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
};
/**
 * Series of vertices. (points)
 */
class Shape
{
public:
  typedef std::tr1::shared_ptr<Shape> ptr;
  std::vector<Point*> vertices;
  Shape() {}
  Shape(std::vector<Point*> vertices_) :
    vertices(vertices_)
  {}
  virtual ~Shape() {}

  virtual void build() {}

  int nVertices() const { return vertices.size(); }

  Point* getVertex(int i)
  {
    return vertices[i];
  }
  void setVertex(int i, Point *v)
  {
    vertices[i] = v;
  }
  void setVertex(int i, double x, double y)
  {
    vertices[i]->setX(x);
    vertices[i]->setY(y);
  }
  virtual const char * getShapeType() = 0;
};

/**
 * Four-vertex shape.
 */
class Quad : public Shape
{
public:
  Quad() {}
  Quad(Point *p1, Point *p2, Point *p3, Point *p4)
  {
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);
    vertices.push_back(p4);
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
  Triangle(Point *p1, Point *p2, Point *p3)
  {
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);
  }
  virtual ~Triangle() {}
  virtual const char * getShapeType() { return "triangle"; }
};

#endif /* SHAPE_H_ */
