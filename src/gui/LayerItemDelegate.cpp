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

#include "LayerItemDelegate.h"
#include <QPainterPath>

#include "LearnMode.h"

namespace mmp {

namespace {

/// Distance from the left of its cell to the eye, in pixels.
const int HIDE_BUTTON_LEFT = 4;

/// Distance from the top of a row to its buttons, in pixels.
const int BUTTON_TOP = 12;

/// Distances from the right of its cell to the left of each button, in pixels.
const int SOLO_BUTTON_FROM_RIGHT = 118;
const int LOCK_BUTTON_FROM_RIGHT = 88;
const int DUPLICATE_BUTTON_FROM_RIGHT = 58;
const int DELETE_BUTTON_FROM_RIGHT = 28;

/// Colour of the outline of a button one can link, while learning.
const QColor LEARN_OUTLINE_COLOR(0x3d, 0xae, 0xe9);

/// Opacity of the filling of the button chosen while learning, between 0 and 255.
const int LEARN_CHOSEN_ALPHA = 70;

/// Width of the outline, in pixels.
const int LEARN_OUTLINE_WIDTH = 2;

/// Room around a button for its outline, and radius of its corners, in pixels.
const int LEARN_OUTLINE_MARGIN = 3;

/// Size of the text of the key written under a button while learning, in points.
const int LEARN_BADGE_POINT_SIZE = 8;

/// Height of the text of the key under a button, in pixels.
const int LEARN_BADGE_HEIGHT = 12;

/// The buttons one can link, in the order they are painted.
const LayerRowButton LINKABLE_BUTTONS[] = { LayerRowButton::Hide, LayerRowButton::Solo, LayerRowButton::Lock };

/// The buttons, in the order they are looked for under the mouse.
const LayerRowButton ALL_BUTTONS[] = { LayerRowButton::Hide, LayerRowButton::Solo, LayerRowButton::Lock,
                                       LayerRowButton::Duplicate, LayerRowButton::Delete };

} // namespace

int layerRowButtonColumn(LayerRowButton button)
{
  return button == LayerRowButton::Hide ? MM::HideColumn : MM::GroupButtonColum;
}

QRect layerRowButtonRect(LayerRowButton button, const QRect& cell)
{
  const int right = cell.x() + cell.width();
  int left = 0;
  switch (button) {
  case LayerRowButton::Hide:      left = cell.x() + HIDE_BUTTON_LEFT; break;
  case LayerRowButton::Solo:      left = right - SOLO_BUTTON_FROM_RIGHT; break;
  case LayerRowButton::Lock:      left = right - LOCK_BUTTON_FROM_RIGHT; break;
  case LayerRowButton::Duplicate: left = right - DUPLICATE_BUTTON_FROM_RIGHT; break;
  case LayerRowButton::Delete:    left = right - DELETE_BUTTON_FROM_RIGHT; break;
  }
  return QRect(left, cell.y() + BUTTON_TOP, MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
}

bool layerRowButtonAt(int column, const QRect& cell, const QPoint& position, LayerRowButton* button)
{
  for (LayerRowButton candidate : ALL_BUTTONS) {
    if (layerRowButtonColumn(candidate) == column && layerRowButtonRect(candidate, cell).contains(position)) {
      *button = candidate;
      return true;
    }
  }
  return false;
}

QString layerRowButtonSetting(LayerRowButton button)
{
  switch (button) {
  case LayerRowButton::Hide: return QStringLiteral("visible");
  case LayerRowButton::Solo: return QStringLiteral("solo");
  case LayerRowButton::Lock: return QStringLiteral("locked");
  default:                   return QString();
  }
}

void LayerItemDelegate::_paintLearnOutline(QPainter* painter, LayerRowButton button,
                                           const QRect& cell, uid layerId) const
{
  LearnMode* learn = LearnMode::current();
  if (!learn || !learn->isActive())
    return;
  const LinkTarget target = LinkTarget::of(LinkTarget::LayerElement, layerId, layerRowButtonSetting(button));
  const QRect frame = layerRowButtonRect(button, cell).adjusted(
        -LEARN_OUTLINE_MARGIN, -LEARN_OUTLINE_MARGIN, LEARN_OUTLINE_MARGIN, LEARN_OUTLINE_MARGIN);

  painter->save();
  if (learn->isChosen(target)) {
    QColor fill = LEARN_OUTLINE_COLOR;
    fill.setAlpha(LEARN_CHOSEN_ALPHA);
    painter->setBrush(fill);
  }
  else {
    painter->setBrush(Qt::NoBrush);
  }
  painter->setPen(QPen(LEARN_OUTLINE_COLOR, LEARN_OUTLINE_WIDTH));
  painter->drawRoundedRect(frame, LEARN_OUTLINE_MARGIN, LEARN_OUTLINE_MARGIN);

  const QString trigger = learn->triggerText(target);
  if (!trigger.isEmpty()) {
    QFont font = painter->font();
    font.setPointSize(LEARN_BADGE_POINT_SIZE);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(LEARN_OUTLINE_COLOR);
    const QRect text(frame.left() - LEARN_OUTLINE_MARGIN, frame.bottom(),
                     frame.width() + 2 * LEARN_OUTLINE_MARGIN, LEARN_BADGE_HEIGHT);
    painter->drawText(text, Qt::AlignHCenter | Qt::AlignTop, trigger);
  }
  painter->restore();
}

LayerItemDelegate::LayerItemDelegate(QObject *parent) :
  QStyledItemDelegate(parent)
{

}

void LayerItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  painter->setRenderHint(QPainter::Antialiasing);

  if (index.isValid())
  {
    QRect rect = option.rect;
    int x = rect.x();
    int y = rect.y();

    if (option.state & QStyle::State_Selected)
    {
      painter->fillRect(rect, MM::DARK_GRAY);
    }

    if (index.column() == MM::HideColumn)
    {
      bool isVisible = index.model()->data(index, Qt::CheckStateRole).toBool();
      if (isVisible)
      {
        QStyleOptionToolButton mappingMuteButton;
        mappingMuteButton.state |= QStyle::State_Enabled;
        mappingMuteButton.rect = layerRowButtonRect(LayerRowButton::Hide, rect);
        mappingMuteButton.icon = QIcon(":/visible-mapping");
        mappingMuteButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE,
                MM::MAPPING_LIST_ICON_SIZE);

        QApplication::style()->drawControl(
              QStyle::CE_ToolButtonLabel, &mappingMuteButton, painter);
      }
    }

    if (index.column() == MM::IconAndNameColum)
    {
      // Draw Icon
      QIcon mappingIcon = qvariant_cast<QIcon>(index.model()->data(index,
                  Qt::DecorationRole));
      QPixmap iconPixmap = mappingIcon.pixmap(24, 24);
      QRect iconRect(x + 5, y, 24, rect.height());
      QApplication::style()->drawItemPixmap(painter, iconRect,
              Qt::AlignLeft | Qt::AlignVCenter, iconPixmap);

      // Draw Text
      QString mappingName = index.model()->data(index, Qt::EditRole).toString();
      QRect textRect(x + 40, y, rect.width() - 40, rect.height());
      QPalette textColor = QPalette(MM::WHITE);

      QApplication::style()->drawItemText(painter, textRect,
              Qt::AlignLeft | Qt::AlignVCenter,
              textColor, true, mappingName, QPalette::Window);
    }

    if (index.column() == MM::GroupButtonColum)
    {
      bool isSolo = index.model()->data(index, Qt::CheckStateRole + 1).toBool();
      bool isLocked = index.model()->data(index, Qt::CheckStateRole + 2).toBool();
      // Create all mappings buttons:
      // solo button
      QStyleOptionToolButton mappingSoloButton;
      mappingSoloButton.state |= QStyle::State_Enabled;
      mappingSoloButton.rect = layerRowButtonRect(LayerRowButton::Solo, rect);
      mappingSoloButton.icon = QIcon(":/solo-mapping");
      mappingSoloButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE,
              MM::MAPPING_LIST_ICON_SIZE);
      mappingSoloButton.text = tr("Solo layer");

      // lock button
      QStyleOptionToolButton mappingLockButton;
      mappingLockButton.state |= QStyle::State_Enabled;
      mappingLockButton.rect = layerRowButtonRect(LayerRowButton::Lock, rect);
      mappingLockButton.icon = QIcon(":/lock-mapping");
      mappingLockButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE,
              MM::MAPPING_LIST_ICON_SIZE);
      mappingLockButton.text = tr("Lock layer");

      // duplicate button
      QStyleOptionToolButton mappingDuplicateButton;
      mappingDuplicateButton.state |= QStyle::State_Enabled;
      mappingDuplicateButton.rect = layerRowButtonRect(LayerRowButton::Duplicate, rect);
      mappingDuplicateButton.icon = QIcon(":/duplicate-mapping");
      mappingDuplicateButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE,
              MM::MAPPING_LIST_ICON_SIZE);
      mappingDuplicateButton.text = tr("Duplicate layer");

      // delete button
      QStyleOptionToolButton mappingDeleteButton;
      mappingDeleteButton.state |= QStyle::State_Enabled;
      mappingDeleteButton.rect = layerRowButtonRect(LayerRowButton::Delete, rect);
      mappingDeleteButton.icon = QIcon(":/delete-mapping");
      mappingDeleteButton.iconSize = QSize(MM::MAPPING_LIST_ICON_SIZE,
              MM::MAPPING_LIST_ICON_SIZE);
      mappingDeleteButton.text = tr("Delete layer");

      if (isSolo)
      {
        QPainterPath mappingSoloPanel;
        mappingSoloPanel.addRoundedRect(mappingSoloButton.rect.adjusted(
              -2, -2, 2, 2), 4, 4);
        painter->setPen(QPen()); // No pen
        painter->fillPath(mappingSoloPanel, MM::DARK_BLUE);
        painter->drawPath(mappingSoloPanel);
      }
      QApplication::style()->drawControl( QStyle::CE_ToolButtonLabel,
              &mappingSoloButton, painter);

      if (isLocked)
      {
        QPainterPath mappingLockPanel;
        mappingLockPanel.addRoundedRect(QRectF(mappingLockButton.rect.adjusted(
              -2, -2, 2, 2)), 4, 4);
        painter->setPen(QPen()); // No pen
        painter->fillPath(mappingLockPanel, MM::DARK_BLUE);
        painter->drawPath(mappingLockPanel);
      }
      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingLockButton, painter);

      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingDuplicateButton, painter);
      QApplication::style()->drawControl(
            QStyle::CE_ToolButtonLabel, &mappingDeleteButton, painter);
    }
  }
  else
  {
    QStyledItemDelegate::paint(painter, option, index);
  }

  // While learning, outline the buttons one can link.
  if (index.isValid())
  {
    const uid layerId = index.model()->data(index, Qt::UserRole).toInt();
    for (LayerRowButton button : LINKABLE_BUTTONS)
    {
      if (layerRowButtonColumn(button) == index.column())
        _paintLearnOutline(painter, button, option.rect, layerId);
    }
  }
}

QWidget *LayerItemDelegate::createEditor(QWidget *parent,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (index.column() == MM::IconAndNameColum)
  {
    QLineEdit *editor = new QLineEdit(parent);
    editor->setFixedHeight(option.rect.height());
    editor->setContentsMargins(option.rect.x() + 4, 0, 0, 0);
    return editor;
  }
  else
  {
    return nullptr;
  }
}

void LayerItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QString value = index.model()->data(index, Qt::EditRole).toString();

  QLineEdit *nameEdit = static_cast<QLineEdit*>(editor); // TODO: use reinterpret_cast instead static_cast
  nameEdit->setText(value);
}

void LayerItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  QLineEdit *nameEdit = static_cast<QLineEdit*>(editor); // TODO: use reinterpret_cast instead static_cast
  model->setData(index, nameEdit->text(), Qt::EditRole);
}

void LayerItemDelegate::updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  Q_UNUSED(index)
  editor->setGeometry(option.rect);
}

bool LayerItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
        const QStyleOptionViewItem &option, const QModelIndex &index)
{
  LayerRowButton button;
  if (event->type() != QEvent::ToolTip || !index.isValid()
      || !layerRowButtonAt(index.column(), option.rect, event->pos(), &button))
    return QStyledItemDelegate::helpEvent(event, view, option, index);

  QStringList lines;
  switch (button)
  {
  case LayerRowButton::Hide:      lines << tr("Show or hide the layer"); break;
  case LayerRowButton::Solo:      lines << tr("Solo layer"); break;
  case LayerRowButton::Lock:      lines << tr("Lock layer"); break;
  case LayerRowButton::Duplicate: lines << tr("Duplicate layer"); break;
  case LayerRowButton::Delete:    lines << tr("Delete layer"); break;
  }
  const QString setting = layerRowButtonSetting(button);
  if (LearnMode* learn = LearnMode::current(); learn && !setting.isEmpty())
    lines << learn->triggerLines(LinkTarget::of(LinkTarget::LayerElement, index.data(Qt::UserRole).toInt(), setting));
  QToolTip::showText(event->globalPos(), lines.join(QChar('\n')), view);
  return true;
}

bool LayerItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
        const QStyleOptionViewItem &option, const QModelIndex &index)
{
  if (event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    LayerRowButton button;
    LearnMode* learn = LearnMode::current();
    if (mouseEvent->buttons() & Qt::LeftButton && index.isValid()
        && layerRowButtonAt(index.column(), option.rect, mouseEvent->pos(), &button)
        && learn && learn->isActive())
    {
      // While learning, a click chooses the setting of the button instead of switching it.
      const QString setting = layerRowButtonSetting(button);
      if (!setting.isEmpty())
        learn->choose(LinkTarget::of(LinkTarget::LayerElement, index.data(Qt::UserRole).toInt(), setting));
      return true;
    }
    else if (mouseEvent->buttons() & Qt::LeftButton && index.isValid()
        && layerRowButtonAt(index.column(), option.rect, mouseEvent->pos(), &button))
    {
      switch (button)
      {
      case LayerRowButton::Hide:
        model->setData(index, !(index.data(Qt::CheckStateRole).toBool()), Qt::CheckStateRole);
        break;
      case LayerRowButton::Solo:
        model->setData(index, !(index.data(Qt::CheckStateRole + 1).toBool()), Qt::CheckStateRole + 1);
        break;
      case LayerRowButton::Lock:
        model->setData(index, !(index.data(Qt::CheckStateRole + 2).toBool()), Qt::CheckStateRole + 2);
        break;
      case LayerRowButton::Duplicate:
        emit itemDuplicated(index.data(Qt::UserRole).toInt());
        break;
      case LayerRowButton::Delete:
        emit itemRemoved(index.data(Qt::UserRole).toInt());
        break;
      }
    }
    else if (mouseEvent->buttons() & Qt::RightButton && index.isValid())
    {
      emit itemContextMenuRequested(mouseEvent->pos());
    }
  }
  return QAbstractItemDelegate::editorEvent(event, model, option, index);
}

}
