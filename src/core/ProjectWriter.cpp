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
#include <QDir>
#include <QFileInfo>

namespace mmp {

ProjectWriter::ProjectWriter(MappingManager& manager) :
    _manager(manager)
{
}

bool ProjectWriter::writeFile(QIODevice* device, const QString& projectDirectory)
{
  MappingManager& manager = _manager;
  QJsonObject project;
  project["version"] = ProjectLabels::FORMAT_VERSION;

  if (manager.hasOutputResolution())
  {
    QJsonObject resolution;
    resolution[ProjectLabels::WIDTH]  = manager.getOutputResolution().width();
    resolution[ProjectLabels::HEIGHT] = manager.getOutputResolution().height();
    project[ProjectLabels::OUTPUT_RESOLUTION] = resolution;
  }

  // Sources (formerly paints).
  QJsonArray sources;
  for (int i=0; i<manager.nSources(); i++)
  {
    QJsonObject source;
    const Source::ptr entry = manager.getSource(i);
    entry->write(source);
    const Video* video = qobject_cast<const Video*>(entry.data());
    const Texture* media = qobject_cast<const Image*>(entry.data());
    if (video && !video->isCamera())
      media = video;
    QString path = source.value(ProjectLabels::URI).toString();
    if (media && !projectDirectory.isEmpty() && !path.isEmpty())
    {
      if (Texture::isRelativeMediaPath(path))
        path = QFileInfo(path).absoluteFilePath();
      // Preserve absolute paths belonging to a different operating system.
      if (QDir::isAbsolutePath(path))
      {
        const QString relative = QDir(projectDirectory).relativeFilePath(path);
        source[ProjectLabels::URI] = media->prefersRelativeMediaPath() || !relative.startsWith("../")
            ? relative : path;
      }
    }
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