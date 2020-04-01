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

namespace mmp {

MShape::MShape(const QVector<QPointF>& vertices_) : _isLocked(false) {
  setVertices(vertices_);
  build();
}

void MShape::copyFrom(const MShape& shape)
{
  // Just copy vertices.
  setVertices(shape.getVertices());
}

MShape* MShape::clone() const {
  MShape* copyShape = _create();
  copyShape->copyFrom(*this);
  return copyShape;
}

void MShape::setShapeMode(MShape::ShapeMode mode, bool next)
{
  if (!next)
    _shapeMode = mode;
  else
    _shapeMode = static_cast<MShape::ShapeMode>((mode + 1) % 3);

}

void MShape::applyTransform(const QTransform& transform)
{
  for (QVector<QPointF>::iterator it = vertices.begin();
       it != vertices.end(); ++it)
    (*it) = transform.map(*it);
}

void MShape::transform(const QPointF& translate, qreal scale, qreal rotate)
{
	const QPointF& center = getCenter();

	// Create transformation.
	QTransform transform;

	// Center and perform rotate + scale, then de-center.
	transform.translate(+center.x(), +center.y());
	if (rotate)
		transform.rotate(rotate);
	if (scale)
		transform.scale(scale, scale);
	transform.translate(-center.x(), -center.y());

	// Apply translation.
	if (!translate.isNull())
		transform.translate(translate.x(), translate.y());

	// Perform operation.
	applyTransform(transform);
}

void MShape::translate(const QPointF& offset)
{
	transform(offset, 0, 0);
}

void MShape::rotate(qreal angle)
{
	transform(QPointF(), 0, angle);
}

void MShape::scale(qreal scaling)
{
	transform(QPointF(), scaling, 0);
}

QPointF MShape::getCenter() const
{
  QPointF center;
  for (QVector<QPointF>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
    center += (*it);
  center /= nVertices();
  return center;
}

void MShape::read(const QDomElement& obj)
{
  // Read basic data.
  Serializable::read(obj);

  // Read vertices.
  QDomElement verticesObj = obj.firstChildElement("vertices");
  QDomNode vertexNode = verticesObj.firstChild();
  QVector<QPointF> vertices;
  while (!vertexNode.isNull())
  {
    const QDomElement& vertexElem = vertexNode.toElement();
    qreal x = vertexElem.attribute("x").toDouble();
    qreal y = vertexElem.attribute("y").toDouble();
    vertices.append(QPointF(x, y));

    vertexNode = vertexNode.nextSibling();
  }

  // Set the vertices.
  setVertices(vertices);

  // Rebuild.
  build();
}

void MShape::write(QDomElement& obj)
{
  // Write basic data.
  Serializable::write(obj);

  // Write vertices.
  QDomElement verticesObj = obj.ownerDocument().createElement("vertices");
  for (int i=0; i<nVertices(); i++)
  {
    QDomElement vertexObj = obj.ownerDocument().createElement("vertex");
    vertexObj.setAttribute("x", QString::number(getVertex(i).x()));
    vertexObj.setAttribute("y", QString::number(getVertex(i).y()));
    verticesObj.appendChild(vertexObj);
  }

  obj.appendChild(verticesObj);
}

}
