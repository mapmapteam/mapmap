/*
 * MappingItemDelegate.h
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

#ifndef MAPPINGITEMDELEGATE_H
#define MAPPINGITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QToolTip>

#include "MM.h"

namespace mmp {

typedef int uid;

class MappingItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  MappingItemDelegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;

signals:
  void itemContextMenuRequested(const QPoint &pos);
  void itemDuplicated(uid itemId);
  void itemRemoved(uid itemId);

protected:
  bool editorEvent(QEvent *event, QAbstractItemModel *model,
                   const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

}

#endif // MAPPINGITEMDELEGATE_H
