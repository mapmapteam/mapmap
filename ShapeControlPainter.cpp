/*
 * ShapeControlPainter.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#include "ShapeControlPainter.h"


ShapeControlPainter::ShapeControlPainter(ShapeGraphicsItem* shapeItem)
  : _shapeItem(shapeItem)
{}

MShape::ptr ShapeControlPainter::getShape() const { return _shapeItem->getShape(); }

void ShapeControlPainter::paint(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices)
{
  _paintShape(painter, canvas);
  _paintVertices(painter, canvas, selectedVertices);
}

void ShapeControlPainter::_paintVertices(QPainter *painter, MapperGLCanvas* canvas, const QList<int>& selectedVertices)
{
  qreal zoomFactor = canvas->getZoomFactor();
  qreal selectRadius = MM::VERTEX_SELECT_RADIUS / zoomFactor;
  qreal strokeWidth  = MM::VERTEX_SELECT_STROKE_WIDTH / zoomFactor;

  for (int i=0; i<getShape()->nVertices(); i++)
    Util::drawControlsVertex(painter, getShape()->getVertex(i), selectedVertices.contains(i), selectRadius, strokeWidth);
}

QPen ShapeControlPainter::getRescaledShapeStroke(MapperGLCanvas* canvas, bool innerStroke)
{
  return QPen(QBrush(MM::CONTROL_COLOR), (innerStroke ? MM::SHAPE_INNER_STROKE_WIDTH : MM::SHAPE_STROKE_WIDTH) / canvas->getZoomFactor());
}

void PolygonControlPainter::_paintShape(QPainter *painter, MapperGLCanvas* canvas)
{
  Polygon* poly = static_cast<Polygon*>(getShape().data());
  Q_ASSERT(poly);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas));

  // Draw inner quads.
  painter->drawPolygon(poly->toPolygon());
}


void EllipseControlPainter::_paintShape(QPainter *painter, MapperGLCanvas* canvas)
{
  Ellipse* ellipse = static_cast<Ellipse*>(getShape().data());
  Q_ASSERT(ellipse);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas));
  painter->setBrush(Qt::NoBrush);

  // Draw ellipse contour.
  QPainterPath path;
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  painter->drawPath(transform.map(path));
}

void MeshControlPainter::_paintShape(QPainter *painter, MapperGLCanvas* canvas)
{
  Mesh* mesh = static_cast<Mesh*>(getShape().data());
  Q_ASSERT(mesh);

  // Init colors and stroke.
  painter->setPen(getRescaledShapeStroke(canvas, true));

  // Draw inner quads.
  QVector<Quad::ptr> quads = mesh->getQuads();
  for (QVector<Quad::ptr>::const_iterator it = quads.begin(); it != quads.end(); ++it)
  {
    painter->drawPolygon((*it)->toPolygon());
  }

  // Draw outer quad.
  painter->setPen(getRescaledShapeStroke(canvas));
  painter->drawPolygon(_shapeItem->mapFromScene(mesh->toPolygon()));
}

