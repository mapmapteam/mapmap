/*
 * MappingListModel.h
 *
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef MAPPINGLISTMODEL_H
#define MAPPINGLISTMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QIcon>
#include <QDebug>
#include <QMimeData>

#include "MM.h"
#include "Mapping.h"

namespace mmp {

typedef int uid;

class MappingListModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  MappingListModel(QObject *parent = nullptr);
  ~MappingListModel() override {}

  int rowCount(const QModelIndex & parent = QModelIndex()) const override;
  int columnCount(const QModelIndex & parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;

  QStringList mimeTypes() const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                        int row, int column, const QModelIndex &parent) override;

  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  void removeItem(int index);
  void addItem(Mapping::ptr mapping, const QIcon &icon, const QString &label);
  void moveItem(int row, int endRow);

  void updateModel();
  void clear();

  QModelIndex getIndexFromRow(int row);
  int getItemRowFromId(uid id) const;
  uid getItemId(const QModelIndex &index) const;
  QModelIndex getIndexFromId(uid id) const;

private:
  struct MappingItem {
    int id;
    QIcon icon;
    QString label;
    bool isLocked;
    bool isVisible;
    bool isSolo;
  };

  QList<MappingItem> mappingList;
};

}

#endif // MAPPINGLISTMODEL_H
