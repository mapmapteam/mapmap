/*
 * MetaObjectRegistry.h
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

#ifndef METAOBJECTREGISTRY_H_
#define METAOBJECTREGISTRY_H_

#include <QObject>
#include <QMap>
#include <QList>
#include "MM.h"

namespace mmp {

class MetaObjectRegistry {
private:
  QList<const QMetaObject*> metaObjectList;
  QMap<QString, const QMetaObject*> metaObjectLookup;

public:
  MetaObjectRegistry() {}
  virtual ~MetaObjectRegistry() {}

  template<class T> void add()
  {
    //  Q_UNUSED(obj);
    const QMetaObject* metaObj = &T::staticMetaObject;
    if (!metaObjectList.contains(metaObj))
    {
      metaObjectList << metaObj;
      metaObjectLookup[metaObj->className()] = metaObj;
    }
  }

  const QMetaObject* getMetaObject(QString className) const;

  static MetaObjectRegistry& instance();

};

}

#endif /* METAOBJECTREGISTRY_H_ */
