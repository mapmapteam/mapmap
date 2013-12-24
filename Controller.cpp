/*
 * Controller.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2013 Vasilis Liaskovitis -- vliaskov@gmail.com
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
#include "Controller.h"
#include <QMetaObject>
#include <QMetaType>
#include <iostream>

template<class T> void Controller::registerObjType(const char* objType)
{
  _mControllerTypes[objType] = &(T::staticMetaObject);
}

bool Controller::createObject(const char* objType, const char* objName)
{
  const QMetaObject *meta = _mControllerTypes[objType];

  if (meta)
  {  
    _mControllerObjects[objName] = meta->newInstance();
    //std::cout << "object creation " << objName << " succeeded\n";
    return true;
  }
  //std::cout << "object creation " << objName << " failed\n";
  return false;
}

bool Controller::setObjectProperty(const char* objName, const char* propName,
            const QVariant &value)
{
  QObject *obj = _mControllerObjects[objName];
  if (obj)
  {
    if (obj->setProperty(propName, value))
    {
      //std::cout << "setproperty succeeded " << propName << "\n";
      return true;
    }
    else
    {
      //std::cout << "setproperty failed " << propName << "\n";
      return false;
    }
  }  
  //std::cout << "object " << objName << " could not be found\n";
  return false;
}

bool Controller::getObjectProperty(const char *objName, const char *propName,
            QVariant &value)
{
  QObject *obj = _mControllerObjects[objName];
  if (obj)
  {
      value = obj->property(propName);
      return value.isValid();
  }
  //std::cout << "Failed to get property " << propName << "\n";
  return false;
}

bool Controller::listObjectProperties(const char *objName, QList<QString>
    &names, QVariantList &values)
{
  QObject *obj = _mControllerObjects[objName];
  if (obj)
  {
    const QMetaObject *metaobject = obj->metaObject();
    int count = metaobject->propertyCount();
    for (int i = 0; i < count; ++i)
    {
      QMetaProperty metaproperty = metaobject->property(i);
      const char *propname = metaproperty.name();
      names.append(QString(propname));
      values.append(obj->property(propname));
    }
    return true;
  }
  return false;
}

bool Controller::listObjects(const char *objType, QList<QString> &objNames)
{
  const QMetaObject *meta = _mControllerTypes[objType];
  for (std::map<const char*, QObject*>::iterator i = _mControllerObjects.begin();
      i != _mControllerObjects.end(); i++)
  {
    const QObject *obj = i->second;
    const char *name = i->first;
    const QMetaObject *metaobject = obj->metaObject();
    if (meta == metaobject)
    {
      //std::cout << "found object " << name << " of type " << objType <<  " \n";
      objNames.append(name);
    }
  }
  return true;
}

Controller::Controller(MainWindow *owner)
{
  _owner = owner;
  /* Register common types for properties
   * FIXME, we can provide a method for other classes to register types of their
   * properties instead of doing it in the controller constructor */
  qRegisterMetaType<int>("int");
  qRegisterMetaType<qreal>("qreal");
  registerObjType<Point>("Point");
}
