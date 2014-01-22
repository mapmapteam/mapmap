/*
 * Shape.cpp
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

#include "Shape.h"

bool Shape::includesPoint(int x, int y)
{
  Point *prev = NULL, *cur;
  int left = 0, right = 0, maxy, miny;
  for (std::vector<Point*>::iterator it = vertices.begin() ;
      it != vertices.end(); it++)
  {
    if (!prev) {
      prev = vertices.back();
    }
    cur = *it;
    miny = std::min(cur->y(), prev->y());
    maxy = std::max(cur->y(), prev->y());

    if (y > miny && y < maxy) {
      if (prev->x() == cur->x())
      {
        if (x < cur->x())
          right++;
        else left++;
      }
      else
      {
        double slope = (cur->y() - prev->y()) / (cur->x() - prev->x());
        double offset = cur->y() - slope * cur->x();
        int xintersect = int((y - offset ) / slope);
        if (x < xintersect)
          right++;
        else left++;
      }
    }
    prev = *it;
  }
  if (right % 2 && left % 2)
    return true;
  return false;
}


void Shape::translate(int x, int y)
{
  for (std::vector<Point*>::iterator it = vertices.begin() ;
      it != vertices.end(); ++it)
  {
    (*it)->setX((*it)->x() + x);
    (*it)->setY((*it)->y() + y);
  }
}

QPolygonF Shape::toPolygon() const
{
  QPolygonF polygon;
  for (std::vector<Point*>::const_iterator it = vertices.begin() ;
      it != vertices.end(); ++it)
  {
    polygon.append((*it)->toPoint());
  }
  return polygon;
}


Mesh::Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4, int nColumns, int nRows)
: Quad(p1, p2, p3, p4), _nColumns(0), _nRows(0)
{
  Q_ASSERT(nColumns >= 2 && nRows >= 2);
  init(nColumns, nRows);
}

Mesh::Mesh(const QList<QPointF>& points, int nColumns, int nRows)
: Quad(), _nColumns(nColumns), _nRows(nRows)
{
  Q_ASSERT(nColumns >= 2 && nRows >= 2);
  Q_ASSERT(points.size() == nColumns * nRows);

  // Resize the vertices2d vector to appropriate dimensions.
  resizeVertices2d(_vertices2d, _nColumns, _nRows);

  // Just build vertices2d in the standard order.
  int k = 0;
  for (int x=0; x<_nColumns; x++)
    for (int y=0; y<_nRows; y++)
    {
      vertices.push_back( new Point(points[k].x(), points[k].y()) );
      _vertices2d[x][y] = k;
      k++;
    }
}

QPolygonF Mesh::toPolygon() const
{
  QPolygonF polygon;
  polygon.append(getVertex2d(0,            0)->toPoint());
  polygon.append(getVertex2d(nColumns()-1, 0)->toPoint());
  polygon.append(getVertex2d(nColumns()-1, nRows()-1)->toPoint());
  polygon.append(getVertex2d(0,            nRows()-1)->toPoint());
  return polygon;
}

void Mesh::resizeVertices2d(std::vector< std::vector<int> >& vertices2d, int nColumns, int nRows)
{
  vertices2d.resize(nColumns);
  for (int i=0; i<nColumns; i++)
    vertices2d[i].resize(nRows);
}

void Mesh::init(int nColumns, int nRows)
{
  // Create vertices correspondence of bouding quad.
  resizeVertices2d(_vertices2d, 2, 2);
  _vertices2d[0][0] = 0;
  _vertices2d[1][0] = 1;
  _vertices2d[1][1] = 2;
  _vertices2d[0][1] = 3;

  // Init number of columns and rows.
  _nColumns = _nRows = 2;

  // Add extra columns and rows.
  for (int i=0; i<nColumns-2; i++)
    addColumn();
  for (int i=0; i<nRows-2; i++)
    addRow();
}

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
void Mesh::addColumn()
{
  // Create new vertices 2d (temporary).
  std::vector< std::vector<int> > newVertices2d;
  resizeVertices2d(newVertices2d, nColumns()+1, nRows());

  // Left displacement of points already there.
  float leftMoveProp = 1.0f/(nColumns()-1) - 1.0f/nColumns();

  // Add a point at each row.
  int k = nVertices();
  for (int y=0; y<nRows(); y++)
  {
    // Get left and right vertices.
    QPointF left  = *getVertex( _vertices2d[0]           [y] );
    QPointF right = *getVertex( _vertices2d[nColumns()-1][y] );
    QPointF diff = right - left;

    // First pass: move middle points.
    for (int x=1; x<nColumns()-1; x++)
    {
      QPointF p = *getVertex( _vertices2d[x][y] );
      p -= diff * x * leftMoveProp;
      setVertex( _vertices2d[x][y], p );
    }

    // Create and add new point.
    QPointF newPoint = right - diff * 1.0f/nColumns();
    _addVertex(newPoint);

    // Assign new vertices 2d.
    for (int x=0; x<nColumns()-1; x++)
      newVertices2d[x][y] = _vertices2d[x][y];

    // The new point.
    newVertices2d[nColumns()-1][y] = k;

    // The rightmost point.
    newVertices2d[nColumns()][y]   = _vertices2d[nColumns()-1][y];

    k++;
  }

  // Copy new mapping.
  _vertices2d = newVertices2d;

  // Increment number of columns.
  _nColumns++;

  // Reorder.
  _reorderVertices();
}

void Mesh::addRow()
{
  // Create new vertices 2d (temporary).
  std::vector< std::vector<int> > newVertices2d;
  resizeVertices2d(newVertices2d, nColumns(), nRows()+1);

  // Top displacement of points already there.
  float topMoveProp = 1.0f/(nRows()-1) - 1.0f/nRows();

  // Add a point at each row.
  int k = nVertices();
  for (int x=0; x<nColumns(); x++)
  {
    // Get left and right vertices.
    QPointF top    = *getVertex( _vertices2d[x][0] );
    QPointF bottom = *getVertex( _vertices2d[x][nRows()-1] );
    QPointF diff = bottom - top;

    // First pass: move middle points.
    for (int y=1; y<nRows()-1; y++)
    {
      QPointF p = *getVertex( _vertices2d[x][y] );
      p -= diff * y * topMoveProp;
      setVertex( _vertices2d[x][y], p );
    }

    // Create and add new point.
    QPointF newPoint = bottom - diff * 1.0f/nRows();
    _addVertex(newPoint);

    // Assign new vertices 2d.
    for (int y=0; y<nRows()-1; y++)
      newVertices2d[x][y] = _vertices2d[x][y];

    // The new point.
    newVertices2d[x][nRows()-1] = k;

    // The rightmost point.
    newVertices2d[x][nRows()]   = _vertices2d[x][nRows()-1];

    k++;
  }

  // Copy new mapping.
  _vertices2d = newVertices2d;

  // Increment number of columns.
  _nRows++;

  // Reorder.
  _reorderVertices();
}

void Mesh::resize(int nColumns_, int nRows_)
{
  Q_ASSERT(nColumns_ >= nColumns() && nRows_ >= nRows());
  while (nColumns_ != nColumns())
    addColumn();
  while (nRows_ != nRows())
    addRow();
}

//  void removeColumn(int columnId)
//  {
//    // Cannot remove first and last columns
//    Q_ASSERT(columnId >= 1 && columnId < nColumns()-1);
//
//    std::vector< std::vector<int> > newVertices2d;
//    resizeVertices2d(newVertices2d, nColumns()-1, nRows());
//
//    // Right displacement of points already there.
//    float rightMoveProp = 1.0f/(nColumns()-2) - 1.0f/(nColumns()-1);
//
//    // Remove a point at each row.
//    int k = nVertices();
//    for (int y=0; y<nRows(); y++)
//    {
//      // Get left and right vertices.
//      QPointF left  = getVertex( _vertices2d[0]           [y] ).toQPointF();
//      QPointF right = getVertex( _vertices2d[nColumns()-1][y] ).toQPointF();
//      QPointF diff = right - left;
//
//      // First pass: move middle points.
//      for (int x=1; x<nColumns()-1; x++)
//      {
//        QPointF p = getVertex( _vertices2d[x][y] ).toQPointF();
//        p -= diff * x * leftMoveProp;
//        setVertex( _vertices2d[x][y], p );
//      }
//
//      // Create and add new point.
//      QPointF newPoint = right - diff * 1.0f/nColumns();
//      vertices.push_back(newPoint);
//
//      // Assign new vertices 2d.
//      for (int x=0; x<nColumns()-1; x++)
//        newVertices2d[x][y] = _vertices2d[x][y];
//
//      // The new point.
//      newVertices2d[nColumns()-1][y] = k;
//
//      // The rightmost point.
//      newVertices2d[nColumns()][y]   = _vertices2d[nColumns()-1][y];
//
//      k++;
//    }
//
//    // Copy new mapping.
//    _vertices2d = newVertices2d;
//
//    // Increment number of columns.
//    _nColumns++;
//
//  }

std::vector<Quad> Mesh::getQuads() const
{
  std::vector<Quad> quads;
  for (int i=0; i<nHorizontalQuads(); i++)
  {
    for (int j=0; j<nVerticalQuads(); j++)
    {
      Quad quad(
          *vertices[ _vertices2d[i]  [j]  ],
          *vertices[ _vertices2d[i+1][j]   ],
          *vertices[ _vertices2d[i+1][j+1] ],
          *vertices[ _vertices2d[i]  [j+1] ]
      );
      quads.push_back(quad);
    }
  }

  return quads;
}

std::vector< std::vector<Quad> > Mesh::getQuads2d() const
{
  std::vector< std::vector<Quad> > quads2d;
  for (int i=0; i<nHorizontalQuads(); i++)
  {
    std::vector<Quad> column;
    for (int j=0; j<nVerticalQuads(); j++)
    {
      Quad quad(
          *vertices[ _vertices2d[i]  [j]  ],
          *vertices[ _vertices2d[i+1][j]   ],
          *vertices[ _vertices2d[i+1][j+1] ],
          *vertices[ _vertices2d[i]  [j+1] ]
      );
      column.push_back(quad);
    }
    quads2d.push_back(column);
  }
  return quads2d;
}

void Mesh::_reorderVertices()
{
  // Populate new vertices vector.
  std::vector<Point*> newVertices(vertices.size());
  int k = 0;
  for (int x=0; x<nColumns(); x++)
    for (int y=0; y<nRows(); y++)
      newVertices[k++] = getVertex( _vertices2d[x][y] );

  // Populate _vertices2d.
  k = 0;
  for (int x=0; x<nColumns(); x++)
    for (int y=0; y<nRows(); y++)
      _vertices2d[x][y] = k++;

  // Copy.
  vertices = newVertices;
}
