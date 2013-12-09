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

ProjectReader::ProjectReader(MappingManager *manager) :
    _manager(manager)
{
}

bool ProjectReader::readFile(QIODevice *device)
{
  _xml.setDevice(device);
  if (_xml.readNextStartElement())
  {
    if (_xml.name() == "project" && _xml.attributes().value("version") == "1.0")
      readProject();
    else
      _xml.raiseError(QObject::tr("The file is not a libremapping version 1.0 file."));
  }
  return ! _xml.error();
}

QString ProjectReader::errorString() const
{
  return QObject::tr("%1\nLine %2, column %3")
    .arg(_xml.errorString())
    .arg(_xml.lineNumber())
    .arg(_xml.columnNumber());
}

void ProjectReader::readProject()
{
  // FIXME: avoid asserts
  Q_ASSERT(_xml.isStartElement() && _xml.name() == "project");

  while (_xml.readNextStartElement() && ! _xml.atEnd())
  {
    if (_xml.name() == "paint")
      readPaint(); //NULL);
    if (_xml.name() == "mapping")
      readMapping(); // NULL);
    else
    {
      std::cout << "skip element " << _xml.name().string() << std::endl;
      _xml.skipCurrentElement();
    }
  }
}

void ProjectReader::readMapping()
{
  // FIXME: we assume an Image mapping 
  Q_ASSERT(_xml.isStartElement() && _xml.name() == "mapping");
  const QString *paintIdAttrValue;
  QXmlStreamAttributes attributes = _xml.attributes();

  if (attributes.hasAttribute("", "paint_id"))
    paintIdAttrValue = attributes.value("", "paint_id").string();

  std::cout << "Found mapping " << "with paint ID " << paintIdAttrValue << std::endl;

  //QString title = _xml.readElementText();
  //item->setText(0, title);
}

void ProjectReader::readPaint()
{
  // FIXME: we assume an Image mapping 
  Q_ASSERT(_xml.isStartElement() && _xml.name() == "paint");
  const QString *paintIdAttrValue;
  const QString *typeAttrValue;
  QXmlStreamAttributes attributes = _xml.attributes();

  if (attributes.hasAttribute("", "name"))
    paintIdAttrValue = attributes.value("", "name").string();
  if (attributes.hasAttribute("", "type"))
    typeAttrValue = attributes.value("", "type").string();

  std::cout << "Found " << typeAttrValue << " paint " << paintIdAttrValue << std::endl;
}
