/*
 * NameAllocator.cpp
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
 * The NameAllocator class.
 */

#ifndef __NAME_ALLOCATOR_H__
#define __NAME_ALLOCATOR_H__

#include <string>
#include <vector>

/**
 * Allocates names for instances by appending an incremental number to a given string.
 * Manages a pool of unique names.
 */
class NameAllocator 
{
    public:
        std::string allocateName(const char *prefix);
        bool freeName(const char *name);
        bool hasName(const char *name);
        std::vector<std::string> listNames() const;
    private:
        std::vector<std::string> names_;
};

#endif // ifndef

