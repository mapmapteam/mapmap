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

  while(! _xml.atEnd() && ! _xml.hasError())
  {
    /* Read next element.*/
    QXmlStreamReader::TokenType token = _xml.readNext();
    /* If token is just StartDocument, we'll go to next.*/
    if (token == QXmlStreamReader::StartDocument)
    {
      continue;
    }
    /* If token is StartElement, we'll see if we can read it.*/
    else if (token == QXmlStreamReader::StartElement)
    {
      if (_xml.name() == "paints")
        continue;
      else if (_xml.name() == "paint")
      {
        std::cout << " * paint" << std::endl;
        readPaint();
      }
      else if (_xml.name() == "mappings")
        continue;
      else if (_xml.name() == "mapping")
      {
        std::cout << " * mapping " << std::endl;
        readMapping(); // NULL);
      }
      else
      {
        std::cout << " * skip element " << _xml.name().string() << std::endl;
        //_xml.skipCurrentElement();
      }
    }
  } // while
  _xml.clear();
}


void ProjectReader::readMapping()
{
  // FIXME: we assume an Image mapping 
  Q_ASSERT(_xml.isStartElement() && _xml.name() == "mapping");
  const QString *paintIdAttrValue;
  QXmlStreamAttributes attributes = _xml.attributes();

  if (attributes.hasAttribute("", "paint_id"))
    paintIdAttrValue = attributes.value("", "paint_id").string();

  std::cout << "   * <mapping> " << "with paint ID " << paintIdAttrValue << std::endl;

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

  std::cout << "Found " << typeAttrValue->toStdString() <<
    " paint " << paintIdAttrValue->toStdString() << std::endl;

  /* Next element... */
  _xml.readNext();
  // /*
  //  * We're going to loop over the things because the order might change.
  //  * We'll continue the loop until we hit an EndElement named person.
  //  */
  while(! (_xml.tokenType() == QXmlStreamReader::EndElement &&
    _xml.name() == "paint"))
  {
    if (_xml.tokenType() == QXmlStreamReader::StartElement)
    {
      if (_xml.name() == "uri")
      {
        /* ...go to the next. */
        _xml.readNext();
        /*
         * This elements needs to contain Characters so we know it's
         * actually data, if it's not we'll leave.
         */
        if(_xml.tokenType() != QXmlStreamReader::Characters)
        {
          // pass
        }
        std::cout << "uri " << _xml.text().toString().toStdString() << std::endl;
      }
      else if (_xml.name() == "width")
      {
        // pass
      }
      else if (_xml.name() == "height")
      {
        // pass
      }
    }
    _xml.readNext();
  }
}
