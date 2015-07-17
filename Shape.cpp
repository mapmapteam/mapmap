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

void MShape::translate(int x, int y)
{
  for (QVector<QPointF>::iterator it = vertices.begin() ;
      it != vertices.end(); ++it)
  {
    it->setX(it->x() + x);
    it->setY(it->y() + y);
  }
}

void Polygon::setVertex(int i, const QPointF& v)
{
  // Constrain vertex.
  QPointF realV = v;
  _constrainVertex(toPolygon(), i, realV);
  // Really set the vertex.
  _rawSetVertex(i, realV);
}

void Polygon::_constrainVertex(const QPolygonF& polygon, int i, QPointF& v)
{
  // Weird, but nothing to do.
  if (polygon.size() <= 3)
    return;

  // Save previous position of vertex.
  QPointF prevV = polygon.at(i);

  // Look at the two adjunct segments to vertex i and see if they
  // intersect with any non-adjacent segments.

  // Construct the list of segments (with the new candidate vertex).
  QVector<QLineF> segments = _getSegments(polygon);
  int prev = wrapAround(i - 1, segments.size());
  int next = wrapAround(i + 1, segments.size());
  segments[prev] = QLineF(polygon.at(prev), v);
  segments[i]    = QLineF(v, polygon.at(next));

  // We now stretch segments a little bit to cope with approximation errors.
  for (QVector<QLineF>::Iterator it = segments.begin(); it != segments.end(); ++it)
  {
    QLineF& seg = *it;
    QPointF p1 = seg.p1();
    QPointF p2 = seg.p2();
    seg.setP1( p1 + (p1 - p2) * 0.35f);
    seg.setP2( p2 + (p2 - p1) * 0.35f);
  }

  // For each adjunct segment.
  for (int adj=0; adj<2; adj++)
  {
    int idx = wrapAround(i + adj - 1, segments.size());
    for (int j=0; j<segments.size(); j++)
    {
      // If the segment to compare to is valid (ie. if it is not
      // the segment itself nor an adjacent one) then check for
      // intersection.
      if (j != idx &&
          j != wrapAround(idx-1, segments.size()) &&
          j != wrapAround(idx+1, segments.size()))
      {
        QPointF intersection;
        if (segments[idx].intersect(segments[j], &intersection) == QLineF::BoundedIntersection)
        {
          // Rearrange segments with new position at intersection point.
          v = intersection;
          segments[prev] = QLineF(polygon.at(prev), v);
          segments[i]    = QLineF(v, polygon.at(next));
        }
      }
    }
  }
}


QVector<QLineF> Polygon::_getSegments() const
{
  return _getSegments(toPolygon());
}

QVector<QLineF> Polygon::_getSegments(const QPolygonF& polygon)
{
  QVector<QLineF> segments;
  for (int i=0; i<polygon.size(); i++)
    segments.push_back(QLineF(polygon.at(i), polygon.at( (i+1) % polygon.size() )));
  return segments;
}

QPolygonF Polygon::toPolygon() const
{
  QPolygonF polygon;
  for (QVector<QPointF>::const_iterator it = vertices.begin() ;
      it != vertices.end(); ++it)
  {
    polygon.append(*it);
  }
  return polygon;
}

Mesh::Mesh() : Quad(), _nColumns(0), _nRows(0) {}

Mesh::Mesh(QPointF p1, QPointF p2, QPointF p3, QPointF p4) : Quad()
{
  // Add points in standard order.
  QVector<QPointF> points;
  points.push_back(p1);
  points.push_back(p2);
  points.push_back(p4);
  points.push_back(p3);

  // Init.
  init(points, 2, 2);
}

Mesh::Mesh(const QVector<QPointF>& points, int nColumns, int nRows) : Quad()
{
  init(points, nColumns, nRows);
}

void Mesh::init(const QVector<QPointF>& points, int nColumns, int nRows)
{
  Q_ASSERT(nColumns >= 2 && nRows >= 2);
  Q_ASSERT(points.size() == nColumns * nRows);

  _nColumns = nColumns;
  _nRows    = nRows;

  // Resize the vertices2d vector to appropriate dimensions.
  resizeVertices2d(_vertices2d, _nColumns, _nRows);

  // Just build vertices2d in the standard order.
  int k = 0;
  for (int y=0; y<_nRows; y++)
    for (int x=0; x<_nColumns; x++)
    {
      vertices.push_back( points[k] );
      _vertices2d[x][y] = k;
      k++;
    }
}

QPolygonF Mesh::toPolygon() const
{
  QPolygonF polygon;
  for (int i=0; i<nColumns(); i++)
    polygon.append(getVertex2d(i, 0));
  for (int i=0; i<nRows(); i++)
    polygon.append(getVertex2d(nColumns()-1, i));
  for (int i=nColumns()-1; i>=0; i--)
    polygon.append(getVertex2d(i, nRows()-1));
  for (int i=nRows()-1; i>=1; i--)
    polygon.append(getVertex2d(0, i));
  return polygon;
}

void Mesh::setVertex(int i, const QPointF& v)
{
  // Extract column and row of vertex.
  int col = i % nColumns();
  int row = i / nColumns();

  // Make a copy.
  QPointF realV = v;

  // Constrain vertex to stay within the internal quads it is part of.
  if (col < nColumns()-1)
  {
    if (row < nRows() - 1)
    {
      Quad quad(getVertex2d(col, row), getVertex2d(col+1, row), getVertex2d(col+1, row+1), getVertex2d(col, row+1));
      _constrainVertex(quad.toPolygon(), 0, realV);
    }
    if (row > 0)
    {
      Quad quad(getVertex2d(col, row), getVertex2d(col+1, row), getVertex2d(col+1, row-1), getVertex2d(col, row-1));
      _constrainVertex(quad.toPolygon(), 0, realV);
    }
  }
  if (col > 0)
  {
    if (row < nRows() - 1)
    {
      Quad quad(getVertex2d(col, row), getVertex2d(col-1, row), getVertex2d(col-1, row+1), getVertex2d(col, row+1));
      _constrainVertex(quad.toPolygon(), 0, realV);
    }
    if (row > 0)
    {
      Quad quad(getVertex2d(col, row), getVertex2d(col-1, row), getVertex2d(col-1, row-1), getVertex2d(col, row-1));
      _constrainVertex(quad.toPolygon(), 0, realV);
    }
  }

  // Do set vertex.
  _rawSetVertex(i, realV);
}

void Mesh::resizeVertices2d(IndexVector2d& vertices2d, int nColumns, int nRows)
{
  vertices2d.resize(nColumns);
  for (int i=0; i<nColumns; i++)
    vertices2d[i].resize(nRows);
}

//void Mesh::init(int nColumns, int nRows)
//{
//  // Create vertices correspondence of bouding quad.
//  resizeVertices2d(_vertices2d, 2, 2);
//  _vertices2d[0][0] = 0;
//  _vertices2d[1][0] = 1;
//  _vertices2d[1][1] = 2;
//  _vertices2d[0][1] = 3;
//
//  // Init number of columns and rows.
//  _nColumns = _nRows = 2;
//
//  // Add extra columns and rows.
//  for (int i=0; i<nColumns-2; i++)
//    addColumn();
//  for (int i=0; i<nRows-2; i++)
//    addRow();
//}

// vertices 0..3 = 4 corners
//
void Mesh::addColumn()
{
  // Create new vertices 2d (temporary).
  IndexVector2d newVertices2d;
  resizeVertices2d(newVertices2d, nColumns()+1, nRows());

  // Left displacement of points already there.
  qreal leftMoveProp = 1.0f/(nColumns()-1) - 1.0f/nColumns();

  // Add a point at each row.
  int k = nVertices();
  for (int y=0; y<nRows(); y++)
  {
    // Get left and right vertices.
    QPointF left  = getVertex2d( 0,            y );
    QPointF right = getVertex2d( nColumns()-1, y );
    QPointF diff  = right - left;

    // First pass: move middle points.
    for (int x=1; x<nColumns()-1; x++)
    {
      QPointF p = getVertex2d(x, y);
      p -= diff * x * leftMoveProp;
      _rawSetVertex( _vertices2d[x][y], p );
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
  IndexVector2d newVertices2d;
  resizeVertices2d(newVertices2d, nColumns(), nRows()+1);

  // Top displacement of points already there.
  qreal topMoveProp = 1.0f/(nRows()-1) - 1.0f/nRows();

  // Add a point at each row.
  int k = nVertices();
  for (int x=0; x<nColumns(); x++)
  {
    // Get left and right vertices.
    QPointF top    = getVertex2d(x, 0);
    QPointF bottom = getVertex2d(x, nRows()-1);
    QPointF diff   = bottom - top;

    // First pass: move middle points.
    for (int y=1; y<nRows()-1; y++)
    {
      QPointF p = getVertex2d(x, y);
      p -= diff * y * topMoveProp;
      _rawSetVertex( _vertices2d[x][y], p );
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

void Mesh::removeColumn(int columnId)
{
  // Cannot remove first and last columns
  Q_ASSERT(columnId >= 1 && columnId < nColumns()-1);

  // Temporary containers that will be used to rebuild new vertex space.
  IndexVector2d newVertices2d;
  resizeVertices2d(newVertices2d, nColumns()-1, nRows());

  QVector<QPointF> newVertices(vertices.size()-nRows());

  // Right displacement of points already there.
  qreal rightMoveProp = 1.0f/(nColumns()-2) - 1.0f/(nColumns()-1);

  // Process all rows.
  int k = 0;
  for (int y=0; y<nRows(); y++)
  {
    // Get left and right vertices.
    QPointF left  = getVertex2d( 0,            y );
    QPointF right = getVertex2d( nColumns()-1, y );
    QPointF diff  = right - left;

    // Move all columns.
    for (int x=0; x<nColumns(); x++)
    {
      // Ignore points from target column.
      if (x == columnId)
        continue;

      // Get current vertex.
      QPointF p = getVertex2d( x, y );

      // The x value of this point in the new space.
      int newX = x < columnId ? x : x-1;

      // Move middle points.
      if (x > 0 && x < nColumns()-1)
      {
        p += (x < columnId ? +1 : -1) * diff * newX * rightMoveProp;
      }

      // Assign new containers.
      newVertices[k]         = p;
      newVertices2d[newX][y] = k;
      k++;
    }
  }

  // Copy new mapping.
  vertices    = newVertices;
  _vertices2d = newVertices2d;

  // Increment number of columns.
  _nColumns--;
}

void Mesh::removeRow(int rowId)
{
  // Cannot remove first and last columns
  Q_ASSERT(rowId >= 1 && rowId < nRows()-1);

  // Temporary containers that will be used to rebuild new vertex space.
  IndexVector2d newVertices2d;
  resizeVertices2d(newVertices2d, nColumns(), nRows()-1);

  QVector<QPointF> newVertices(vertices.size()-nColumns());

  // Bottom displacement of points already there.
  qreal bottomMoveProp = 1.0f/(nRows()-2) - 1.0f/(nRows()-1);

  // Process all columns.
  int k = 0;
  for (int x=0; x<nColumns(); x++)
  {
    // Get top and bottom vertices.
    QPointF top    = getVertex2d(x, 0);
    QPointF bottom = getVertex2d(x, nRows()-1);
    QPointF diff   = bottom - top;

    // Move all rows.
    for (int y=0; y<nRows(); y++)
    {
      // Ignore points from target row.
      if (y == rowId)
        continue;

      // Get current vertex.
      QPointF p = getVertex2d( x, y );

      // The y value of this point in the new space.
      int newY = y < rowId ? y : y-1;

      // Move middle points.
      if (y > 0 && y < nRows()-1)
      {
        p += (y < rowId ? +1 : -1) * diff * newY * bottomMoveProp;
      }

      // Assign new containers.
      newVertices[k]         = p;
      newVertices2d[x][newY] = k;
      k++;
    }
  }

  // Copy new mapping.
  vertices    = newVertices;
  _vertices2d = newVertices2d;

  // Increment number of columns.
  _nRows--;
}


void Mesh::resize(int nColumns_, int nRows_)
{
  // Brutal: if asked to reduce columns or rows, just delete and redo.
  if (nColumns_ < nColumns())
  {
    while (nColumns_ != nColumns())
      removeColumn(nColumns()-2);
  }
  if (nRows_ < nRows())
  {
    while (nRows_ != nRows())
      removeRow(nRows()-2);
  }
  if (nColumns_ > nColumns())
  {
    while (nColumns_ != nColumns())
      addColumn();
  }
  if (nRows_ > nRows())
  {
    while (nRows_ != nRows())
      addRow();
  }
}
QVector<Quad> Mesh::getQuads() const
{
  QVector<Quad> quads;
  for (int i=0; i<nHorizontalQuads(); i++)
  {
    for (int j=0; j<nVerticalQuads(); j++)
    {
      Quad quad(
          getVertex2d(i,   j  ),
          getVertex2d(i+1, j  ),
          getVertex2d(i+1, j+1),
          getVertex2d(i,   j+1)
      );
      quads.push_back(quad);
    }
  }

  return quads;
}

QVector< QVector<Quad> > Mesh::getQuads2d() const
{
  QVector< QVector<Quad> > quads2d;
  for (int i=0; i<nHorizontalQuads(); i++)
  {
    QVector<Quad> column;
    for (int j=0; j<nVerticalQuads(); j++)
    {
      Quad quad(
          getVertex2d(i,   j  ),
          getVertex2d(i+1, j  ),
          getVertex2d(i+1, j+1),
          getVertex2d(i,   j+1)
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
  QVector<QPointF> newVertices(vertices.size());
  int k = 0;
  for (int y=0; y<nRows(); y++)
    for (int x=0; x<nColumns(); x++)
      newVertices[k++] = getVertex2d( x, y );

  // Populate _vertices2d.
  k = 0;
  for (int y=0; y<nRows(); y++)
    for (int x=0; x<nColumns(); x++)
      _vertices2d[x][y] = k++;

  // Copy.
  vertices = newVertices;
}

QTransform Ellipse::toUnitCircle() const
{
  const QPointF& center = getCenter();
  return QTransform().scale(1.0/getHorizontalRadius(), 1.0/getVerticalRadius())
                     .rotateRadians(-getRotationRadians())
                     .translate(-center.x(), -center.y());
}

QTransform Ellipse::fromUnitCircle() const
{
  return toUnitCircle().inverted();
}

bool Ellipse::includesPoint(qreal x, qreal y)
{
  return (QVector2D(toUnitCircle().map(QPointF(x, y))).length() <= 1);
}

void Ellipse::setVertex(int i, const QPointF& v)
{
  // Save vertical axis vector.
  const QVector2D& vAxis  = getVerticalAxis();

  // If changed one of the two rotation-controlling points, adjust the other two points.
  if (i == 0 || i == 2)
  {
    // Transformation ellipse_t --> circle.
    QTransform transform = toUnitCircle();

    // Change the vertex.
    _rawSetVertex(i, v);

    // Combine with transformation circle -> ellipse_{t+1}.
    transform *= fromUnitCircle();

    // Set vertices.
    MShape::setVertex(1, transform.map( getVertex(1) ));
    MShape::setVertex(3, transform.map( getVertex(3) ));
    if (hasCenterControl())
      MShape::setVertex(4, transform.map( getVertex(4) ));
  }

  // If changed one of the two other points, just change the vertical axis.
  else if (i == 1 || i == 3)
  {
    // Retrieve the new horizontal axis vector and center.
    const QVector2D center(getCenter());

    QVector2D vFromCenter = QVector2D(v) - center;

    // Find projection of v onto vAxis / 2.
    QVector2D vAxisNormalized = vAxis.normalized();
    const QVector2D& projection = QVector2D::dotProduct( vFromCenter, vAxisNormalized ) * vAxisNormalized;

    // Assign vertical control points.
    QPointF v1;
    QPointF v3;
    if (i == 1)
    {
      v1 = (center + projection).toPointF();
      v3 = (center - projection).toPointF();
    }
    else
    {
      v1 = (center - projection).toPointF();
      v3 = (center + projection).toPointF();
    }

    // Transformation ellipse_t --> circle.
    QTransform transform = toUnitCircle();

    // Change vertical points.
    _rawSetVertex(1, v1);
    _rawSetVertex(3, v3);

    // Combine with transformation circle -> ellipse_{t+1}.
    transform *= fromUnitCircle();

    // Set vertices.
    if (hasCenterControl())
      _rawSetVertex(4, transform.map( getVertex(4) ));
  }

  // Center control point (make sure it stays inside!).
  else if (hasCenterControl())
  {
    // Clip control point.
    _rawSetVertex(4, clipInside(v));
  }

  // Just to be sure.
  sanitize();
}
