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

#include "NameAllocator.h"
#include <algorithm> // std::find
#include <iostream>
#include <sstream>

static const bool VERBOSE = false;

std::string NameAllocator::allocateName(const char *prefix)
{
    int i = 0;
    while (true)
    {
        std::ostringstream tmp;
        tmp << prefix << i;
        std::string name = tmp.str();
        if (hasName(name.c_str()))
        {
            if (VERBOSE)
            {
                std::ostringstream os;
                os << "NameAllocator: Already has " << name;
                std::cout << os.str();
            }
        }
        else
        {
            names_.push_back(name);
            {
                if (VERBOSE)
                {
                    std::ostringstream os;
                    os << "NameAllocator: Allocating " << name;
                    std::cout << os.str();
                }
            }
            return name;
        }
        ++i; // important!
    }
}

bool NameAllocator::freeName(const char *name)
{
    if (hasName(name))
    {
        if (VERBOSE)
        {
            std::ostringstream os;
            os << "NameAllocator: Freeing " << name;
            std::cout << os.str();
        }
        names_.erase(std::find(names_.begin(), names_.end(),
            std::string(name)));
        return true;
    }
    else
        return false;
}

bool NameAllocator::hasName(const char *name)
{
    return std::find(names_.begin(), names_.end(), std::string(name))
        != names_.end();
}

std::vector<std::string> NameAllocator::listNames() const
{
    return names_;
}

