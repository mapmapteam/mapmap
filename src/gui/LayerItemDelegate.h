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

#ifndef LAYERITEMDELEGATE_H
#define LAYERITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QToolTip>
#include <QAbstractItemView>
#include <QHelpEvent>

#include "MM.h"

namespace mmp {

typedef int uid;

/// The buttons painted on a row of the list of layers.
enum class LayerRowButton { Hide, Solo, Lock, Duplicate, Delete };

/// Returns the column of the list of layers that holds a button.
int layerRowButtonColumn(LayerRowButton button);

/// Returns the rectangle of a button in the cell of its column, for painting it, for
/// knowing where it is clicked and for outlining it.
QRect layerRowButtonRect(LayerRowButton button, const QRect& cell);

/// Returns the button of a column under a position, if any, in button.
bool layerRowButtonAt(int column, const QRect& cell, const QPoint& position, LayerRowButton* button);

/// Returns the path of the setting of the layer that a button switches, such as "visible",
/// or an empty text for a button that switches none (Duplicate, Delete).
QString layerRowButtonSetting(LayerRowButton button);

class LayerItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  LayerItemDelegate(QObject *parent = nullptr);

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

private:
  /// While learning, outlines the button of a layer one can link, with the key linked to it.
  void _paintLearnOutline(QPainter* painter, LayerRowButton button, const QRect& cell, uid layerId) const;

public:
  /// Shows the name of the button under the mouse, and the key linked to it.
  bool helpEvent(QHelpEvent *event, QAbstractItemView *view,
                 const QStyleOptionViewItem &option, const QModelIndex &index) override;

protected:
  bool editorEvent(QEvent *event, QAbstractItemModel *model,
                   const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

}

#endif // LAYERITEMDELEGATE_H
