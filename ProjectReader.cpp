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
  if (root.tagName() != "project" || root.attribute("version") != MM::VERSION)
  {
    _xml.raiseError(QObject::tr("The file is not a mapmap version %1 file.").arg(MM::VERSION));
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
  // TODO: this is dangerous if we have
  MappingManager& manager = _window->getMappingManager();
  manager.clearAll();

  QDomElement paints = project.firstChildElement(ProjectAttributes::PAINTS);
  QDomElement mappings = project.firstChildElement(ProjectAttributes::MAPPINGS);

  // Parse paints.
  QDomNode paintNode = paints.firstChild();
  while (!paintNode.isNull())
  {
    Paint::ptr paint = parsePaint(paintNode.toElement());

    if (paint.isNull())
    {
      qDebug() << "Problem creating paint." << endl;
    }
    else
    {
      manager.addPaint(paint);
      _window->addPaintItem(paint->getId(), paint->getIcon(), paint->getName());
    }
    paintNode = paintNode.nextSibling();
  }

  // Parse mappings.
  QDomNode mappingNode = mappings.firstChild();
  while (!mappingNode.isNull())
  {
    Mapping::ptr mapping = parseMapping(mappingNode.toElement());
    if (mapping.isNull())
    {
      qDebug() << "Problem creating mapping." << endl;
    }
    else
    {
      manager.addMapping(mapping);
      _window->addMappingItem(mapping->getId());
    }

    mappingNode = mappingNode.nextSibling();
  }
}

Paint::ptr ProjectReader::parsePaint(const QDomElement& paintElem)
{
  QString className = paintElem.attribute(ProjectAttributes::CLASS_NAME);
  int id            = paintElem.attribute(ProjectAttributes::ID, QString::number(NULL_UID)).toInt();

  qDebug() << "Found paint with classname: " << className << endl;

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    // Create new instance.
    Paint::ptr paint (qobject_cast<Paint*>(metaObject->newInstance( Q_ARG(int, id)) ));
    if (paint.isNull())
    {
      qDebug() << QObject::tr("Problem at creation of paint.") << endl;
//      _xml.raiseError(QObject::tr("Problem at creation of paint."));
    }
    else
      qDebug() << "Created new instance with id: " << paint->getId();

    paint->read(paintElem);

    return paint;
  }

  else
  {
    _xml.raiseError(QObject::tr("Unable to create paint of type '%1'.").arg(className));
    return Paint::ptr();
  }
}

Mapping::ptr ProjectReader::parseMapping(const QDomElement& mappingElem)
{
  // Get attributes.
  QString className = mappingElem.attribute(ProjectAttributes::CLASS_NAME);
  int id            = mappingElem.attribute(ProjectAttributes::ID, QString::number(NULL_UID)).toInt();

  qDebug() << "Found mapping with classname: " << className << endl;

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    // Create new instance.
    Mapping::ptr mapping (qobject_cast<Mapping*>(metaObject->newInstance( Q_ARG(int, id)) ));
    if (mapping.isNull())
    {
      qDebug() << QObject::tr("Problem at creation of mapping.") << endl;
//      _xml.raiseError(QObject::tr("Problem at creation of paint."));
    }
    else
      qDebug() << "Created new instance with id: " << mapping->getId();

    mapping->read(mappingElem);

    return mapping;
  }

  else
  {
    _xml.raiseError(QObject::tr("Unable to create paint of type '%1'.").arg(className));
    return Mapping::ptr();
  }
}

//
//Mapping::ptr ProjectReader::parseMapping(const QDomElement& mappingElem)
//{
//  QString className = mappingElem.attribute(ProjectAttributes::CLASS_NAME);
//  int id            = mappingElem.attribute(ProjectAttributes::ID, QString::number(NULL_UID)).toInt();
//  int paintId       = mappingElem.attribute(ProjectAttributes::PAINT_ID, QString::number(NULL_UID)).toInt();
//  QString name      = mappingElem.attribute(ProjectAttributes::NAME, "");
//
//  qDebug() << "Found mapping with classname: " << className << endl;
//
//  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
//  if (metaObject)
//  {
//    // Get paint and shape.
//    Paint::ptr paint = _window->getMappingManager().getPaintById(paintId);
//
//    // Create new instance.
//    Mapping::ptr mapping (qobject_cast<Mapping*>(metaObject->newInstance( Q_ARG(Paint::ptr, paint), Q_ARG(int, id)) ));
//    if (!mapping)
//      _xml.raiseError(QObject::tr("Problem at creation of mapping."));
//    else
//      qDebug() << "Created new instance with id: " << mapping->getId();
//
//    // Fill up properties.
//    int count = metaObject->propertyCount();
//    for (int i=0; i<count; ++i) {
//      // Get property/tag.
//      QMetaProperty property = metaObject->property(i);
//
//      // If property is writable, try to find it and rewrite it.
//      if (property.isWritable())
//      {
//        const char* propertyName =  property.name();
//
//        // Find element.
//        QDomElement propertyElem = mappingElem.firstChildElement(propertyName);
//        if (!propertyElem.isNull())
//        {
//          mapping->setProperty(propertyName, QVariant(propertyElem.text()));
//        }
//      }
//    }
//
//    // Gather shapes.
//    MappingManager& manager = _window->getMappingManager();
//    manager.addMapping(mapping);
//    _window->createMeshTextureMapping(mapping->getId(), mapping->getPaint()->getId(), )
//
//
//    return mapping;
//  }
//
//  else
//  {
//    _xml.raiseError(QObject::tr("Unable to create paint of type '%1'.").arg(className));
//    return Paint::ptr();
//  }
//}
//
//Mapping::ptr ProjectReader::parseMapping(const QDomElement& mapping)
//{
//  uid id = NULL_UID;
//  QString mappingAttrId      = mapping.attribute("id", QString::number(NULL_UID));
//  QString mappingAttrPaintId = mapping.attribute("paint_id", QString::number(NULL_UID));
//  QString mappingAttrType    = mapping.attribute("type", "");
//
//  bool isLocked = (mapping.attribute("locked", QString::number(1)) == "1");
//  bool isSolo = (mapping.attribute("solo", QString::number(1)) == "1");
//  bool isVisible = (mapping.attribute("visible", QString::number(1)) == "1");
//
//  // Get destination shape.
//  QDomElement dst = mapping.firstChildElement("destination");
//  QVector<QPointF> dstPoints;
//
//  if (mappingAttrType == "triangle_texture")
//  {
//    // Parse destination triangle.
//    _parseTriangle(dst, dstPoints);
//
//    // Get / parse source shape.
//    QDomElement src = mapping.firstChildElement("source");
//    QVector<QPointF> srcPoints;
//    _parseTriangle(src, srcPoints);
//
//    id = _window->createTriangleTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), srcPoints, dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create triangle texture mapping"));
//  }
//  else if (mappingAttrType == "mesh_texture")
//  {
//    // Parse destination mesh.
//    int nColumns;
//    int nRows;
//    _parseMesh(dst, dstPoints, nColumns, nRows);
//
//    // Get / parse source shape.
//    QDomElement src = mapping.firstChildElement("source");
//    QVector<QPointF> srcPoints;
//    _parseMesh(src, srcPoints, nColumns, nRows);
//
//    id = _window->createMeshTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), nColumns, nRows, srcPoints, dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create mesh texture mapping"));
//
//  }
//  else if (mappingAttrType == "ellipse_texture")
//  {
//    // Parse destination ellipse.
//    _parseEllipse(dst, dstPoints);
//
//    // Get / parse source shape.
//    QDomElement src = mapping.firstChildElement("source");
//    QVector<QPointF> srcPoints;
//    _parseEllipse(src, srcPoints);
//
//    id = _window->createEllipseTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), srcPoints, dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create ellipse texture mapping"));
//  }
//  else if (mappingAttrType == "triangle_color")
//  {
//    // Parse destination triangle.
//    _parseTriangle(dst, dstPoints);
//
//    id = _window->createTriangleColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create triangle color mapping"));
//  }
//  else if (mappingAttrType == "quad_color")
//  {
//    // Parse destination quad.
//    _parseQuad(dst, dstPoints);
//
//    id = _window->createQuadColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create quad color mapping"));
//  }
//  else if (mappingAttrType == "ellipse_color")
//  {
//    // Parse destination ellipse.
//    _parseEllipse(dst, dstPoints);
//
//    id = _window->createEllipseColorMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), dstPoints);
//
//    if (id == NULL_UID)
//      _xml.raiseError(QObject::tr("Cannot create ellipse color mapping"));
//  }
//  else
//    _xml.raiseError(QObject::tr("Unsupported mapping type: %1.").arg(mappingAttrType));
//
//  // and then set some more attributes:
//  if (id != NULL_UID)
//  {
//    _window->setMappingVisible(id, isVisible);
//    _window->setMappingSolo   (id, isSolo);
//    _window->setMappingLocked (id, isLocked);
//  }
//}
//
//void ProjectReader::_parseStandardShape(const QString& type, const QDomElement& shape, QVector<QPointF>& points, int nVertices)
//{
//  // Check that the element is really a triangle.
//  QString typeAttr = shape.attribute("shape", "");
//  if (typeAttr != type)
//    _xml.raiseError(QObject::tr("Wrong shape type \"%1\" for destination: expected \"%2\".").arg(typeAttr).arg(type));
//
//  // Reset list of points.
//  points.clear();
//
//  // Add vertices.
//  QDomElement vertex = shape.firstChildElement("vertex");
//  while (!vertex.isNull())
//  {
//    points.push_back(_parseVertex(vertex));
//    vertex = vertex.nextSiblingElement("vertex");
//  }
//
//  if (nVertices >= 0 && points.size() != nVertices)
//    _xml.raiseError(QObject::tr("Shape of type '%1' has %2 vertices: expected %3.").arg(type).arg(points.size()).arg(nVertices));
//}
//
//void ProjectReader::_parseQuad(const QDomElement& quad, QVector<QPointF>& points)
//{
//  _parseStandardShape("quad", quad, points, 4);
//}
//
//void ProjectReader::_parseTriangle(const QDomElement& triangle, QVector<QPointF>& points)
//{
//  _parseStandardShape("triangle", triangle, points, 3);
//}
//
//void ProjectReader::_parseEllipse(const QDomElement& ellipse, QVector<QPointF>& points)
//{
//  _parseStandardShape("ellipse", ellipse, points);
//  if (points.size() != 4 && points.size() != 5)
//    _xml.raiseError(QObject::tr("Shape has %1 vertices: expected 4 or 5.").arg(points.size()));
//}
//
//void ProjectReader::_parseMesh(const QDomElement& mesh, QVector<QPointF>& points, int& nColumns, int& nRows)
//{
//  // Check that the element is really a mash.
//  QString type = mesh.attribute("shape", "");
//  if (type != "mesh")
//    _xml.raiseError(QObject::tr("Wrong shape type for destination: %1.").arg(type));
//
//  // Reset list of points.
//  points.clear();
//
//  // Check columns and rows.
//  nColumns = mesh.firstChildElement("dimensions").attribute("columns").toInt();
//  nRows    = mesh.firstChildElement("dimensions").attribute("rows").toInt();
//
//  // Add vertices.
//  QDomElement vertex = mesh.firstChildElement("vertex");
//  while (!vertex.isNull())
//  {
//    points.push_back(_parseVertex(vertex));
//    vertex = vertex.nextSiblingElement("vertex");
//  }
//
//  if (points.size() != nColumns*nRows)
//    _xml.raiseError(QObject::tr("Shape has wrong number of vertices."));
//}
//
//
//QPointF ProjectReader::_parseVertex(const QDomElement& vertex)
//{
//  return QPointF(
//      vertex.attribute("x", "0").toFloat(),
//      vertex.attribute("y", "0").toFloat()
//      );
//}
//
//
////void ProjectReader::readProject()
////{
////  // FIXME: avoid asserts
////  Q_ASSERT(_xml.isStartElement() && _xml.name() == "project");
////
////  while(! _xml.atEnd() && ! _xml.hasError())
////  {
////    /* Read next element.*/
////    QXmlStreamReader::TokenType token = _xml.readNext();
////    /* If token is just StartDocument, we'll go to next.*/
////    if (token == QXmlStreamReader::StartDocument)
////    {
////      continue;
////    }
////    /* If token is StartElement, we'll see if we can read it.*/
////    else if (token == QXmlStreamReader::StartElement)
////    {
////      if (_xml.name() == "paints")
////        continue;
////      else if (_xml.name() == "paint")
////      {
////        std::cout << " * paint" << std::endl;
////        readPaint();
////      }
////      else if (_xml.name() == "mappings")
////        continue;
////      else if (_xml.name() == "mapping")
////      {
////        std::cout << " * mapping " << std::endl;
////        readMapping(); // NULL);
////      }
////      else
////      {
////        std::cout << " * skip element " << _xml.name().string() << std::endl;
////        //_xml.skipCurrentElement();
////      }
////    }
////  } // while
////  _xml.clear();
////}
////
////void ProjectReader::readMapping()
////{
////  // FIXME: we assume an Image mapping
////  Q_ASSERT(_xml.isStartElement() && _xml.name() == "mapping");
////  const QString *paint_id_attr;
////  QXmlStreamAttributes attributes = _xml.attributes();
////
////  if (attributes.hasAttribute("", "paint_id"))
////    paint_id_attr = attributes.value("", "paint_id").string();
////
////  std::cout << "   * <mapping> " << "with paint ID " << paint_id_attr << std::endl;
////
////  //QString title = _xml.readElementText();
////  //item->setText(0, title);
////}
////
////void ProjectReader::readPaint()
////{
////  // FIXME: we assume an Image mapping
////  Q_ASSERT(_xml.isStartElement() && _xml.name() == "paint");
////  const QString *paint_id_attr;
////  const QString *uri_attr;
////  const QString *typeAttrValue;
////  QXmlStreamAttributes attributes = _xml.attributes();
////
////  if (attributes.hasAttribute("", "name"))
////    paint_id_attr = attributes.value("", "name").string();
////  if (attributes.hasAttribute("", "type"))
////    typeAttrValue = attributes.value("", "type").string();
////
////  std::cout << "Found " << typeAttrValue->toStdString() <<
////    " paint " << paint_id_attr->toStdString() << std::endl;
////
////  /* Next element... */
////  _xml.readNext();
////  // /*
////  //  * We're going to loop over the things because the order might change.
////  //  * We'll continue the loop until we hit an EndElement.
////  //  */
////  while(! (_xml.tokenType() == QXmlStreamReader::EndElement &&
////    _xml.name() == "paint"))
////  {
////    if (_xml.tokenType() == QXmlStreamReader::StartElement)
////    {
////      if (_xml.name() == "uri")
////      {
////        /* ...go to the next. */
////        _xml.readNext();
////        /*
////         * This elements needs to contain Characters so we know it's
////         * actually data, if it's not we'll leave.
////         */
////        if(_xml.tokenType() != QXmlStreamReader::Characters)
////        {
////          // pass
////        }
////        //uri_attr = _xml.text().toString();
////        //std::cout << "uri " << uri_attr.toStdString() << std::endl;
////      }
////      else if (_xml.name() == "width")
////      {
////        // pass
////      }
////      else if (_xml.name() == "height")
////      {
////        // pass
////      }
////    }
////    _xml.readNext();
////  }
////
////  // TODO: call this->_manager->getController->createPaint(...)
////}

