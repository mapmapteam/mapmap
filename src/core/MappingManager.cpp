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

namespace mmp {

MappingManager::MappingManager()
{
  // TODO Auto-generated constructor stub

}

QMap<uid, Mapping::ptr> MappingManager::getPaintMappings(const Paint::ptr paint) const
{
  QMap<uid, Mapping::ptr> paintMappings;
  for (QVector<Mapping::ptr>::const_iterator it = mappingVector.begin(); it != mappingVector.end(); ++it)
  {
    if ((*it)->getPaint() == paint)
    {
      paintMappings[(*it)->getId()] = *it;
    }
  }
  return paintMappings;
}


Paint::ptr MappingManager::getPaintByName(QString name)
{
  return _getElementByName(paintVector, name);
}

QVector<Paint::ptr> MappingManager::getPaintsByNameRegExp(QString namePattern)
{
  return _getElementsByNameRegExp(paintVector, namePattern);
}

QVector<Paint::ptr> MappingManager::getPaintsCompatibleWith(Mapping::ptr mapping)
{
	QVector<Paint::ptr> paints;
	for (QVector<Paint::ptr>::const_iterator it = paintVector.constBegin();
	     it != paintVector.constEnd(); ++it)
		if (mapping->paintIsCompatible(*it))
			paints.append(*it);
	return paints;
}

Mapping::ptr MappingManager::getMappingByName(QString name)
{
  return _getElementByName(mappingVector, name);
}

QVector<Mapping::ptr> MappingManager::getMappingsByNameRegExp(QString namePattern)
{
  return _getElementsByNameRegExp(mappingVector, namePattern);
}

QMap<uid, Mapping::ptr> MappingManager::getPaintMappingsById(uid paintId) const
{
  return getPaintMappings(paintMap[paintId]);
}

uid MappingManager::addPaint(Paint::ptr paint)
{
  paintVector.push_back(paint);
  paintMap[paint->getId()] = paint;
  return paint->getId();
}

bool MappingManager::removePaint(uid paintId)
{
  // Make sure the paint to which this paint refers to exists in the manager.
  Paint::ptr paint = getPaintById(paintId);
  if (paint)
  {
    // Remove all mappings associated with paint.
    QMap<uid, Mapping::ptr> paintMappings = getPaintMappings(paint);
    for (QMap<uid, Mapping::ptr>::const_iterator it = paintMappings.constBegin();
         it != paintMappings.constEnd(); ++it)
    {
      removeMapping(it.key());
    }

    // Remove paint.
    int idx = paintVector.lastIndexOf(paint);
    Q_ASSERT(idx != -1);
    paintVector.remove(idx);
    paintMap.remove(paintId);
    paint->~Paint(); // FIX ME: Explicit call of paint destructor in order add Camera more than once
    return true;
  }
  else
  {
    return false;
  }
}

bool MappingManager::replacePaintMappings(Paint::ptr oldpaint,
        Paint::ptr newpaint)
{
  // Make sure the paint to which this paint refers to exists in the manager.
  if (oldpaint && newpaint)
  {
    QMap<uid, Mapping::ptr> paintMappings = getPaintMappings(oldpaint);
    for (QMap<uid, Mapping::ptr>::const_iterator it = paintMappings.constBegin();
         it != paintMappings.constEnd(); ++it)
    {
      Mapping::ptr mapping = it.value();
      mapping->setPaint(newpaint);
    }
    return true;
  }
  else
  {
    return false;
  }
}

uid MappingManager::addMapping(Mapping::ptr mapping)
{
  // Make sure the paint to which this mapping refers to exists in the manager.
  Q_ASSERT ( paintVector.contains(mapping->getPaint()) );

  mappingVector.insert(0, mapping);
  mappingMap[mapping->getId()] = mapping;

  return mapping->getId();
}

bool MappingManager::removeMapping(uid mappingId)
{
  // Make sure the paint to which this mapping refers to exists in the manager.
  Mapping::ptr mapping = getMappingById(mappingId);
  if (mapping)
  {
    int idx = mappingVector.lastIndexOf(mapping);
    Q_ASSERT( idx != -1 ); // Q_ASSERT(mappingVector.contains(mapping));
    mappingVector.remove(idx);
    mappingMap.remove(mappingId);
    updateMappingsDepths();

    return true;
  }
  else
  {
    return false;
  }
}

/// Moves a mapping of given uid by a certain number of steps up or down.
bool MappingManager::moveMapping(uid mappingId, int toIndex)
{
  // Make sure the paint to which this mapping refers to exists in the manager.
  int idx = getMappingIndex(mappingId);
  if (idx >= 0)
  {
    mappingVector.move(idx, toIndex);
    updateMappingsDepths();
    return true;
  }
  else
  {
    return false;
  }
}

QVector<Mapping::ptr> MappingManager::getVisibleMappings() const
{
  QVector<Mapping::ptr> visible;

  // First pass: check if one of the mappings is in solo mode.
  bool hasSolo = false;
  for (QVector<Mapping::ptr>::const_iterator it = mappingVector.begin();
          it != mappingVector.end(); ++it)
  {
    if ((*it)->isSolo())
    {
      hasSolo = true;
      break;
    }
  }

  // Second pass: fill the visible vector.
  for (QVector<Mapping::ptr>::const_iterator it = mappingVector.begin();
          it != mappingVector.end(); ++it)
  {
    // Solo has priority over invisible (mute)
    if ( (hasSolo && (*it)->isSolo()) ||
         (! hasSolo && (*it)->isVisible()) )
    {
      visible.push_back(*it);
    }
  }

  return visible;
}

/// Returns true iff the mapping is visible.
bool MappingManager::mappingIsVisible(Mapping::ptr mapping) const
{
  // Solo mappings are always visible.
  if (mapping->isSolo())
  {
    return true;
  }

  // Non-solo invisible mappings are always invisible.
  else if (! mapping->isVisible())
  {
    return false;
  }

  // Mapping is non-solo yet visible: check if another mapping is solo
  // (which would thus make it invisible).
  else
  {
    for (QVector<Mapping::ptr>::const_iterator it = mappingVector.begin();
            it != mappingVector.end(); ++it)
    {
      if ((*it)->isSolo())
      {
        return false;
      }
    }

    // Mapping is non-solo yet visible and there are no solo mappings.
    return true;
  }
}

/// Returns the list of visible paints (ie. paints for which at least one mapping is visible).
QVector<Paint::ptr> MappingManager::getVisiblePaints() const
{
  QVector<Paint::ptr> visiblePaints;
  QVector<Mapping::ptr> visibleMappings = getVisibleMappings();
  for (QVector<Mapping::ptr>::const_iterator it = visibleMappings.begin();
          it != visibleMappings.end(); ++it)
  {
    Paint::ptr paint((*it)->getPaint());
    if (!visiblePaints.contains(paint))
      visiblePaints.push_back(paint);
  }
  return visiblePaints;
}

void MappingManager::reorderMappings(QVector<uid> mappingIds)
{
  // Both vector needs to have the same size.
  Q_ASSERT( mappingIds.size() == mappingVector.size() );
  mappingVector.clear();
  int depth = 0;
  for (QVector<uid>::iterator it = mappingIds.begin();
          it != mappingIds.end(); ++it)
  {
    // Uid should be a key of the mappingMap.
    Q_ASSERT( mappingMap.contains(*it) );
    // Makes sure the uids are not repeated.
    Q_ASSERT( ! mappingVector.contains(mappingMap[*it]) );
    // Adds the mapping at the right place in the vector.
    Mapping::ptr mapping = mappingMap[*it];
    mapping->setDepth(depth);
    mappingVector.push_back( mapping );

    depth++;
  }
}

void MappingManager::updateMappingsDepths()
{
  int depth = 0;
  for (QVector<Mapping::ptr>::iterator it = mappingVector.begin();
          it != mappingVector.end(); ++it)
  {
    (*it)->setDepth(depth);
    depth++;
  }
}

//bool MappingManager::removeMapping(Mapping::ptr mapping)
//{
//}

void MappingManager::clearAll()
{
  paintVector.clear();
  mappingVector.clear();
  paintMap.clear();
  mappingMap.clear();
}

}
