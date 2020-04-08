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
  QString projectVersion = root.attribute("version");
  // The handling of the version number will get fancier as we go.
  if (root.tagName() != "project") {
    _xml.raiseError(QObject::tr("The contents of this file does not look like a MapMap project."));
    return false;
  } else if (! this->isValidVersion(projectVersion)) {
    _xml.raiseError(
        QObject::tr("The version of MapMap %1 used to save this file is not readable by this MapMap version %2.").arg(
            projectVersion, MM::VERSION));
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

  QDomElement paints = project.firstChildElement(ProjectLabels::PAINTS);
  QDomElement mappings = project.firstChildElement(ProjectLabels::MAPPINGS);

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

      // Locate media file if not found
      if (paint->getSourceType() == Paint::SourceType::Video)
      {
        QSharedPointer<Video> media = qSharedPointerCast<Video>(paint);
        Q_CHECK_PTR(media);
        if (!_window->fileExists(media->getUri()))
          media->setUri(_window->locateMediaFile(media->getUri(), false));
      }
      if (paint->getSourceType() == Paint::SourceType::Image)
      {
        QSharedPointer<Image> image = qSharedPointerCast<Image>(paint);
        Q_CHECK_PTR(image);
        if (!_window->fileExists(image->getUri()))
          image->setUri(_window->locateMediaFile(image->getUri(), true));
      }
    }
    paintNode = paintNode.nextSibling();
  }

  // Parse mappings.
  QDomNode mappingNode = mappings.firstChild();
  QVector<Mapping::ptr> allMappings;
  while (!mappingNode.isNull())
  {
    Mapping::ptr mapping = parseMapping(mappingNode.toElement());
    if (mapping.isNull())
    {
      qDebug() << "Problem creating mapping." << endl;
    }
    else
    {
      allMappings.push_back(mapping);
    }

    mappingNode = mappingNode.nextSibling();
  }

  // Add all mappings in reverse order.
  for (QVector<Mapping::ptr>::const_reverse_iterator it = allMappings.rbegin();
          it != allMappings.rend(); ++it)
  {
    manager.addMapping(*it);
    _window->addMappingItem((*it)->getId());
  }
}

Paint::ptr ProjectReader::parsePaint(const QDomElement& paintElem)
{
  QString className = Serializable::classNameCleanToReal(paintElem.attribute(ProjectLabels::CLASS_NAME));
  int id            = paintElem.attribute(ProjectLabels::ID, QString::number(NULL_UID)).toInt();

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
  QString className = Serializable::classNameCleanToReal(mappingElem.attribute(ProjectLabels::CLASS_NAME));
  int id            = mappingElem.attribute(ProjectLabels::ID, QString::number(NULL_UID)).toInt();

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

    mapping->read(mappingElem);

    return mapping;
  }

  else
  {
    _xml.raiseError(QObject::tr("Unable to create paint of type '%1'.").arg(className));
    return Mapping::ptr();
  }
}

}
