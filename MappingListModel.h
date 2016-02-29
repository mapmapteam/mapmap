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
#include <QItemSelectionModel>

#include "MM.h"

class MappingListModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  MappingListModel(QObject *parent = 0);
  ~MappingListModel();

  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  int columnCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;

  bool setData(const QModelIndex &index, const QVariant &value, int role);

  void removeItem(int index);
  void addItem(const QIcon &icon, const QString &name, int id);

  void updateModel();
  QModelIndex selectedIndex(int row);

  void setSelectedRow(int row);
  int getSelectedRow() const;
  int getItemRowFromId(int id) const;
  int getItemId(const QModelIndex &index) const;
  QModelIndex getIndexFromId(int id) const;

public slots:
  void setVisibility(const QModelIndex &index);

private:
  struct MappingItem {
    int id;
    QIcon icon;
    QString name;
    bool isVisible;
  };

  QList<MappingItem> mappingList;

  int selectedRow;
};

#endif // MAPPINGLISTMODEL_H
