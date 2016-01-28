/*
 * ProjecWriter.cpp
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
#include "ProjectWriter.h"
#include <sstream>

const char* ProjectAttributes::CLASS_NAME  = "className";
const char* ProjectAttributes::PAINTS      = "paints";
const char* ProjectAttributes::MAPPINGS    = "mappings";
const char* ProjectAttributes::ID          = "id";
const char* ProjectAttributes::PAINT_ID    = "paintId";
const char* ProjectAttributes::NAME        = "name";
const char* ProjectAttributes::SOURCE      = "id";
const char* ProjectAttributes::DESTINATION = "name";

ProjectWriter::ProjectWriter(MainWindow *window) :
    _window(window)
{
  _xml.setAutoFormatting(true);
}

bool ProjectWriter::writeFile(QIODevice *device)
{
  MappingManager& manager = _window->getMappingManager();
  QDomDocument doc;
  QDomElement project = doc.createElement("project");
  project.setAttribute("version", MM::VERSION);

  // Paints.
  QDomElement paints = doc.createElement("paints");
  for (int i=0; i<manager.nPaints(); i++)
  {
    QDomElement paint = doc.createElement("paint");
    manager.getPaint(i)->write(paint);
    paints.appendChild(paint);
  }

  // Mappings.
  QDomElement mappings = doc.createElement("mappings");
  for (int i=0; i<manager.nMappings(); i++)
  {
    QDomElement mapping = doc.createElement("mapping");
    manager.getMapping(i)->write(mapping);
    mappings.appendChild(mapping);
  }

  project.appendChild(paints);
  project.appendChild(mappings);
  doc.appendChild(project);

  QTextStream out(device);
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  out << "<!DOCTYPE mapmap>" << endl;
  out << doc.toString(2);

  return true;
}

//void ProjectWriter::writeItem(Paint *item)
//{
//  _xml.writeStartElement("paint");
//  _xml.writeAttribute("id", QString::number(item->getId()));
//  //_xml.writeAttribute("name", item->getName());
//  _xml.writeAttribute("type", item->getType());
//
//  if (item->getType() == "media")
//  {
//    // FIXME: check paint type before casting to Media
//    Media *media = (Media *) item;
//
//    _xml.writeTextElement("uri", media->getUri());
//    {
//      std::ostringstream os;
//      os << media->getX();
//      _xml.writeTextElement("x", os.str().c_str());
//    }
//
//    {
//      std::ostringstream os;
//      os << media->getY();
//      _xml.writeTextElement("y", os.str().c_str());
//    }
//
//    {
//      std::ostringstream os;
//      os << media->getRate();
//      _xml.writeTextElement("rate", os.str().c_str());
//    }
//
//    _xml.writeEndElement();
//    //_xml.writeEmptyElement("hello");
//  }
//  else if (item->getType() == "image")
//  {
//    // FIXME: check paint type before casting to Image
//    Image *media = (Image *) item;
//
//    _xml.writeTextElement("uri", media->getUri());
//    {
//      std::ostringstream os;
//      os << media->getX();
//      _xml.writeTextElement("x", os.str().c_str());
//    }
//
//    {
//      std::ostringstream os;
//      os << media->getY();
//      _xml.writeTextElement("y", os.str().c_str());
//    }
//
//    _xml.writeEndElement();
//    //_xml.writeEmptyElement("hello");
//  }
//  else if (item->getType() == "color")
//  {
//    // FIXME: check paint type before casting to Image
//    Color *color = (Color *) item;
//
//    _xml.writeTextElement("rgb", color->getColor().name());
//    _xml.writeEndElement();
//  }
//  else
//    qDebug() << "Unknown type, cannot save: " << item->getType() << endl;
//}
//
//void ProjectWriter::writeShapeVertices(MShape *shape)
//{
//  if (shape->getType() == "mesh") {
//    Mesh* mesh = (Mesh*) shape;
//    _xml.writeStartElement("dimensions");
//    _xml.writeAttribute("columns", QString::number(mesh->nColumns()));
//    _xml.writeAttribute("rows", QString::number(mesh->nRows()));
//    _xml.writeEndElement(); // vertex
//  }
//
//  for (int i = 0; i < shape->nVertices(); i++)
//  {
//    const QPointF& point = shape->getVertex(i);
//    _xml.writeStartElement("vertex");
//    _xml.writeAttribute("x", QString::number(point.x()));
//    _xml.writeAttribute("y", QString::number(point.y()));
//    _xml.writeEndElement(); // vertex
//  }
//}
//
//void ProjectWriter::writeItem(Mapping *item)
//{
//  _xml.writeStartElement("mapping");
//  qDebug() << "ID: " << item->getId() << endl;
//  _xml.writeAttribute("id", QString::number(item->getId()));
//  _xml.writeAttribute("paint_id", QString::number(item->getPaint()->getId()));
//  _xml.writeAttribute("type", item->getType());
//
//  // boolean attributes:
//  _xml.writeAttribute("locked", QString::number((int) item->isLocked() ? 1 : 0));
//  _xml.writeAttribute("solo", QString::number((int) item->isSolo() ? 1 : 0));
//  _xml.writeAttribute("visible", QString::number((int) item->isVisible() ? 1 : 0));
//
//  MShape *shape = item->getShape().data();
//  _xml.writeStartElement("destination");
//  _xml.writeAttribute("shape", shape->getType());
//  writeShapeVertices(shape);
//  _xml.writeEndElement(); // shape
//
//  if (item->getType().endsWith("_texture"))
//  {
//    TextureMapping *tex = (TextureMapping *) item;
//    shape = tex->getInputShape().data();
//
//    _xml.writeStartElement("source");
//    _xml.writeAttribute("shape", shape->getType());
//    writeShapeVertices(shape);
//    _xml.writeEndElement(); // shape
//
//    _xml.writeEndElement(); // mapping
//  }
//  else if (item->getType().endsWith("_color"))
//  {
//    _xml.writeEndElement(); // mapping
//  }
//  else
//    qDebug() << "Unknown type, cannot save: " << item->getType() << endl;
//}
//
