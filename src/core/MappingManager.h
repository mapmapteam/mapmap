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

namespace mmp {

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

  /// Returns indices of paint or (-1) if not found.
  int getPaintIndex(Paint::ptr paint) const { return paintVector.lastIndexOf(paint); }
  int getPaintIndex(uint paintId) const { return getPaintIndex(paintMap[paintId]); }

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

  /// Returns mapping with given name (first match).
  Paint::ptr getPaintByName(QString name);

  /// Returns all mappings with given regexp.
  QVector<Paint::ptr> getPaintsByNameRegExp(QString namePattern);

	/// Get paints compatible with given mapping.
	QVector<Paint::ptr> getPaintsCompatibleWith(Mapping::ptr mapping);

  /// Adds a mapping and returns its uid.
  uid addMapping(Mapping::ptr mapping);

  /// Removes a mapping of given uid.
  bool removeMapping(uid mappingId);

  /// Moves a mapping of given uid by a certain number of steps up or down.
  bool moveMapping(uid mappingId, int toIndex);

  /// Returns the number of mappings.
  int nMappings() const { return mappingVector.size(); }

  /**
   * Returns the i-th mapping in the vector. Good for iterating over all mappings. Vector is
   * ordered from bottom (deepest) to top (shallowest) layer.
   */
  Mapping::ptr getMapping(int i) { return mappingVector[i]; }

  /// Returns mapping with given uid.
  Mapping::ptr getMappingById(uid id) const { return mappingMap[id]; }

  /// Returns mapping with given name (first match).
  Mapping::ptr getMappingByName(QString name);

  /// Returns all mappings with given regexp.
  QVector<Mapping::ptr> getMappingsByNameRegExp(QString namePattern);

  /// Returns indices of mapping or (-1) if not found.
  int getMappingIndex(Mapping::ptr mapping) const { return mappingVector.lastIndexOf(mapping); }
  int getMappingIndex(uint mappingId) const { return getMappingIndex(getMappingById(mappingId)); }

  int getMappingDepth(Mapping::ptr mapping) const { return -getMappingIndex(mapping); }

  /// Reorders the mappings according to given list of uids. QVector needs to
  void reorderMappings(QVector<uid> mappingIds);

  /// Update mapping depths after a move.
  void updateMappingsDepths();

  /// Returns the ordered list of visible mappings, using both the "visible" and "solo" properties.
  QVector<Mapping::ptr> getVisibleMappings() const;

  /// Returns true iff the mapping is visible.
  bool mappingIsVisible(Mapping::ptr mapping) const;

  /// Returns the list of visible paints (ie. paints for which at least one mapping is visible).
  QVector<Paint::ptr> getVisiblePaints() const;

  void clearAll();

private:
  template<class T>
  QSharedPointer<T> _getElementByName(const QVector<QSharedPointer<T> >& vector, QString name)
  {
    for (QSharedPointer<T> it: vector)
    {
      if (it->getName() == name)
      {
        return it;
      }
    }
    // Nothing found.
    return QSharedPointer<T>();
  }

  template<class T>
  QVector<QSharedPointer<T> > _getElementsByNameRegExp(const QVector<QSharedPointer<T> >& vector, QString namePattern)
  {
    QVector<QSharedPointer<T> > matchedElems;
    QRegExp regExp(namePattern, Qt::CaseSensitive, QRegExp::Wildcard);
    for (QSharedPointer<T> it: vector)
    {
      if (regExp.exactMatch(it->getName()))
      {
        matchedElems.push_back(it);
      }
    }
    return matchedElems;
  }
};

}

#endif /* MAPPINGMANAGER_H_ */
