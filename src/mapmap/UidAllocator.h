/*
 * UidAllocator.cpp
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

/**
 * The UidAllocator class. Allocates unique IDs that can be used to uniquely identify
 * elements from collections of objects.
 */

#ifndef __UID_ALLOCATOR_H__
#define __UID_ALLOCATOR_H__

#include <string>
#include <vector>
#include "MM.h"

/// A UID in Libremapping is represented as a integer.
typedef int uid;

/// Represents a UID that does not refer to any entity.
#define NULL_UID (0)

namespace mmp {

/**
 * Allocates uids for instances by appending an incremental number to a given string.
 * Manages a pool of unique names.
 */
class UidAllocator
{
public:
  uid allocate();
  bool free(uid id);
  bool reserve(uid id);
  bool exists(uid id) const;
  std::vector<uid> list() const { return _ids; }

private:
  std::vector<uid> _ids;
};

}

#endif // ifndef
