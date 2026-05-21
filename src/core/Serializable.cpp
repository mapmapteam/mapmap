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

void Serializable::read(const QJsonObject& obj)
{
  QList<QString> specialNames = _propertiesSpecial();

  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    QMetaProperty property = metaObject()->property(i);
    const char* propertyName = property.name();

    if (specialNames.contains(propertyName))
      continue;

    if (property.isWritable())
    {
      if (QString(propertyName) == QString("objectName"))
        continue;

      if (obj.contains(propertyName))
        setProperty(propertyName, obj[propertyName].toString());
    }
  }
}

void Serializable::write(QJsonObject& obj)
{
  QList<QString> specialNames = _propertiesSpecial();

  obj[ProjectLabels::CLASS_NAME] = cleanClassName();

  int count = metaObject()->propertyCount();
  for (int i=0; i<count; ++i) {
    QMetaProperty property = metaObject()->property(i);
    const char* propertyName = property.name();

    if (specialNames.contains(propertyName))
      continue;

    if (!property.isStored())
      continue;

    if (property.isWritable() && property.isReadable())
    {
      if (QString(propertyName) == QString("objectName"))
        continue;

      obj[propertyName] = property.read(this).toString();
    }
  }
}

}
