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
  QString className = paintElem.attribute(ProjectLabels::CLASS_NAME);
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
  QString className = mappingElem.attribute(ProjectLabels::CLASS_NAME);
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


