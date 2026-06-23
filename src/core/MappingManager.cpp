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

QMap<uid, Layer::ptr> MappingManager::getSourceLayers(const Source::ptr source) const
{
  QMap<uid, Layer::ptr> sourceLayers;
  for (QVector<Layer::ptr>::const_iterator it = layerVector.begin(); it != layerVector.end(); ++it)
  {
    if ((*it)->getSource() == source)
    {
      sourceLayers[(*it)->getId()] = *it;
    }
  }
  return sourceLayers;
}


Source::ptr MappingManager::getSourceByName(QString name)
{
  return _getElementByName(sourceVector, name);
}

QString MappingManager::generateUniqueSourceName(const QString& baseName) const
{
  auto isTaken = [this](const QString& candidate) {
    for (const Source::ptr& source : sourceVector)
      if (source->getName() == candidate)
        return true;
    return false;
  };

  if (!isTaken(baseName))
    return baseName;

  for (int n = 2; ; ++n)
  {
    const QString candidate = QString("%1 %2").arg(baseName).arg(n);
    if (!isTaken(candidate))
      return candidate;
  }
}

QVector<Source::ptr> MappingManager::getSourcesByNameRegExp(QString namePattern)
{
  return _getElementsByNameRegExp(sourceVector, namePattern);
}

QVector<Source::ptr> MappingManager::getSourcesCompatibleWith(Layer::ptr layer)
{
	QVector<Source::ptr> paints;
	for (QVector<Source::ptr>::const_iterator it = sourceVector.constBegin();
	     it != sourceVector.constEnd(); ++it)
		if (layer->sourceIsCompatible(*it))
			paints.append(*it);
	return paints;
}

Layer::ptr MappingManager::getLayerByName(QString name)
{
  return _getElementByName(layerVector, name);
}

QVector<Layer::ptr> MappingManager::getLayersByNameRegExp(QString namePattern)
{
  return _getElementsByNameRegExp(layerVector, namePattern);
}

QMap<uid, Layer::ptr> MappingManager::getSourceLayersById(uid sourceId) const
{
  return getSourceLayers(sourceMap[sourceId]);
}

uid MappingManager::addSource(Source::ptr source)
{
  sourceVector.push_back(source);
  sourceMap[source->getId()] = source;
  return source->getId();
}

bool MappingManager::removeSource(uid sourceId)
{
  // Make sure the source to which this source refers to exists in the manager.
  Source::ptr source = getSourceById(sourceId);
  if (source)
  {
    // Remove all mappings associated with source.
    QMap<uid, Layer::ptr> sourceLayers = getSourceLayers(source);
    for (QMap<uid, Layer::ptr>::const_iterator it = sourceLayers.constBegin();
         it != sourceLayers.constEnd(); ++it)
    {
      removeLayer(it.key());
    }

    // Remove source. Its lifetime is managed by QSharedPointer: dropping the
    // manager's references here is enough. The source may still be held by the
    // undo stack (so it can be restored), and is freed once the last shared
    // pointer is released.
    //
    // NOTE: do NOT call source->~Source() explicitly. Because ~Source() is
    // virtual, that runs the full destructor chain on an object the shared
    // pointers still own, so the object is destroyed a second time when the
    // last reference is released — a double-free that crashes on quit/undo
    // (notably with Syphon sources, which carry extra state).
    int idx = sourceVector.lastIndexOf(source);
    Q_ASSERT(idx != -1);
    sourceVector.remove(idx);
    sourceMap.remove(sourceId);
    return true;
  }
  else
  {
    return false;
  }
}

bool MappingManager::replaceSourceLayers(Source::ptr oldSource,
        Source::ptr newSource)
{
  // Make sure the source to which this source refers to exists in the manager.
  if (oldSource && newSource)
  {
    QMap<uid, Layer::ptr> sourceLayers = getSourceLayers(oldSource);
    for (QMap<uid, Layer::ptr>::const_iterator it = sourceLayers.constBegin();
         it != sourceLayers.constEnd(); ++it)
    {
      Layer::ptr layer = it.value();
      layer->setSource(newSource);
    }
    return true;
  }
  else
  {
    return false;
  }
}

uid MappingManager::addLayer(Layer::ptr layer)
{
  // Make sure the source to which this mapping refers to exists in the manager.
  Q_ASSERT ( sourceVector.contains(layer->getSource()) );

  layerVector.insert(0, layer);
  layerMap[layer->getId()] = layer;

  return layer->getId();
}

bool MappingManager::removeLayer(uid mappingId)
{
  // Make sure the source to which this mapping refers to exists in the manager.
  Layer::ptr layer = getLayerById(mappingId);
  if (layer)
  {
    int idx = layerVector.lastIndexOf(layer);
    Q_ASSERT( idx != -1 ); // Q_ASSERT(layerVector.contains(layer));
    layerVector.remove(idx);
    layerMap.remove(mappingId);
    updateLayerDepths();

    return true;
  }
  else
  {
    return false;
  }
}

/// Moves a mapping of given uid by a certain number of steps up or down.
bool MappingManager::moveLayer(uid mappingId, int toIndex)
{
  // Make sure the source to which this mapping refers to exists in the manager.
  int idx = getLayerIndex(mappingId);
  if (idx >= 0)
  {
    layerVector.move(idx, toIndex);
    updateLayerDepths();
    return true;
  }
  else
  {
    return false;
  }
}

QVector<Layer::ptr> MappingManager::getVisibleLayers() const
{
  QVector<Layer::ptr> visible;

  // First pass: check if one of the mappings is in solo mode.
  bool hasSolo = false;
  for (QVector<Layer::ptr>::const_iterator it = layerVector.begin();
          it != layerVector.end(); ++it)
  {
    if ((*it)->isSolo())
    {
      hasSolo = true;
      break;
    }
  }

  // Second pass: fill the visible vector.
  for (QVector<Layer::ptr>::const_iterator it = layerVector.begin();
          it != layerVector.end(); ++it)
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
bool MappingManager::layerIsVisible(Layer::ptr layer) const
{
  // Solo mappings are always visible.
  if (layer->isSolo())
  {
    return true;
  }

  // Non-solo invisible mappings are always invisible.
  else if (! layer->isVisible())
  {
    return false;
  }

  // Mapping is non-solo yet visible: check if another mapping is solo
  // (which would thus make it invisible).
  else
  {
    for (QVector<Layer::ptr>::const_iterator it = layerVector.begin();
            it != layerVector.end(); ++it)
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
QVector<Source::ptr> MappingManager::getVisibleSources() const
{
  QVector<Source::ptr> visibleSources;
  QVector<Layer::ptr> visibleLayers = getVisibleLayers();
  for (QVector<Layer::ptr>::const_iterator it = visibleLayers.begin();
          it != visibleLayers.end(); ++it)
  {
    Source::ptr source((*it)->getSource());
    if (!visibleSources.contains(source))
      visibleSources.push_back(source);
  }
  return visibleSources;
}

void MappingManager::reorderLayers(QVector<uid> mappingIds)
{
  // Both vector needs to have the same size.
  Q_ASSERT( mappingIds.size() == layerVector.size() );
  layerVector.clear();
  int depth = 0;
  for (QVector<uid>::iterator it = mappingIds.begin();
          it != mappingIds.end(); ++it)
  {
    // Uid should be a key of the layerMap.
    Q_ASSERT( layerMap.contains(*it) );
    // Makes sure the uids are not repeated.
    Q_ASSERT( ! layerVector.contains(layerMap[*it]) );
    // Adds the mapping at the right place in the vector.
    Layer::ptr layer = layerMap[*it];
    layer->setDepth(depth);
    layerVector.push_back( layer );

    depth++;
  }
}

void MappingManager::updateLayerDepths()
{
  int depth = 0;
  for (QVector<Layer::ptr>::iterator it = layerVector.begin();
          it != layerVector.end(); ++it)
  {
    (*it)->setDepth(depth);
    depth++;
  }
}

//bool MappingManager::removeLayer(Layer::ptr layer)
//{
//}

void MappingManager::clearAll()
{
  sourceVector.clear();
  layerVector.clear();
  sourceMap.clear();
  layerMap.clear();
}

}
