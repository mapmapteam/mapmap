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

MM_BEGIN_NAMESPACE

MappingListModel::MappingListModel(QObject *parent) :
  QAbstractTableModel(parent) {}

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
  
  switch (role) {
  case Qt::CheckStateRole:
    return mappingList.at(index.row()).isVisible ? Qt::Checked : Qt::Unchecked;
    break;
  case Qt::SizeHintRole:
    if (index.column() == MM::HideColumn)
      return QSize(20, 40);
    if (index.column() == MM::IconAndNameColum)
      return QSize(140, 40);
    break;
  case Qt::CheckStateRole + 1:
    return mappingList.at(index.row()).isSolo ? Qt::Checked : Qt::Unchecked;
    break;
  case Qt::CheckStateRole + 2:
    return mappingList.at(index.row()).isLocked ? Qt::Checked : Qt::Unchecked;
    break;
  case Qt::UserRole:
    return QVariant(mappingList.at(index.row()).id);
    break;
  case Qt::EditRole:
    return QVariant(mappingList.at(index.row()).name);
    break;
  case Qt::DisplayRole:
    return QVariant(mappingList.at(index.row()).name);
    break;
  case Qt::DecorationRole:
    return mappingList.at(index.row()).icon;
    break;
  default:
    return QVariant();
    break;
  }

  return QVariant();
}

Qt::ItemFlags MappingListModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  if (index.column() == MM::IconAndNameColum)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
        Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
      Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions MappingListModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

#define MIMETYPE QLatin1String("mapping/model.item.list")

QStringList MappingListModel::mimeTypes() const
{
  QStringList types;
  types << MIMETYPE;
  return types;
}

QMimeData *MappingListModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = QAbstractTableModel::mimeData(indexes);
  QByteArray encodeData;
  QDataStream stream(&encodeData, QIODevice::WriteOnly);

  foreach (QModelIndex index, indexes) {
    if (index.isValid()) {
      if (index.column() == MM::HideColumn) {
        int id = data(index, Qt::UserRole).toInt();
        stream << id;
        break; // Just take a first index and go
      }
    }
  }

  mimeData->setData(MIMETYPE, encodeData);

  return mimeData;
}

bool MappingListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if (!data->hasFormat(MIMETYPE)
      || column > 0)
    return false;

  if (action == Qt::IgnoreAction)
    return true;

  int endRow;

  if (!parent.isValid()) {
    if (row < 0)
      endRow = mappingList.size();
    else
      endRow = qMin(row, mappingList.size());
  } else {
    endRow = parent.row();
  }

  QByteArray encodeData = data->data(MIMETYPE);
  QDataStream stream(&encodeData, QIODevice::ReadOnly);

  while (!stream.atEnd()) {
    int id;
    stream >> id;

    int rows = getItemRowFromId(id);
    if (!beginMoveRows(QModelIndex(), rows, rows, QModelIndex(), endRow))
      continue;
    mappingList.move(rows, endRow);
    endMoveRows();

    ++endRow;
  }

  return true;
}

bool MappingListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid())
    return false;

  if (role == Qt::CheckStateRole && value.type() == QVariant::Bool) {
    if (mappingList[index.row()].isVisible != value.toBool()) {
      mappingList[index.row()].isVisible = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  if (role == Qt::CheckStateRole + 1 && value.type() == QVariant::Bool) {
    if (mappingList[index.row()].isSolo != value.toBool()) {
      mappingList[index.row()].isSolo = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  if (role == Qt::CheckStateRole + 2 && value.type() == QVariant::Bool) {
    if (mappingList[index.row()].isLocked != value.toBool()) {
      mappingList[index.row()].isLocked = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  if (role == Qt::EditRole && index.column() == MM::IconAndNameColum) {
    if (mappingList[index.row()].name != value.toString()) {
      mappingList[index.row()].name = value.toString();
      emit dataChanged(index, index);
      return true;
    }
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
  item.isLocked = false;
  item.isSolo = false;
  item.id = id;
  mappingList.insert(0, item);
}

void MappingListModel::updateModel()
{
  beginResetModel();
  endResetModel();
}

void MappingListModel::clear()
{
  for (QList<MappingItem>::iterator it = mappingList.end() - 1;
       it >= mappingList.begin(); --it) {
    mappingList.erase(it);
    updateModel();
  }
}

QModelIndex MappingListModel::getIndexFromRow(int row)
{
  return this->createIndex(row, 1);
}

int MappingListModel::getItemRowFromId(uid id) const
{
  for ( int row = 0; row < mappingList.size(); row++) {
    int itemId = mappingList.at(row).id;
    if (itemId == id)
      return row;
  }

  return -1;
}

QModelIndex MappingListModel::getIndexFromId(uid id) const
{
  return this->createIndex(getItemRowFromId(id), 0);
}

uid MappingListModel::getItemId(const QModelIndex &index) const
{
  return mappingList.at(index.row()).id;
}

void MappingListModel::setVisibility(const QModelIndex &index)
{
  if (index.isValid() && index.column() == MM::HideColumn) {
    setData(index, !(mappingList.at(index.row()).isVisible), Qt::CheckStateRole);
  }
}

MM_END_NAMESPACE
