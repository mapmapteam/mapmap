/*
 * Mesh.h
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


#ifndef MESH_H_
#define MESH_H_

#include "Quad.h"

namespace mmp {

class Mesh : public Quad
{
  Q_OBJECT

  Q_PROPERTY(int nColumns READ nColumns WRITE setNColumns)
  Q_PROPERTY(int nRows    READ nRows    WRITE setNRows)

  typedef QVector<QVector<int> > IndexVector2d;

public:
  Q_INVOKABLE Mesh();

  // This constructor creates a quad mesh (four corners) using the same order as for the quad
  // constructor (ie. clockwise).
  Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4);

  // Standard mesh constructor.
  Mesh(const QVector<QPointF>& points, int nColumns, int nRows);

  virtual ~Mesh() {}

  // Performs the actual adding of points (used for loading).
  void init(const QVector<QPointF>& points, int nColumns, int nRows);

  virtual void build();

  virtual ShapeType getType() const { return ShapeType::Mesh; }

  /// Returns a polygon that is formed by all the contour points of the mesh.
  virtual QPolygonF toPolygon() const;

  // Override the parent, checking to make sure the vertices are displaced correctly.
  virtual void setVertex(int i, const QPointF& v);

  // Returns true iff vertex index is considered a major (external) control point.
  virtual bool isMajorVertex(int idx) const;

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

  void removeColumn(int columnId);
  void removeRow(int rowId);

  void resize(int nColumns_, int nRows_);

  QVector<Quad::ptr> getQuads() const;
  QVector<QVector<Quad::ptr> > getQuads2d() const;

  int nColumns() const { return _nColumns; }
  int nRows() const  { return _nRows; }

  void setNColumns(int nColumns_) {
    resize(nColumns_, nRows());
  }
  void setNRows(int nRows_) {
    resize(nColumns(), nRows_);
  }

  int nHorizontalQuads() const { return _nColumns-1; }
  int nVerticalQuads() const { return _nRows-1; }

  void copyFrom(const MShape& shape);

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

  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const { return new Mesh(); }
};

}

#endif /* MESH_H_ */
