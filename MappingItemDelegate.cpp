/*
 * MppingItemDelegate.h
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

#include "MappingItemDelegate.h"

MappingItemDelegate::MappingItemDelegate(QObject *parent) :
  QStyledItemDelegate(parent)
{

}

void MappingItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (index.isValid()) {
    QRect rect = option.rect;
    int x = rect.x();
    int y = rect.y();

    if (option.state & QStyle::State_Selected)
      painter->fillRect(rect, MM::DARK_GRAY);

    if (index.column() == MM::HideColumn) {
      bool isVisible = index.model()->data(index, Qt::CheckStateRole).toBool();
      if (isVisible) {
        QStyleOptionToolButton mappingMuteButton;
        mappingMuteButton.state |= QStyle::State_Enabled;
        mappingMuteButton.rect = QRect(x + 4, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
        mappingMuteButton.icon = QIcon(":/visible-mapping");
        mappingMuteButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

        QApplication::style()->drawControl(
              QStyle::CE_ToolButtonLabel, &mappingMuteButton, painter);
      }
    }

    if(index.column() == MM::IconAndNameColum) {
      // Draw Icon
      QIcon mappingIcon = qvariant_cast<QIcon>(index.model()->data(index, Qt::DecorationRole));
      QPixmap iconPixmap = mappingIcon.pixmap(24, 24);
      QRect iconRect(x + 5, y, 24, rect.height());
      QApplication::style()->drawItemPixmap(painter, iconRect,
                                            Qt::AlignLeft | Qt::AlignVCenter, iconPixmap);

      // Draw Text
      QString mappingName = index.model()->data(index, Qt::DisplayRole).toString();
      QRect textRect(x + 40, y, rect.width() - 40, rect.height());
      QPalette textColor = QPalette(MM::WHITE);

      QApplication::style()->drawItemText(painter, textRect, Qt::AlignLeft | Qt::AlignVCenter,
                                          textColor, true, mappingName, QPalette::Window);
    }

    if (index.column() == MM::GroupButtonColum) {
      // Draw Buttons
      QStyleOptionToolButton mappingSoloButton;
      mappingSoloButton.state |= QStyle::State_Enabled;
      mappingSoloButton.rect = QRect(x + 10, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
      mappingSoloButton.icon = QIcon(":/solo-mapping");
      mappingSoloButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

      QStyleOptionToolButton mappingLockButton;
      mappingLockButton.state |= QStyle::State_Enabled;
      mappingLockButton.rect = QRect(x + 40, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
      mappingLockButton.icon = QIcon(":/lock-mapping");
      mappingLockButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

      QStyleOptionToolButton mappingDuplicateButton;
      mappingDuplicateButton.state |= QStyle::State_Enabled;
      mappingDuplicateButton.rect = QRect(x + 70, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
      mappingDuplicateButton.icon = QIcon(":/duplicate-mapping");
      mappingDuplicateButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

      QStyleOptionToolButton mappingDeleteButton;
      mappingDeleteButton.state |= QStyle::State_Enabled;
      mappingDeleteButton.rect = QRect(x + 100, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
      mappingDeleteButton.icon = QIcon(":/delete-mapping");
      mappingDeleteButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingSoloButton, painter);
      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingLockButton, painter);
      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingDuplicateButton, painter);
      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingDeleteButton, painter);

    }
  } else {
    QStyledItemDelegate::paint(painter, option, index);
  }
}

QWidget *MappingItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (index.column() == MM::IconAndNameColum) {
    QLineEdit *editor = new QLineEdit(parent);
    editor->setFixedHeight(option.rect.height());
    editor->setContentsMargins(option.rect.x() + 4, 0, 0, 0);
    return editor;
  } else
    return 0;
}

void MappingItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QString value = index.model()->data(index, Qt::EditRole).toString();

  QLineEdit *nameEdit = static_cast<QLineEdit*>(editor); // TODO: use reinterpret_cast instead static_cast
  nameEdit->setText(value);
}

void MappingItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  QLineEdit *nameEdit = static_cast<QLineEdit*>(editor); // TODO: use reinterpret_cast instead static_cast
  model->setData(index, nameEdit->text(), Qt::EditRole);
}

void MappingItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  Q_UNUSED(index);
  editor->setGeometry(option.rect);
}

bool MappingItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
  QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
  QRect rect = option.rect;
  int x = rect.x();
  int y = rect.y();

  QRect soloButtonRect = QRect(x + 10, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
  QRect lockButtonRect = QRect(x + 40, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
  QRect duplicateButtonRect = QRect(x + 70, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
  QRect deleteButtonRect = QRect(x + 100, y + 12, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);

  if (event->type() == QEvent::MouseMove) {
    // TODO: handle tooltip
  }

  if (event->type() == QEvent::MouseButtonPress) {

    if (index.column() == MM::GroupButtonColum) {

      if (soloButtonRect.contains(mouseEvent->pos()))
        model->setData(index, !(index.data(Qt::CheckStateRole + 1).toBool()), Qt::CheckStateRole + 1);

      if (lockButtonRect.contains(mouseEvent->pos()))
        model->setData(index, !(index.data(Qt::CheckStateRole + 2).toBool()), Qt::CheckStateRole + 2);

      if (duplicateButtonRect.contains(mouseEvent->pos()))
        emit itemDuplicated(index.data(Qt::UserRole).toInt());

      if (deleteButtonRect.contains(mouseEvent->pos()))
        emit itemRemoved(index.data(Qt::UserRole).toInt());
    }
  }
  return false;
}
