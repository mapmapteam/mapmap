/*
 * ProjectReader.cpp
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
#include "ProjectReader.h"
#include <sstream>
#include <iostream>
#include <string>

ProjectReader::ProjectReader(MainWindow *window) : _window(window)
{
}

bool ProjectReader::readFile(QIODevice *device)
{
  QString errorStr;
  int errorLine;
  int errorColumn;

  QDomDocument doc;
  if (!doc.setContent(device, false, &errorStr, &errorLine, &errorColumn)) {
    std::cerr << "Error: Parse error at line " << errorLine << ", "
              << "column " << errorColumn << ": "
              << qPrintable(errorStr) << std::endl;
    return false;
  }

  QDomElement root = doc.documentElement();
  // The handling of the version number will get fancier as we go.
  if (root.tagName() != "project" || root.attribute("version") != "0.1")
  {
    _xml.raiseError(QObject::tr("The file is not a mapmap version 0.1 file."));
    return false;
  }

  parseProject(root);

  return (! _xml.hasError() );
}

QString ProjectReader::errorString() const
{
  return QObject::tr("%1\nLine %2, column %3")
    .arg(_xml.errorString())
    .arg(_xml.lineNumber())
    .arg(_xml.columnNumber());
}


void ProjectReader::parseProject(const QDomElement& project)
{
  QDomElement paints = project.firstChildElement("paints");
  QDomElement mappings = project.firstChildElement("mappings");

  // Parse paints.
  QDomNode paint = paints.firstChild();
  while (!paint.isNull())
  {
    parsePaint(paint.toElement());
    paint = paint.nextSibling();
  }

  // Parse mappings.
  QDomNode mapping = mappings.firstChild();
  while (!mapping.isNull())
  {
    parseMapping(mapping.toElement());
    mapping = mapping.nextSibling();
  }
}

void ProjectReader::parsePaint(const QDomElement& paint)
{
  QString paintAttrId   = paint.attribute("id", QString::number(NULL_UID));
  QString paintAttrName = paint.attribute("name", "");
  QString paintAttrType = paint.attribute("type", "");

  if (paintAttrType == "media")
  {
    QString uri = paint.firstChildElement("uri").text();
    QString x   = paint.firstChildElement("x").text();
    QString y   = paint.firstChildElement("y").text();

    uid id = _window->createMediaPaint(paintAttrId.toInt(), uri, x.toFloat(), y.toFloat());
    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create media with uri %1.").arg(uri));
  }
  else if (paintAttrType == "color")
  {
    QString rgb = paint.firstChildElement("rgb").text();
    QColor color(rgb);

    uid id = _window->createColorPaint(paintAttrId.toInt(), color);
    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create color with RGB hex code %1.").arg(rgb));

  }
  else
    _xml.raiseError(QObject::tr("Unsupported paint type: %1.").arg(paintAttrType));

}

void ProjectReader::parseMapping(const QDomElement& mapping)
{
  QString mappingAttrId      = mapping.attribute("id", QString::number(NULL_UID));
  QString mappingAttrPaintId = mapping.attribute("paint_id", QString::number(NULL_UID));
  QString mappingAttrType    = mapping.attribute("type", "");

  // Get destination shape.
  QDomElement dst = mapping.firstChildElement("destination");
  QVector<QPointF> dstPoints;

  if (mappingAttrType == "triangle_texture")
  {
    // Parse destination triangle.
    _parseTriangle(dst, dstPoints);

    // Get / parse source shape.
    QDomElement src = mapping.firstChildElement("source");
    QVector<QPointF> srcPoints;
    _parseTriangle(src, srcPoints);

    uid id = _window->createTriangleTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), srcPoints, dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create triangle texture mapping"));
  }
  else if (mappingAttrType == "mesh_texture")
  {
    // Parse destination mesh.
    int nColumns;
    int nRows;
    _parseMesh(dst, dstPoints, nColumns, nRows);

    // Get / parse source shape.
    QDomElement src = mapping.firstChildElement("source");
    QVector<QPointF> srcPoints;
    _parseMesh(src, srcPoints, nColumns, nRows);

    uid id = _window->createMeshTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), nColumns, nRows, srcPoints, dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create mesh texture mapping"));

  }
  else if (mappingAttrType == "ellipse_texture")
  {
    // Parse destination ellipse.
    _parseEllipse(dst, dstPoints);

    // Get / parse source shape.
    QDomElement src = mapping.firstChildElement("source");
    QVector<QPointF> srcPoints;
    _parseEllipse(src, srcPoints);

    uid id = _window->createEllipseTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), srcPoints, dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create ellipse texture mapping"));
  }
  else if (mappingAttrType == "triangle_color")
  {
    // Parse destination triangle.
    _parseTriangle(dst, dstPoints);

    uid id = _window->createTriangleColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create triangle color mapping"));
  }
  else if (mappingAttrType == "quad_color")
  {
    // Parse destination quad.
    _parseQuad(dst, dstPoints);

    uid id = _window->createQuadColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create quad color mapping"));
  }
  else if (mappingAttrType == "ellipse_color")
  {
    // Parse destination ellipse.
    _parseEllipse(dst, dstPoints);

    uid id = _window->createEllipseColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create ellipse color mapping"));
  }
  else
    _xml.raiseError(QObject::tr("Unsupported mapping type: %1.").arg(mappingAttrType));
}

void ProjectReader::_parseStandardShape(const QString& type, const QDomElement& shape, QVector<QPointF>& points, int nVertices)
{
  // Check that the element is really a triangle.
  QString typeAttr = shape.attribute("shape", "");
  if (typeAttr != type)
    _xml.raiseError(QObject::tr("Wrong shape type \"%1\" for destination: expected \"%2\".").arg(typeAttr).arg(type));

  // Reset list of points.
  points.clear();

  // Add vertices.
  QDomElement vertex = shape.firstChildElement("vertex");
  while (!vertex.isNull())
  {
    points.push_back(_parseVertex(vertex));
    vertex = vertex.nextSiblingElement("vertex");
  }

  if (nVertices >= 0 && points.size() != nVertices)
    _xml.raiseError(QObject::tr("Shape of type '%1' has %2 vertices: expected %3.").arg(type).arg(points.size()).arg(nVertices));
}

void ProjectReader::_parseQuad(const QDomElement& quad, QVector<QPointF>& points)
{
  _parseStandardShape("quad", quad, points, 4);
}

void ProjectReader::_parseTriangle(const QDomElement& triangle, QVector<QPointF>& points)
{
  _parseStandardShape("triangle", triangle, points, 3);
}

void ProjectReader::_parseEllipse(const QDomElement& ellipse, QVector<QPointF>& points)
{
  _parseStandardShape("ellipse", ellipse, points);
  if (points.size() != 4 && points.size() != 5)
    _xml.raiseError(QObject::tr("Shape has %1 vertices: expected 4 or 5.").arg(points.size()));
}

void ProjectReader::_parseMesh(const QDomElement& mesh, QVector<QPointF>& points, int& nColumns, int& nRows)
{
  // Check that the element is really a mash.
  QString type = mesh.attribute("shape", "");
  if (type != "mesh")
    _xml.raiseError(QObject::tr("Wrong shape type for destination: %1.").arg(type));

  // Reset list of points.
  points.clear();

  // Check columns and rows.
  nColumns = mesh.firstChildElement("dimensions").attribute("columns").toInt();
  nRows    = mesh.firstChildElement("dimensions").attribute("rows").toInt();

  // Add vertices.
  QDomElement vertex = mesh.firstChildElement("vertex");
  while (!vertex.isNull())
  {
    points.push_back(_parseVertex(vertex));
    vertex = vertex.nextSiblingElement("vertex");
  }

  if (points.size() != nColumns*nRows)
    _xml.raiseError(QObject::tr("Shape has wrong number of vertices."));
}


QPointF ProjectReader::_parseVertex(const QDomElement& vertex)
{
  return QPointF(
      vertex.attribute("x", "0").toFloat(),
      vertex.attribute("y", "0").toFloat()
      );
}


//void ProjectReader::readProject()
//{
//  // FIXME: avoid asserts
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "project");
//
//  while(! _xml.atEnd() && ! _xml.hasError())
//  {
//    /* Read next element.*/
//    QXmlStreamReader::TokenType token = _xml.readNext();
//    /* If token is just StartDocument, we'll go to next.*/
//    if (token == QXmlStreamReader::StartDocument)
//    {
//      continue;
//    }
//    /* If token is StartElement, we'll see if we can read it.*/
//    else if (token == QXmlStreamReader::StartElement)
//    {
//      if (_xml.name() == "paints")
//        continue;
//      else if (_xml.name() == "paint")
//      {
//        std::cout << " * paint" << std::endl;
//        readPaint();
//      }
//      else if (_xml.name() == "mappings")
//        continue;
//      else if (_xml.name() == "mapping")
//      {
//        std::cout << " * mapping " << std::endl;
//        readMapping(); // NULL);
//      }
//      else
//      {
//        std::cout << " * skip element " << _xml.name().string() << std::endl;
//        //_xml.skipCurrentElement();
//      }
//    }
//  } // while
//  _xml.clear();
//}
//
//void ProjectReader::readMapping()
//{
//  // FIXME: we assume an Image mapping
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "mapping");
//  const QString *paint_id_attr;
//  QXmlStreamAttributes attributes = _xml.attributes();
//
//  if (attributes.hasAttribute("", "paint_id"))
//    paint_id_attr = attributes.value("", "paint_id").string();
//
//  std::cout << "   * <mapping> " << "with paint ID " << paint_id_attr << std::endl;
//
//  //QString title = _xml.readElementText();
//  //item->setText(0, title);
//}
//
//void ProjectReader::readPaint()
//{
//  // FIXME: we assume an Image mapping
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "paint");
//  const QString *paint_id_attr;
//  const QString *uri_attr;
//  const QString *typeAttrValue;
//  QXmlStreamAttributes attributes = _xml.attributes();
//
//  if (attributes.hasAttribute("", "name"))
//    paint_id_attr = attributes.value("", "name").string();
//  if (attributes.hasAttribute("", "type"))
//    typeAttrValue = attributes.value("", "type").string();
//
//  std::cout << "Found " << typeAttrValue->toStdString() <<
//    " paint " << paint_id_attr->toStdString() << std::endl;
//
//  /* Next element... */
//  _xml.readNext();
//  // /*
//  //  * We're going to loop over the things because the order might change.
//  //  * We'll continue the loop until we hit an EndElement.
//  //  */
//  while(! (_xml.tokenType() == QXmlStreamReader::EndElement &&
//    _xml.name() == "paint"))
//  {
//    if (_xml.tokenType() == QXmlStreamReader::StartElement)
//    {
//      if (_xml.name() == "uri")
//      {
//        /* ...go to the next. */
//        _xml.readNext();
//        /*
//         * This elements needs to contain Characters so we know it's
//         * actually data, if it's not we'll leave.
//         */
//        if(_xml.tokenType() != QXmlStreamReader::Characters)
//        {
//          // pass
//        }
//        //uri_attr = _xml.text().toString();
//        //std::cout << "uri " << uri_attr.toStdString() << std::endl;
//      }
//      else if (_xml.name() == "width")
//      {
//        // pass
//      }
//      else if (_xml.name() == "height")
//      {
//        // pass
//      }
//    }
//    _xml.readNext();
//  }
//
//  // TODO: call this->_manager->getController->createPaint(...)
//}

