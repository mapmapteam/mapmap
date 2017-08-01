/*
 * MetaObjectRegistry.cpp
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

#include "MetaObjectRegistry.h"

namespace mmp {

MetaObjectRegistry& MetaObjectRegistry::instance()
{
  static MetaObjectRegistry inst;
  return inst;
}

const QMetaObject* MetaObjectRegistry::getMetaObject(QString className) const
{
  QMap<QString, const QMetaObject*>::const_iterator it = metaObjectLookup.find(className);
  return (it == metaObjectLookup.constEnd() ? 0 : *it);
}

}
