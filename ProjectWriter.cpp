/*
 * MainWindow.cpp
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

ProjectWriter::ProjectWriter(MappingManager *manager) :
    _manager(manager)
{
  _xml.setAutoFormatting(true);
}

bool ProjectWriter::writeFile(QIODevice *device)
{
  _xml.setDevice(device);
  
  _xml.writeStartDocument();
  _xml.writeDTD("<!DOCTYPE libremapping>");
  _xml.writeStartElement("project");
  _xml.writeAttribute("version", "1.0");

  for (int i = 0; i < _manager->nPaints(); i++)
    writeItem(_manager->getPaint(i).get());

  for (int i = 0; i < _manager->nMappings(); i++)
    writeItem(_manager->getMapping(i).get());

  _xml.writeEndDocument();
  return true;
}

void ProjectWriter::writeItem(Paint *item)
{
  _xml.writeStartElement("paint");
  _xml.writeAttribute("name", item->getName());
  _xml.writeAttribute("type", "image");

  // FIXME: check paint type before casting to Image
  Image *image = (Image *) item;

  _xml.writeTextElement("uri", image->getImagePath());
  {
    std::ostringstream os;
    os << image->getWidth();
    _xml.writeTextElement("width", os.str().c_str());
  }

  {
    std::ostringstream os;
    os << image->getHeight();
    _xml.writeTextElement("height", os.str().c_str());
  }

  _xml.writeEndElement();
  //_xml.writeEmptyElement("hello");
}

void ProjectWriter::writeShapeVertices(Shape *shape)
{
  for (int i = 0; i < shape->nVertices(); i++)
  {
    const Point & point = shape->getVertex(i);
    _xml.writeStartElement("vertex");
    {
      std::ostringstream os;
      os << point.x;
      _xml.writeAttribute("x", os.str().c_str());
    }
    {
      std::ostringstream os;
      os << point.y;
      _xml.writeAttribute("y", os.str().c_str());
    }
    _xml.writeEndElement(); // vertex
  }
}

void ProjectWriter::writeItem(Mapping *item)
{
  _xml.writeStartElement("mapping");
  _xml.writeAttribute("paint_id", item->getPaint()->getName());

  Shape *shape = item->getShape().get();
  _xml.writeStartElement("destination");
  _xml.writeAttribute("shape", shape->getShapeType());
  writeShapeVertices(shape);
  _xml.writeEndElement(); // shape

  // FIXME: check mapping type before casting to TextureMapping
  TextureMapping *tex = (TextureMapping *) item;
  shape = tex->getInputShape().get();
  _xml.writeStartElement("source");
  _xml.writeAttribute("shape", shape->getShapeType());
  writeShapeVertices(shape);
  _xml.writeEndElement(); // shape

  _xml.writeEndElement(); // mapping
}

