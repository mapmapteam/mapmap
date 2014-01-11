/*
 * MappingManager.cpp
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

#include "MappingManager.h"
#include <iostream>

MappingManager::MappingManager()
{
  // TODO Auto-generated constructor stub

}

std::map<uid, Mapping::ptr> MappingManager::getPaintMappings(const Paint::ptr paint) const
{
  std::map<uid, Mapping::ptr> paintMappings;
  for (std::vector<Mapping::ptr>::const_iterator it = mappingVector.begin(); it != mappingVector.end(); ++it)
  {
    if ((*it)->getPaint() == paint)
      paintMappings[(*it)->getId()] = *it;
  }
  return paintMappings;
}

std::map<uid, Mapping::ptr> MappingManager::getPaintMappingsById(uid paintId) const
{
  return getPaintMappings( paintMap.at(paintId) );
}

uid MappingManager::addPaint(Paint::ptr paint )
{
  paintVector.push_back(paint);
  paintMap[paint->getId()] = paint;
  return paint->getId();
}

uid MappingManager::addMapping(Mapping::ptr mapping)
{
  // Make sure the paint to which this mapping refers to exists in the manager.
  Q_ASSERT (std::find(paintVector.begin(), paintVector.end(), mapping->getPaint()) != paintVector.end());

  mappingVector.push_back(mapping);
  mappingMap[mapping->getId()] = mapping;

  return mapping->getId();
}

std::vector<Mapping::ptr> MappingManager::getVisibleMappings() const
{
  std::vector<Mapping::ptr> visible;

  // First pass: check if one of the mappings is in solo mode.
  bool hasSolo = false;
  for (std::vector<Mapping::ptr>::const_iterator it = mappingVector.begin(); it != mappingVector.end(); ++it)
  {
    if ((*it)->isSolo())
    {
      hasSolo = true;
      break;
    }
  }

  // Second pass: fill the visible vector.
  for (std::vector<Mapping::ptr>::const_iterator it = mappingVector.begin(); it != mappingVector.end(); ++it)
  {
    // Solo has priority over invisible (mute)
    if ( (hasSolo && (*it)->isSolo()) || (!hasSolo && (*it)->isVisible()) )
      visible.push_back(*it);
  }

  return visible;
}

void MappingManager::reorderMappings(std::vector<uid> mappingIds)
{
  Q_ASSERT( mappingIds.size() == mappingVector.size() );
  mappingVector.clear();
  for (std::vector<uid>::iterator it = mappingIds.begin(); it != mappingIds.end(); ++it)
  {
    Q_ASSERT( mappingMap.find(*it) != mappingMap.end() );
    mappingVector.push_back( mappingMap[*it] );
  }
}

//bool MappingManager::removeMapping(Mapping::ptr mapping)
//{
//}

void MappingManager::clearProject()
{
  std::cout << "TODO: implement MappingManager::clearProject() !!!" << std::endl;
  // We also need to update the GUI accordingly!
}
