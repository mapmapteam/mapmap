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

void MShape::copyFrom(const MShape& shape)
{
  // Just copy vertices.
  vertices = shape.vertices;
}

MShape* MShape::clone() const {
  MShape* copyShape = _create();
  copyShape->copyFrom(*this);
  return copyShape;
}

void MShape::translate(const QPointF& offset)
{
  // We can feel free to translate every vertex without check by default.
  for (QVector<QPointF>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    *it += offset;
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
  setVertices(vertices);
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

