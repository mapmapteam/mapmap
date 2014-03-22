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
 * This is a container class for all the paints and mappings. It is on the model
 * side.
 */
class MappingManager
{
public:
  MappingManager();

private:
  // Model.
  QVector<Paint::ptr> paintVector;
  QMap<uid, Paint::ptr> paintMap;
  QVector<Mapping::ptr> mappingVector;
  QMap<uid, Mapping::ptr> mappingMap;

public:
  /// Returns the list of mappings associated with given paint.
  QMap<uid, Mapping::ptr> getPaintMappings(const Paint::ptr paint) const;
  QMap<uid, Mapping::ptr> getPaintMappingsById(uid paintId) const;

  uid addPaint(Paint::ptr paint);
  bool removePaint(uid paintId);
  int nPaints() const { return paintVector.size(); }
  Paint::ptr getPaint(int i) { return paintVector[i]; }
  Paint::ptr getPaintById(uid id) { return paintMap[id]; }

  uid addImage(const QString imagePath, int frameWidth, int frameHeight);

  uid addMapping(Mapping::ptr mapping);
  bool removeMapping(uid mappingId);
//  bool removeMapping(Mapping::ptr mapping);
  int nMappings() const { return mappingVector.size(); }
  Mapping::ptr getMapping(int i) { return mappingVector[i]; }
  Mapping::ptr getMappingById(uid id) { return mappingMap[id]; }

  void reorderMappings(QVector<uid> mappingIds);

  QVector<Mapping::ptr> getVisibleMappings() const;

  void clearAll();
};

#endif /* MAPPINGMANAGER_H_ */
