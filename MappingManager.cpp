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
  std::string name = _nameAllocator.allocateName("image_");
  Image* img = new Image(name.c_str(), imagePath);

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
  layerVector.push_back(layer);
  layerMap[layer->getId()] = layer;
  return layer->getId();
}

std::vector<Layer::ptr> MappingManager::getVisibleLayers() const
{
  std::vector<Layer::ptr> visible;
  bool hasSolo = false;
  for (std::vector<Layer::ptr>::const_iterator it = layerVector.begin(); it != layerVector.end(); ++it)
  {
    if ((*it)->isSolo())
    {
      hasSolo = true;
      break;
    }
  }
  for (std::vector<Layer::ptr>::const_iterator it = layerVector.begin(); it != layerVector.end(); ++it)
  {
    // Solo has priority over invisible (mute)
    if ( (hasSolo && (*it)->isSolo()) || (!hasSolo && (*it)->isVisible()) )
      visible.push_back(*it);
  }
  return visible;
}

void MappingManager::reorderLayers(std::vector<uint> layerIds)
{
  Q_ASSERT( layerIds.size() == layerVector.size() );
  // TODO: do a better check than this...
  layerVector.clear();
  for (std::vector<uint>::iterator it = layerIds.begin(); it != layerIds.end(); ++it)
  {
    Q_ASSERT( layerMap.find(*it) != layerMap.end() );
    layerVector.push_back( layerMap[*it] );
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
