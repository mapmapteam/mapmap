/*
 * MappingManager.h
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

#ifndef MAPPINGMANAGER_H_
#define MAPPINGMANAGER_H_

#include "Paint.h"
#include "Mapping.h"
#include "NameAllocator.h"

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
  std::vector<Paint::ptr> paints;
  std::vector<Mapping::ptr> mappings;
  NameAllocator _nameAllocator;

public:
  /// Returns the list of mappings associated with given paint.
  std::map<int, Mapping::ptr> getPaintMappings(const Paint::ptr paint) const;
  std::map<int, Mapping::ptr> getPaintMappings(int id) const;

  int addPaint(Paint::ptr paint);
//  bool removePaint(Paint::ptr paint);
  int nPaints() const { return paints.size(); }
  Paint::ptr getPaint(int i) { return paints[i]; }

  int addImage(const QString imagePath, int frameWidth, int frameHeight);

  int addMapping(Mapping::ptr mapping);
//  bool removeMapping(Mapping::ptr mapping);
  int nMappings() const { return mappings.size(); }
  Mapping::ptr getMapping(int i) { return mappings[i]; }
  void clearProject();
};

#endif /* MAPPINGMANAGER_H_ */
