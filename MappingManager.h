/*
 * MappingManager.h
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

#ifndef MAPPINGMANAGER_H_
#define MAPPINGMANAGER_H_

#include "Paint.h"
#include "Mapping.h"
#include "Layer.h"

/**
 * This is a container class for all the paints and mappings. It is on the model
 * side.
 */
class MappingManager
{
public:
  MappingManager();

private:
  // Model.
  std::vector<Paint::ptr> paintVector;
  std::map<uint, Paint::ptr> paintMap;
  std::vector<Mapping::ptr> mappingVector;
  std::map<uint, Mapping::ptr> mappingMap;

  std::vector<Layer::ptr> layerVector;
  std::map<uint, Layer::ptr> layerMap;

public:
  /// Returns the list of mappings associated with given paint.
  std::map<uint, Mapping::ptr> getPaintMappings(const Paint::ptr paint) const;
  std::map<uint, Mapping::ptr> getPaintMappingsById(uint paintId) const;

  uint addPaint(Paint::ptr paint);
//  bool removePaint(Paint::ptr paint);
  int nPaints() const { return paintVector.size(); }
  Paint::ptr getPaint(int i) { return paintVector[i]; }
  Paint::ptr getPaintById(uint id) { return paintMap[id]; }

  uint addImage(const QString imagePath, int frameWidth, int frameHeight);

  uint addMapping(Mapping::ptr mapping);
//  bool removeMapping(Mapping::ptr mapping);
  int nMappings() const { return mappingVector.size(); }
  Mapping::ptr getMapping(int i) { return mappingVector[i]; }
  Mapping::ptr getMappingById(uint id) { return mappingMap[id]; }

  int nLayers() const { return layerVector.size(); }
  uint addLayer(Mapping::ptr mapping);
  Layer::ptr getLayer(int i) { return layerVector[i]; }
  Layer::ptr getLayerById(uint id) { return layerMap[id]; }

  std::vector<Layer::ptr> getVisibleLayers() const;
};

#endif /* MAPPINGMANAGER_H_ */
