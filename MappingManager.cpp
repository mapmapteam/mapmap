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

MappingManager::MappingManager()
{
  // TODO Auto-generated constructor stub

}

std::map<int, Mapping::ptr> MappingManager::getPaintMappings(const Paint::ptr paint) const
{
  std::map<int, Mapping::ptr> paintMappings;
  for (int i=0; i<mappings.size(); i++)
  {
    if (mappings[i]->getPaint() == paint)
      paintMappings[i] = mappings[i];
  }
  return paintMappings;
}

std::map<int, Mapping::ptr> MappingManager::getPaintMappings(int i) const
{
  return getPaintMappings( paints[i] );
}

int MappingManager::addPaint(Paint::ptr paint)
{
  paints.push_back(paint);
  return paints.size()-1;
}

int MappingManager::addImage(const QString imagePath, int frameWidth, int frameHeight)
{
  Image* img  = new Image(imagePath);

  img->setPosition( (frameWidth  - img->getWidth() ) / 2.0f,
                    (frameHeight - img->getHeight()) / 2.0f );

  return addPaint(Paint::ptr(img));
}

//bool MappingManager::removePaint(Paint::ptr paint)
//{
//
//}

int MappingManager::addMapping(Mapping::ptr mapping)
{
  if (std::find(paints.begin(), paints.end(), mapping->getPaint()) != paints.end())
  {
    mappings.push_back(mapping);
    return mappings.size()-1;
  }
  else
    return (-1);
}

//bool MappingManager::removeMapping(Mapping::ptr mapping)
//{
//}
