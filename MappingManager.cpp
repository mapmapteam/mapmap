/*
 * MappingManager.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

std::map<uint, Mapping::ptr> MappingManager::getPaintMappings(const Paint::ptr paint) const
{
  std::map<uint, Mapping::ptr> paintMappings;
  for (std::vector<Mapping::ptr>::const_iterator it = mappingVector.begin(); it != mappingVector.end(); ++it)
  {
    if ((*it)->getPaint() == paint)
      paintMappings[(*it)->getId()] = *it;
  }
  return paintMappings;
}

std::map<uint, Mapping::ptr> MappingManager::getPaintMappingsById(uint paintId) const
{
  return getPaintMappings( paintMap.at(paintId) );
}

uint MappingManager::addPaint(Paint::ptr paint)
{
  paintVector.push_back(paint);
  paintMap[paint->getId()] = paint;
  return paint->getId();
}

uint MappingManager::addImage(const QString imagePath, int frameWidth, int frameHeight)
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

uint MappingManager::addMapping(Mapping::ptr mapping)
{
  // Make sure the paint to which this mapping refers to exists in the manager.
  Q_ASSERT (std::find(paintVector.begin(), paintVector.end(), mapping->getPaint()) != paintVector.end());

  mappingVector.push_back(mapping);
  mappingMap[mapping->getId()] = mapping;

  return mapping->getId();
}

uint MappingManager::addLayer(Mapping::ptr mapping)
{
  addMapping(mapping);
  Layer::ptr layer(new Layer);
  layer->setMapping(mapping);
  layers.push_back(layer);
  return layer->getId();
}

//bool MappingManager::removeMapping(Mapping::ptr mapping)
//{
//}
