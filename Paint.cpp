/*
 * Paint.cpp
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

#include "Paint.h"

UidAllocator Paint::allocator;

Paint::Paint(uid id)
{
  if (id == NULL_UID)
    id = allocator.allocate();
  else
  {
    Q_ASSERT(!allocator.exists(id));
    allocator.reserve(id);
  }
  // Assign id.
  _id = id;
}

Paint::~Paint()
{
  allocator.free(_id);
}

/* Implementation of the Video class */

Video::Video(const QString uri_, uid id=NULL_UID):
    Texture(id),
    uri(uri_)
{
}

Video::~Video()
{
}

void Video::build()
{
}

int Video::getWidth() const
{
  // TODO
  return 0;
}

int Video::getHeight() const
{
  // TODO
  return 0;
}

const uchar* Video::getBits() const
{
  // TODO
  return 0;
}

