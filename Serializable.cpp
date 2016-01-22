/*
 * Serializable.cpp
 *
 * (c) 2016 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "Serializable.h"

void Serializable::read(const QDomElement& obj)
{
  QList<QString> attributeNames = _propertiesAttributes();

  // Fill up properties.
  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    // Get property/tag.
    QMetaProperty property = metaObject()->property(i);

    // If property is writable, try to find it and rewrite it.
    if (property.isWritable())
    {
      // Name of property.
      const char* propertyName =  property.name();

      // Always ignore objectName default property.
      if (QString(propertyName) == QString("objectName"))
        continue;

      // Check in attributes.
      else if (attributeNames.contains(propertyName))
      {
        if (obj.hasAttribute(propertyName))
          setProperty(propertyName, obj.attribute(propertyName));
      }

      // Check in children.
      else
      {
        // Find element.
        QDomElement propertyElem = obj.firstChildElement(propertyName);
        if (!propertyElem.isNull())
        {
          // Set property.
          setProperty(propertyName, propertyElem.text());
        }
      }
    }
  }
}

void Serializable::write(QDomElement& obj)
{
  QList<QString> attributeNames = _propertiesAttributes();

  // Write up classname.
  obj.setAttribute("className", metaObject()->className());

  // Fill up properties.
  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    // Get property/tag.
    QMetaProperty property = metaObject()->property(i);

    // Don't save unstored properties.
    if (!property.isStored(this))
      continue;

    // If property is writable, try to find it and rewrite it.
    if (property.isWritable() && property.isReadable())
    {
      // Name of property.
      const char* propertyName =  property.name();
      qDebug() << "Read " << propertyName << " : " << property.read(this) << endl;
      QString propertyValue = property.read(this).toString();

      // Always ignore objectName default property.
      if (QString(propertyName) == QString("objectName"))
        continue;

      // Add to attributes.
      if (attributeNames.contains(propertyName))
      {
        obj.setAttribute(propertyName, propertyValue);
      }

      // Add to children.
      else
      {
        QDomElement propertyNode = obj.ownerDocument().createElement(propertyName);
        QDomText    text         = obj.ownerDocument().createTextNode(propertyValue);
        propertyNode.appendChild(text);
        obj.appendChild(propertyNode);
      }
    }
  }
}

