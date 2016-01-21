/*
 * Element.cpp
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

#include "Element.h"

#include <QDebug>

Element::Element(uid id, UidAllocator* allocator) : _name(""), _isLocked(false), _opacity(1.0f), _allocator(allocator)
{
  qDebug() << "Trying to create Element with allocator " << allocator << " and id " << id << endl;
  if (id == NULL_UID)
    id = allocator->allocate();
  else
  {
    Q_ASSERT(!allocator->exists(id));
    allocator->reserve(id);
  }
  // Assign id.
  _id = id;
  // Reset name.
  unsetName();
}

Element::~Element() {
  _allocator->free(_id);
}

