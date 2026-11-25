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

#include <algorithm>
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

public:

  // Enumerations
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

  /// Creates a shape without vertices and declares its settings.
  MShape();
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

  /// Moves vertex i; notifies the setting of that vertex. Subclasses that constrain the
  /// vertex do the same once they have moved it.
  virtual void setVertex(int i, const QPointF& v)
  {
    _rawSetVertex(i, v);
    _vertexChanged(i);
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

  /// Returns the bounding rectangle of all vertices.
  QRectF getBoundingRect() const;

  /**
   * Returns the transform that flips this shape in place.
   *
   * When keepBounds is false, the flip is done about getCenter(). When it is
   * true, it is done about the center of the bounding rectangle, so that the
   * outline of an axis-aligned rectangle stays exactly where it was.
   */
  QTransform flipTransform(FlipDirection direction, bool keepBounds = false) const;

  /**
   * Returns the transform that rotates this shape by a quarter or half turn
   * (clockwise on screen, where y points down).
   *
   * When keepBounds is false, the rotation is done about getCenter() and the
   * outline turns with it. When it is true, the rotated shape is stretched
   * back into its original bounding rectangle: an axis-aligned rectangle
   * keeps its outline and only the order of its vertices changes.
   */
  QTransform rotateTransform(Rotation rotation, bool keepBounds = false) const;

  /**
   * Transforms that turn the image shown inside a layer, by moving its input
   * shape (the region picked in the source) while the output shape stays put.
   *
   * Moving the picked region one way turns the image the other way, so the
   * rotation is inverted: imageRotateTransform(Rotate90CW) makes the image
   * appear turned clockwise in the output.
   */
  QTransform imageFlipTransform(FlipDirection direction) const;
  QTransform imageRotateTransform(Rotation rotation) const;

  /// Returns the rotation that cancels the given one.
  static Rotation inverseRotation(Rotation rotation);

	virtual void copyFrom(const MShape& shape);

	virtual MShape* clone() const;

  bool isLocked() const    { return _isLocked; }
  /// Locks or unlocks the shape; notifies the "locked" setting when it changes.
  void setLocked(bool locked);
  void toggleLocked()  { setLocked(!_isLocked); }

  const QVector<QPointF>& getVertices() const { return vertices; }
  /// Replaces all the vertices; notifies the "vertices" list.
  virtual void setVertices(const QVector<QPointF>& vertices_);

  // Returns true iff vertex index is considered a major (external) control point.
  virtual bool isMajorVertex(int idx) const { Q_UNUSED(idx); return true; }

	// Returns center of object.
	virtual QPointF getCenter() const;

  virtual void read(const QJsonObject& obj);
  virtual void write(QJsonObject& obj);

protected:
  QVector<QPointF> vertices;
  bool _isLocked;

  /// Appends a vertex without notifying: call _verticesChanged() once done.
  void _addVertex(const QPointF& vertex)
  {
    vertices.push_back(vertex);
  }

  /// Moves a vertex without notifying.
  void _rawSetVertex(int i, const QPointF& v)
  {
    vertices[i] = v;
  }

  /// Call after the vertices changed in any way: follows their number in the "vertices"
  /// list, then notifies the whole list once.
  void _verticesChanged();

  /// Call after vertex i alone moved: notifies its setting only.
  void _vertexChanged(int i);

  /// Call after an operation that moved some vertices: notifies those that differ from
  /// before, or the whole list when their number changed.
  void _verticesMoved(const QVector<QPointF>& before);

  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const = 0;

  /// The vertices are read and written by MShape::read() and MShape::write().
  QStringList _reservedKeys() const override { return Serializable::_reservedKeys() << "vertices"; }

private:
  /// Declares the settings of a shape: its lock and its vertices.
  void _declareSettings();

  /// The settings of the vertices, which stay in the vector of vertices.
  SettingList* _verticesSettings;
};


}

#endif /* SHAPE_H_ */
