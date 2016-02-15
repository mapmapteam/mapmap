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

#include <QVector>
#include <QMap>

#include "Paint.h"
#include "Mapping.h"

/**
 * This is a container class for all the paints and mappings ie. the main model object that allows
 * CRUD over paints and mappings.
 */
class MappingManager
{
public:
  /// Constructor.
  MappingManager();

private:
  // Model elements.
  /// Container for all paints.
  QVector<Paint::ptr> paintVector;

  /// Maps from uids to paints.
  QMap<uid, Paint::ptr> paintMap;

  /// Container for all mappings (ordered from bottom layer to top layer).
  QVector<Mapping::ptr> mappingVector;

  /// Maps from uids to mappings.
  QMap<uid, Mapping::ptr> mappingMap;

public:
  /// Returns the list of mappings associated with given paint.
  QMap<uid, Mapping::ptr> getPaintMappings(const Paint::ptr paint) const;

  /// Returns the list of mappings associated with given paint uid.
  QMap<uid, Mapping::ptr> getPaintMappingsById(uid paintId) const;

  /// Adds a paint and returns its uid.
  uid addPaint(Paint::ptr paint);

  /// Returns the uid of a paint.
  uid getPaintId(Paint::ptr paint) const { return paintMap.key(paint); }

  /// Removes a paint of given uid.
  bool removePaint(uid paintId);

  /// DEPRECATED.
  bool replacePaintMappings(Paint::ptr oldpaint, Paint::ptr newpaint);

  /// Returns the number of paints.
  int nPaints() const { return paintVector.size(); }

  /// Returns the i-th paint in the vector. Good for iterating over all paints.
  Paint::ptr getPaint(int i) { return paintVector[i]; }

  /// Returns paint with given uid.
  Paint::ptr getPaintById(uid id) { return paintMap[id]; }

  /// Adds a mapping and returns its uid.
  uid addMapping(Mapping::ptr mapping);

  /// Removes a mapping of given uid.
  bool removeMapping(uid mappingId);

  /// Returns the number of mappings.
  int nMappings() const { return mappingVector.size(); }

  /**
   * Returns the i-th mapping in the vector. Good for iterating over all mappings. Vector is
   * ordered from bottom to top layer.
   */
  Mapping::ptr getMapping(int i) { return mappingVector[i]; }

  /// Returns mapping with given uid.
  Mapping::ptr getMappingById(uid id) { return mappingMap[id]; }

  /// Reorders the mappings according to given list of uids. QVector needs to
  void reorderMappings(QVector<uid> mappingIds);

  /// Returns the ordered list of visible mappings, using both the "visible" and "solo" properties.
  QVector<Mapping::ptr> getVisibleMappings() const;

  /// Returns true iff the mapping is visible.
  bool mappingIsVisible(Mapping::ptr mapping) const;

  void clearAll();
};

#endif /* MAPPINGMANAGER_H_ */
