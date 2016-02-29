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

#include "MappingListModel.h"

MappingListModel::MappingListModel(QObject *parent) :
  QAbstractTableModel(parent)
{

}

MappingListModel::~MappingListModel()
{

}

int MappingListModel::rowCount(const QModelIndex &parent) const
{
  return (parent.isValid() && parent.column() != 0) ? 0 : mappingList.size();
}

int MappingListModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return 3;
}

QVariant MappingListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.column() == MM::MuteColunm) {

    if (role == Qt::CheckStateRole)
      return mappingList.at(index.row()).isVisible ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::SizeHintRole)
      return QSize(30, 40);

  } else {

    if (role == Qt::EditRole || role == Qt::DisplayRole)
      return QVariant(mappingList.at(index.row()).name);

    if (role == Qt::UserRole)
      return QVariant(mappingList.at(index.row()).id);

    if (role == Qt::DecorationRole)
      return mappingList.at(index.row()).icon;

    if (role == Qt::SizeHintRole)
      return QSize(130, 40);

    if (role == Qt::TextAlignmentRole)
      return int(Qt::AlignVCenter);
  }

  return QVariant();
}

QVariant MappingListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
    return QString::number(section);

  return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags MappingListModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  if (index.column() == MM::MuteColunm)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
        Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
      Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool MappingListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid())
    return false;

  if (role == Qt::CheckStateRole && value.type() == QVariant::Bool) {
    mappingList[index.row()].isVisible = value.toBool();
    emit dataChanged(index, index);
    return true;
  }

  if (role == Qt::EditRole && index.column() == MM::IconAndNameColum) {
    mappingList[index.row()].name = value.toString();
    emit dataChanged(index, index);
    return true;
  }

  return false;
}

void MappingListModel::removeItem(int index)
{
  QList<MappingItem>::iterator it = mappingList.begin();
  mappingList.erase(it + index);
}

void MappingListModel::addItem(const QIcon &icon, const QString &name, int id)
{
  MappingItem item;

  item.icon = icon;
  item.name = name;
  item.isVisible = true;
  item.id = id;
  mappingList.insert(0, item);
}

void MappingListModel::updateModel()
{
  beginResetModel();
  endResetModel();
}

QModelIndex MappingListModel::selectedIndex(int row)
{
  return this->createIndex(row, 1);
}

void MappingListModel::setSelectedRow(int row)
{
  selectedRow = row;
}

int MappingListModel::getSelectedRow() const
{
  return selectedRow;
}

int MappingListModel::getItemRowFromId(int id) const
{
  for ( int row = 0; row < mappingList.size(); row++) {
    int itemId = mappingList.at(row).id;
    if (itemId == id)
      return row;
  }
}

QModelIndex MappingListModel::getIndexFromId(int id) const
{
  return this->createIndex(getItemRowFromId(id), 0);
}

int MappingListModel::getItemId(const QModelIndex &index) const
{
  return mappingList.at(index.row()).id;
}

void MappingListModel::setVisibility(const QModelIndex &index)
{
  if (index.isValid() && index.column() == MM::MuteColunm) {
    setData(index, !(mappingList.at(index.row()).isVisible), Qt::CheckStateRole);
  }
}
