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

MM_BEGIN_NAMESPACE

typedef int uid;

class MappingListModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  MappingListModel(QObject *parent = 0);
  ~MappingListModel() {}

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
  Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;

  QStringList mimeTypes() const Q_DECL_OVERRIDE;
  QMimeData *mimeData(const QModelIndexList &indexes) const Q_DECL_OVERRIDE;
  bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                        int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;

  bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;

  void removeItem(int index);
  void addItem(const QIcon &icon, const QString &name, int id);

  void updateModel();
  void clear();

  QModelIndex getIndexFromRow(int row);
  int getItemRowFromId(uid id) const;
  uid getItemId(const QModelIndex &index) const;
  QModelIndex getIndexFromId(uid id) const;

public slots:
  void setVisibility(const QModelIndex &index);

private:
  struct MappingItem {
    int id;
    QIcon icon;
    QString name;
    bool isLocked;
    bool isVisible;
    bool isSolo;
  };

  QList<MappingItem> mappingList;
};

MM_END_NAMESPACE

#endif // MAPPINGLISTMODEL_H
