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

  typedef QVector<QVector<int> > IndexVector2d;

public:
  /// Fewest columns, or rows, of vertices of a mesh: its two edges.
  static const int MIN_MESH_DIVISIONS = 2;
  /// Most columns, or rows, of vertices of a mesh: adding them, and drawing the mesh, take
  /// time that grows with the square of their number.
  static const int MAX_MESH_DIVISIONS = 32;

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
    _vertexChanged(_vertices2d[i][j]);
  }

  void setVertex2d(int i, int j, double x, double y)
  {
    setVertex2d(i, j, QPointF(x, y));
  }

  void resizeVertices2d(IndexVector2d& vertices2d, int nColumns, int nRows);

  //
  void addColumn();
  void addRow();

  void removeColumn(int columnId);
  void removeRow(int rowId);

  /// Changes the number of columns and rows, kept within the bounds of the
  /// "nColumns" and "nRows" settings; notifies the settings that changed.
  void resize(int nColumns_, int nRows_);

  QVector<Quad::ptr> getQuads() const;
  QVector<QVector<Quad::ptr> > getQuads2d() const;

  int nColumns() const { return _nColumns; }
  int nRows() const  { return _nRows; }

  /// Changes the number of columns, kept within the bounds of the "nColumns" setting.
  void setNColumns(int nColumns_) {
    resize(nColumns_, nRows());
  }
  /// Changes the number of rows, kept within the bounds of the "nRows" setting.
  void setNRows(int nRows_) {
    resize(nColumns(), nRows_);
  }

  int nHorizontalQuads() const { return _nColumns-1; }
  int nVerticalQuads() const { return _nRows-1; }

  void copyFrom(const MShape& shape);

  /**
   * Reads a mesh as MShape::read() does, then makes it consistent. A grid larger than
   * MAX_MESH_DIVISIONS, from a project made before meshes had a maximum, is reduced to it
   * by keeping evenly spaced columns and rows of the file, its edges included. Vertices that
   * do not match the grid, in a damaged or hand-written file, are spread again over the
   * grid, within their bounding rectangle. Both cases warn.
   */
  virtual void read(const QJsonObject& obj);

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

  /// addColumn(), addRow(), removeColumn() and removeRow() without notifying.
  void _addColumn();
  void _addRow();
  void _removeColumn(int columnId);
  void _removeRow(int rowId);

  /// Call after the grid changed: notifies the vertices, and the number of columns
  /// and of rows when they differ from the previous ones. Does nothing while the tree
  /// does not exist.
  void _gridChanged(int previousColumns, int previousRows);

  /// Returns a new MShape (using default constructor).
  virtual MShape* _create() const { return new Mesh(); }

  /// Keeps evenly spaced columns and rows of vertices given on a grid of fileColumns x
  /// fileRows, at most MAX_MESH_DIVISIONS of each, as the new grid.
  void _reduceFrom(const QVector<QPointF>& fileVertices, int fileColumns, int fileRows);

  /// Places the vertices on a regular grid of the current size, within the bounding
  /// rectangle of the vertices.
  void _spreadOverGrid();

  /// Declares the settings of a mesh, after those of a shape: its numbers of columns and rows.
  void _declareSettings(SettingGroup* root) override;
};

}

#endif /* MESH_H_ */
