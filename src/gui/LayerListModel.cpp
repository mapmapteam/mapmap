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

#include "LayerListModel.h"
#include <QIODevice>

namespace mmp {

LayerListModel::LayerListModel(QObject *parent) :
  QAbstractTableModel(parent) {}

int LayerListModel::rowCount(const QModelIndex &parent) const
{
  return (parent.isValid() && parent.column() != 0) ? 0 : layerList.size();
}

int LayerListModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)
  return 3;
}

QVariant LayerListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  switch (role) {
  case Qt::CheckStateRole:
    return layerList.at(index.row()).isVisible ? Qt::Checked : Qt::Unchecked;

  case Qt::SizeHintRole:
    if (index.column() == MM::HideColumn)
      return QSize(MM::MAPPING_LIST_HIDE_COLUMN, 40);
    if (index.column() == MM::IconAndNameColum)
      return QSize(MM::MAPPING_LIST_NAME_COLUMN, 40);
    if (index.column() == MM::GroupButtonColum)
      return QSize(MM::MAPPING_LIST_BUTTONS_COLUMN, 40);
   break;
  case Qt::CheckStateRole + 1:
    return layerList.at(index.row()).isSolo ? Qt::Checked : Qt::Unchecked;

  case Qt::CheckStateRole + 2:
    return layerList.at(index.row()).isLocked ? Qt::Checked : Qt::Unchecked;

  case Qt::UserRole:
    return QVariant(layerList.at(index.row()).id);

  case Qt::EditRole:
    return QVariant(layerList.at(index.row()).label);

  case Qt::DisplayRole:
    return QVariant(layerList.at(index.row()).label);

  case Qt::DecorationRole:
    return layerList.at(index.row()).icon;

  case Qt::ToolTipRole:
    return QString("ID: %1").arg(layerList.at(index.row()).id);

  default:
    return QVariant();
  }

  return QVariant();
}

Qt::ItemFlags LayerListModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::NoItemFlags;

  if (index.column() == MM::IconAndNameColum)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
        Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
      Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions LayerListModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

#define MIMETYPE QLatin1String("mapping/model.item.list")

QStringList LayerListModel::mimeTypes() const
{
  QStringList types;
  types << MIMETYPE;
  return types;
}

QMimeData *LayerListModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = QAbstractTableModel::mimeData(indexes);
  QByteArray encodeData;
  QDataStream stream(&encodeData, QIODevice::WriteOnly);

  for (QModelIndex index: indexes) {
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

bool LayerListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if (!data->hasFormat(MIMETYPE)
      || column > 0)
    return false;

  if (action == Qt::IgnoreAction)
    return true;

  int endRow;

  if (!parent.isValid()) {
    if (row < 0)
      endRow = layerList.size();
    else
      endRow = qMin(row, layerList.size());
  } else {
    endRow = parent.row();
  }

  QByteArray encodeData = data->data(MIMETYPE);
  QDataStream stream(&encodeData, QIODevice::ReadOnly);

  while (!stream.atEnd()) {
    int id;
    stream >> id;

    int row = getItemRowFromId(id);
    moveItem(row, endRow);

    ++endRow;
  }

  return true;
}

bool LayerListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid())
    return false;

  if (role == Qt::CheckStateRole && value.typeId() == QMetaType::Bool) {
    if (layerList[index.row()].isVisible != value.toBool()) {
      layerList[index.row()].isVisible = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  if (role == Qt::CheckStateRole + 1 && value.typeId() == QMetaType::Bool) {
    if (layerList[index.row()].isSolo != value.toBool()) {
      layerList[index.row()].isSolo = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  if (role == Qt::CheckStateRole + 2 && value.typeId() == QMetaType::Bool) {
    if (layerList[index.row()].isLocked != value.toBool()) {
      layerList[index.row()].isLocked = value.toBool();
      emit dataChanged(index, index);
      return true;
    }
  }

  // Note: Removed the check for column as this was blocking OSC from changing name.
  if (role == Qt::EditRole /* && index.column() == MM::IconAndNameColum*/) {
    if (layerList[index.row()].label != value.toString()) {
      layerList[index.row()].label = value.toString();
      emit dataChanged(index, index);
      return true;
    }
  }

  return false;
}

void LayerListModel::removeItem(int index)
{
  auto it = layerList.begin();
  layerList.erase(it + index);
}

void LayerListModel::moveItem(int row, int endRow)
{
  if (beginMoveRows(QModelIndex(), row, row, QModelIndex(), (row < endRow ? endRow+1 : endRow)))
  {
    layerList.move(row, endRow);
    endMoveRows();
  }
}

void LayerListModel::addItem(Layer::ptr mapping, const QIcon &icon, const QString &label)
{
  LayerItem item;

  item.id = mapping->getId();
  item.icon = icon;
  item.label = label;
  item.isVisible = mapping->isVisible();
  item.isLocked = mapping->isLocked();
  item.isSolo = mapping->isSolo();
  layerList.insert(0, item);
}

void LayerListModel::updateModel()
{
  beginResetModel();
  endResetModel();
}

void LayerListModel::clear()
{
  beginResetModel();
  layerList.clear();
  endResetModel();
}

QModelIndex LayerListModel::getIndexFromRow(int row)
{
  return this->createIndex(row, 1);
}

int LayerListModel::getItemRowFromId(uid id) const
{
  for ( int row = 0; row < layerList.size(); row++) {
    int itemId = layerList.at(row).id;
    if (itemId == id)
      return row;
  }

  return -1;
}

QModelIndex LayerListModel::getIndexFromId(uid id) const
{
  return this->createIndex(getItemRowFromId(id), 0);
}

uid LayerListModel::getItemId(const QModelIndex &index) const
{
  return layerList.at(index.row()).id;
}

}
