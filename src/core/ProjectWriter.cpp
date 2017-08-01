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

namespace mmp {

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

}