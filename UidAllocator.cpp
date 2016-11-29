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

#include "UidAllocator.h"
#include <algorithm> // std::find
#include <iostream>
#include <sstream>

namespace mmp {

uid UidAllocator::allocate()
{
  // Iterate until we find an id that has not been taken.
  uid id = 1;
  while (exists(id))
    id++;

  // Add id to the list.
  _ids.push_back(id);

  // Return it.
  return id;
}

bool UidAllocator::reserve(uid id)
{
  if (exists(id))
    return false;
  else
  {
    _ids.push_back(id);
    return true;
  }
}

bool UidAllocator::free(uid id)
{
  std::vector<uid>::iterator it = std::find(_ids.begin(), _ids.end(), id);
  if (it != _ids.end())
  {
    _ids.erase(it);
    return true;
  }
  else
    return false;
}

bool UidAllocator::exists(uid id) const
{
  return ( std::find(_ids.begin(), _ids.end(), id) != _ids.end() );
}

}
