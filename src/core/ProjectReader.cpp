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
#include <iostream>

namespace mmp {

ProjectReader::ProjectReader(MainWindow *window) : _window(window)
{
}

bool ProjectReader::isValidVersion(const QString& versionString)
{
    QRegularExpression re(MM::SUPPORTED_FILE_VERSIONS);
    QRegularExpressionMatch match = re.match(versionString);
    return match.hasMatch();
}

bool ProjectReader::readFile(QIODevice *device)
{
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(device->readAll(), &parseError);
  if (doc.isNull()) {
    _errorString = QString("Parse error at offset %1: %2")
        .arg(parseError.offset).arg(parseError.errorString());
    return false;
  }

  QJsonObject root = doc.object();
  QString projectVersion = root["version"].toString();

  if (projectVersion.isEmpty()) {
    _errorString = QObject::tr("The contents of this file does not look like a MapMap project.");
    return false;
  } else if (!isValidVersion(projectVersion)) {
    _errorString = QObject::tr("The version of MapMap %1 used to save this file is not readable by this MapMap version %2.")
        .arg(projectVersion, MM::VERSION);
    return false;
  }

  parseProject(root);

  return _errorString.isEmpty();
}

QString ProjectReader::errorString() const
{
  return _errorString;
}

void ProjectReader::parseProject(const QJsonObject& project)
{
  MappingManager& manager = _window->getMappingManager();
  manager.clearAll();

  QJsonArray sources = project[ProjectLabels::SOURCES].toArray();
  QJsonArray layers  = project[ProjectLabels::LAYERS].toArray();

  // Parse sources (formerly sources).
  for (const auto& val : sources)
  {
    Source::ptr source = parseSource(val.toObject());

    if (source.isNull())
    {
      qDebug() << "Problem creating source." << Qt::endl;
    }
    else
    {
      manager.addSource(source);
      _window->addSourceItem(source->getId(), source->getIcon(), source->getName());

      // Locate media file if not found
      if (source->getSourceType() == Source::SourceType::Video)
      {
        QSharedPointer<Video> media = qSharedPointerCast<Video>(source);
        Q_CHECK_PTR(media);
        if (!_window->fileExists(media->getUri()))
          media->setUri(_window->locateMediaFile(media->getUri(), false));
      }
      if (source->getSourceType() == Source::SourceType::Image)
      {
        QSharedPointer<Image> image = qSharedPointerCast<Image>(source);
        Q_CHECK_PTR(image);
        if (!_window->fileExists(image->getUri()))
          image->setUri(_window->locateMediaFile(image->getUri(), true));
      }
    }
  }

  // Parse layers (formerly mappings).
  QVector<Layer::ptr> allLayers;
  for (const auto& val : layers)
  {
    Layer::ptr layer = parseLayer(val.toObject());
    if (layer.isNull())
    {
      qDebug() << "Problem creating layer." << Qt::endl;
    }
    else
    {
      allLayers.push_back(layer);
    }
  }

  // Add all mappings in reverse order.
  for (QVector<Layer::ptr>::const_reverse_iterator it = allLayers.rbegin();
          it != allLayers.rend(); ++it)
  {
    manager.addLayer(*it);
    _window->addLayerItem((*it)->getId());
  }
}

Source::ptr ProjectReader::parseSource(const QJsonObject& obj)
{
  QString className = Serializable::classNameCleanToReal(obj[ProjectLabels::CLASS_NAME].toString());
  int id            = obj[ProjectLabels::ID].toInt(NULL_UID);

  qDebug() << "Found source with classname: " << className << Qt::endl;

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    Source::ptr source(qobject_cast<Source*>(metaObject->newInstance(Q_ARG(int, id))));

    if (source.isNull())
    {
      qDebug() << QObject::tr("Problem at creation of source.") << Qt::endl;
    }
    else
      qDebug() << "Created new instance with id: " << source->getId();

    source->read(obj);

    return source;
  }
  else
  {
    // Unknown source type (e.g. a macOS-only Syphon source opened on another
    // platform). Skip it with a warning rather than failing the whole load.
    qWarning() << "Skipping unsupported source of type" << className;
    return Source::ptr();
  }
}

Layer::ptr ProjectReader::parseLayer(const QJsonObject& obj)
{
  QString className = Serializable::classNameCleanToReal(obj[ProjectLabels::CLASS_NAME].toString());
  int id            = obj[ProjectLabels::ID].toInt(NULL_UID);

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    Layer::ptr layer(qobject_cast<Layer*>(metaObject->newInstance(Q_ARG(int, id))));
    if (layer.isNull())
    {
      qDebug() << QObject::tr("Problem at creation of layer.") << Qt::endl;
    }

    layer->read(obj);

    return layer;
  }
  else
  {
    _errorString = QObject::tr("Unable to create layer of type '%1'.").arg(className);
    return Layer::ptr();
  }
}

}
