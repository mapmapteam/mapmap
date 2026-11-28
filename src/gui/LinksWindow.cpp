/*
 * LinksWindow.cpp
 *
 * (c) 2026 MapMap contributors
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

#include "LinksWindow.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "MappingManager.h"

namespace mmp {

namespace {

/// Size of the window when first shown, in pixels.
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 320;

/// Margins of the content of the window, in pixels.
const int WINDOW_MARGIN = 8;

/// The actions offered for a link, in the order of the menu.
const LinkAction ACTIONS[] = { LinkAction::Toggle, LinkAction::SwitchOn, LinkAction::SwitchOff };

} // namespace

LinksWindow::LinksWindow(MappingManager* manager, QWidget* parent)
  : QWidget(parent, Qt::Window),
    _manager(manager),
    _empty(new QLabel(this)),
    _table(new QTableWidget(this))
{
  setWindowTitle(tr("Links"));
  resize(WINDOW_WIDTH, WINDOW_HEIGHT);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(WINDOW_MARGIN, WINDOW_MARGIN, WINDOW_MARGIN, WINDOW_MARGIN);

  _empty->setText(tr("No link yet. Choose Edit > Keyboard Learn, click a control, then press a key."));
  _empty->setWordWrap(true);
  layout->addWidget(_empty);

  _table->setColumnCount(ColumnCount);
  _table->setHorizontalHeaderLabels({ tr("Trigger"), tr("Device"), tr("Element"), tr("Setting"), tr("Action"), QString() });
  _table->verticalHeader()->hide();
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _table->setSelectionBehavior(QAbstractItemView::SelectRows);
  _table->horizontalHeader()->setSectionResizeMode(ElementColumn, QHeaderView::Stretch);
  _table->horizontalHeader()->setSectionResizeMode(SettingColumn, QHeaderView::Stretch);
  layout->addWidget(_table, 1);

  // Later: the list is made again, which deletes the menu or the button that changed it.
  connect(&_manager->links(), &LinkTable::changed, this, &LinksWindow::refresh, Qt::QueuedConnection);
  refresh();
}

void LinksWindow::showEvent(QShowEvent* event)
{
  refresh();
  QWidget::showEvent(event);
}

void LinksWindow::refresh()
{
  const QList<Link>& links = _manager->links().links();
  _empty->setVisible(links.isEmpty());
  _table->setVisible(!links.isEmpty());
  _table->setRowCount(links.size());

  for (int row = 0; row < links.size(); ++row) {
    const Link& link = links.at(row);
    const Element::ptr element = _manager->linkTargetElement(link.target);
    const Setting* setting = _manager->linkTargetSetting(link.target);

    // A link to a deleted element does nothing until the deletion is undone.
    const QString elementName = element ? element->getName() : tr("(deleted)");
    const QString settingName = setting ? setting->info().label() : link.target.path;

    const QStringList texts = { link.trigger.text(), link.trigger.kindText(), elementName, settingName };
    for (int column = 0; column < texts.size(); ++column) {
      QTableWidgetItem* item = new QTableWidgetItem(texts.at(column));
      item->setFlags(element ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::ItemIsSelectable);
      _table->setItem(row, column, item);
    }

    // A link toggles its setting, or always turns it on, or off.
    QComboBox* action = new QComboBox(_table);
    for (LinkAction choice : ACTIONS)
      action->addItem(linkActionText(choice, setting), int(choice));
    action->setCurrentIndex(action->findData(int(link.action)));
    action->setEnabled(bool(element));
    connect(action, &QComboBox::activated, this, [this, link, action]() {
      const int index = _manager->links().indexOf(link.trigger);
      _manager->links().setAction(index, LinkAction(action->currentData().toInt()));
    });
    _table->setCellWidget(row, ActionColumn, action);

    QToolButton* remove = new QToolButton(_table);
    remove->setText(tr("Remove"));
    remove->setToolTip(tr("Remove this link"));
    connect(remove, &QToolButton::clicked, this, [this, link]() {
      // The index of the link may have changed since the list was shown.
      const int index = _manager->links().indexOf(link.trigger);
      _manager->links().removeAt(index);
    });
    _table->setCellWidget(row, RemoveColumn, remove);
  }
  _table->resizeColumnToContents(TriggerColumn);
  _table->resizeColumnToContents(KindColumn);
  _table->resizeColumnToContents(ActionColumn);
  _table->resizeColumnToContents(RemoveColumn);
}

} // namespace mmp
