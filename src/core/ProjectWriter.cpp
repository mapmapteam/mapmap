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

namespace mmp {

ProjectWriter::ProjectWriter(MainWindow *window) :
    _window(window)
{
}

bool ProjectWriter::writeFile(QIODevice *device)
{
  MappingManager& manager = _window->getMappingManager();
  QJsonObject project;
  project["version"] = MM::VERSION;

  // Sources (formerly paints).
  QJsonArray sources;
  for (int i=0; i<manager.nSources(); i++)
  {
    QJsonObject source;
    manager.getSource(i)->write(source);
    sources.append(source);
  }
  project[ProjectLabels::SOURCES] = sources;

  // Layers (formerly mappings).
  QJsonArray layers;
  for (int i=0; i<manager.nLayers(); i++)
  {
    QJsonObject layer;
    manager.getLayer(i)->write(layer);
    layers.append(layer);
  }
  project[ProjectLabels::LAYERS] = layers;

  QJsonDocument doc(project);
  device->write(doc.toJson(QJsonDocument::Indented));

  return true;
}

}