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

#ifndef M_SHAPE_H_
#define M_SHAPE_H_

#include <iostream>

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
#include <QTransform>

#include <QSharedPointer>

#include "Maths.h"
#include "Serializable.h"

namespace mmp {

/**
 * Shape represented by a series of control points.
 */
class MShape : public Serializable
{
  Q_OBJECT

  Q_PROPERTY(bool    locked  READ isLocked   WRITE setLocked)
  Q_PROPERTY(QVector<QPointF> vertices READ getVertices WRITE setVertices STORED false)
public:

  // Enumerations
  enum ShapeMode {
    DefaultMode,
    ScaleMode,
    RotateMode
  };

  enum FlipDirection {
    Horizontal,
    Vertical
  };

  enum Rotation {
    Rotate90CW,
    Rotate90CCW,
    Rotate180
  };

  enum ShapeType {
    Mesh, Triangle, Ellipse, Quad
  };

  typedef QSharedPointer<MShape> ptr;

  MShape() : _isLocked(false) {}
  MShape(const QVector<QPointF>& vertices_);
  virtual ~MShape() {}

  /**
   * This method should be called after vertices and other properties have been set
   * to compute any other information needed by the object and possibly do some sanitizing.
   */
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

  virtual void applyTransform(const QTransform& transform);

  virtual ShapeType getType() const = 0;

  /** Return true if Shape includes point (x,y), false otherwise
   *  Algorithm should work for all polygons, including non-convex
   *  Found at http://www.cs.tufts.edu/comp/163/notes05/point_inclusion_handout.pdf
   */
  virtual bool includesPoint(qreal x, qreal y) {
    return includesPoint(QPoint(x, y));
  }

  virtual bool includesPoint(const QPointF& p) = 0;

  virtual void transform(const QPointF& translate, qreal scale, qreal rotate);

  /// Translate all vertices of shape by the vector (x,y).
  virtual void translate(const QPointF& offset);


	virtual void rotate(qreal angle);
	virtual void scale(qreal scaling);

	virtual void copyFrom(const MShape& shape);

	virtual MShape* clone() const;

  bool isLocked() const    { return _isLocked; }
  void setLocked(bool locked)    { _isLocked = locked; }
  void toggleLocked()  { _isLocked = !_isLocked; }

  MShape::ShapeMode shapeModeState() const { return _shapeMode; }
  void setShapeMode(MShape::ShapeMode mode, bool next = false);

  const QVector<QPointF>& getVertices() const { return vertices; }
  virtual void setVertices(const QVector<QPointF>& vertices_)
  {
    // Deep copy.
    vertices.resize(vertices_.size());
    qCopy(vertices_.begin(), vertices_.end(), vertices.begin());
  }

  // Returns true iff vertex index is considered a major (external) control point.
  virtual bool isMajorVertex(int idx) const { Q_UNUSED(idx); return true; }

	// Returns center of object.
	virtual QPointF getCenter() const;

  virtual void read(const QDomElement& obj);
  virtual void write(QDomElement& obj);

protected:
  QVector<QPointF> vertices;
  bool _isLocked;

  void _addVertex(const QPointF& vertex)
  {
    vertices.push_back(vertex);
  }

  void _rawSetVertex(int i, const QPointF& v)
  {
    vertices[i] = v;
  }

  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const = 0;

  // Lists QProperties that should NOT be parsed automatically.
  virtual QList<QString> _propertiesSpecial() const { return Serializable::_propertiesSpecial() << "vertices"; }

private:
  MShape::ShapeMode _shapeMode = MShape::DefaultMode;
};


}

#endif /* SHAPE_H_ */
