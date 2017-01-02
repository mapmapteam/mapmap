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

namespace mmp {

QString Serializable::cleanClassName() const
{
  return classNameRealToClean(metaObject()->className());
}

QString Serializable::classNameRealToClean(const QString& realClassName)
{
  Q_ASSERT(realClassName.startsWith(MM::NAMESPACE_PREFIX));
  // Removes the NAMESPACE:: prefix.
  return realClassName.right( realClassName.size() - MM::NAMESPACE_PREFIX.size() );
}

QString Serializable::classNameCleanToReal(const QString& cleanClassName)
{
  Q_ASSERT(!cleanClassName.startsWith(MM::NAMESPACE_PREFIX));
  return MM::NAMESPACE_PREFIX + cleanClassName;
}

void Serializable::read(const QDomElement& obj)
{
  QList<QString> attributeNames = _propertiesAttributes();
  QList<QString> specialNames   = _propertiesSpecial();

  // Fill up properties.
  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    // Get property/tag.
    QMetaProperty property = metaObject()->property(i);

    // Name of property.
    const char* propertyName =  property.name();

    // Don't try to write special properties (leave it to children).
    if (specialNames.contains(propertyName))
      continue;

    // If property is writable, try to find it and rewrite it.
    if (property.isWritable())
    {

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
  QList<QString> specialNames   = _propertiesSpecial();

  // Write up classname.
  obj.setAttribute(ProjectLabels::CLASS_NAME, cleanClassName());

  // Fill up properties.
  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    // Get property/tag.
    QMetaProperty property = metaObject()->property(i);

    // Name of property.
    const char* propertyName =  property.name();

    // Don't try to write special properties (leave it to children).
    if (specialNames.contains(propertyName))
      continue;

    // Don't save unstored properties.
    if (!property.isStored(this))
      continue;

    // If property is writable, try to find it and rewrite it.
    if (property.isWritable() && property.isReadable())
    {
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
        _writeNode(obj, propertyName, propertyValue);
      }
    }
  }
}

void Serializable::_writeNode(QDomElement& obj, const QString& nodeName, const QString& nodeValue)
{
  QDomElement propertyNode = obj.ownerDocument().createElement(nodeName);
  QDomText    text         = obj.ownerDocument().createTextNode(nodeValue);
  propertyNode.appendChild(text);
  obj.appendChild(propertyNode);
}



}
