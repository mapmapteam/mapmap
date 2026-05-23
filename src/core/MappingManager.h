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
#include <QRegularExpression>

#include "Source.h"
#include "Layer.h"

namespace mmp {

/**
 * This is a container class for all the sources and mappings ie. the main model object that allows
 * CRUD over sources and mappings.
 */
class MappingManager
{
public:
  /// Constructor.
  MappingManager();

private:
  // Model elements.
  /// Container for all sources.
  QVector<Source::ptr> sourceVector;

  /// Maps from uids to sources.
  QMap<uid, Source::ptr> sourceMap;

  /// Container for all mappings (ordered from bottom layer to top layer).
  QVector<Layer::ptr> layerVector;

  /// Maps from uids to mappings.
  QMap<uid, Layer::ptr> layerMap;

public:
  /// Returns the list of mappings associated with given source.
  QMap<uid, Layer::ptr> getSourceLayers(const Source::ptr source) const;

  /// Returns the list of mappings associated with given source uid.
  QMap<uid, Layer::ptr> getSourceLayersById(uid sourceId) const;

  /// Adds a source and returns its uid.
  uid addSource(Source::ptr source);

  /// Returns the uid of a source.
  uid getSourceId(Source::ptr source) const { return sourceMap.key(source); }

  /// Returns indices of source or (-1) if not found.
  int getSourceIndex(Source::ptr source) const { return sourceVector.lastIndexOf(source); }
  int getSourceIndex(uint sourceId) const { return getSourceIndex(sourceMap[sourceId]); }

  /// Removes a source of given uid.
  bool removeSource(uid sourceId);

  /// DEPRECATED.
  bool replaceSourceLayers(Source::ptr oldSource, Source::ptr newSource);

  /// Returns the number of sources.
  int nSources() const { return sourceVector.size(); }

  /// Returns the i-th source in the vector. Good for iterating over all sources.
  Source::ptr getSource(int i) { return sourceVector[i]; }

  /// Returns source with given uid.
  Source::ptr getSourceById(uid id) { return sourceMap[id]; }

  /// Returns mapping with given name (first match).
  Source::ptr getSourceByName(QString name);

  /// Returns all mappings with given regexp.
  QVector<Source::ptr> getSourcesByNameRegExp(QString namePattern);

	/// Get paints compatible with given mapping.
	QVector<Source::ptr> getSourcesCompatibleWith(Layer::ptr mapping);

  /// Adds a mapping and returns its uid.
  uid addLayer(Layer::ptr mapping);

  /// Removes a mapping of given uid.
  bool removeLayer(uid mappingId);

  /// Moves a mapping of given uid by a certain number of steps up or down.
  bool moveLayer(uid mappingId, int toIndex);

  /// Returns the number of mappings.
  int nLayers() const { return layerVector.size(); }

  /**
   * Returns the i-th mapping in the vector. Good for iterating over all mappings. Vector is
   * ordered from bottom (deepest) to top (shallowest) layer.
   */
  Layer::ptr getLayer(int i) { return layerVector[i]; }

  /// Returns mapping with given uid.
  Layer::ptr getLayerById(uid id) const { return layerMap[id]; }

  /// Returns mapping with given name (first match).
  Layer::ptr getLayerByName(QString name);

  /// Returns all mappings with given regexp.
  QVector<Layer::ptr> getLayersByNameRegExp(QString namePattern);

  /// Returns indices of mapping or (-1) if not found.
  int getLayerIndex(Layer::ptr mapping) const { return layerVector.lastIndexOf(mapping); }
  int getLayerIndex(uint mappingId) const { return getLayerIndex(getLayerById(mappingId)); }

  int getLayerDepth(Layer::ptr mapping) const { return -getLayerIndex(mapping); }

  /// Reorders the mappings according to given list of uids. QVector needs to
  void reorderLayers(QVector<uid> mappingIds);

  /// Update mapping depths after a move.
  void updateLayerDepths();

  /// Returns the ordered list of visible mappings, using both the "visible" and "solo" properties.
  QVector<Layer::ptr> getVisibleLayers() const;

  /// Returns true iff the mapping is visible.
  bool layerIsVisible(Layer::ptr mapping) const;

  /// Returns the list of visible sources (ie. paints for which at least one mapping is visible).
  QVector<Source::ptr> getVisibleSources() const;

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
    QRegularExpression regExp(QRegularExpression::wildcardToRegularExpression(namePattern));
    for (QSharedPointer<T> it: vector)
    {
      if (regExp.match(it->getName()).hasMatch())
      {
        matchedElems.push_back(it);
      }
    }
    return matchedElems;
  }
};

}

#endif /* MAPPINGMANAGER_H_ */
