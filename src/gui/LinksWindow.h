/*
 * LinksWindow.h
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

#ifndef LINKS_WINDOW_H_
#define LINKS_WINDOW_H_

#include <QWidget>

class QLabel;
class QTableWidget;

namespace mmp {

class MappingManager;

/**
 * The window listing the links of the project: one row per link, with its trigger, the kind
 * of its trigger, the element and the setting it drives, what it does to the setting (toggle
 * it, or always turn it on, or off), and a button to remove it.
 *
 * It is a window, not a dialog: the learned keys keep acting while it has the focus.
 */
class LinksWindow : public QWidget
{
  Q_OBJECT

public:
  /// The columns of the list.
  enum Column { TriggerColumn, KindColumn, ElementColumn, SettingColumn, ActionColumn, RemoveColumn, ColumnCount };

  /// Creates the window for the links of a project.
  explicit LinksWindow(MappingManager* manager, QWidget* parent = nullptr);

  /// How long the row of a link stays lit when its trigger arrives, in milliseconds.
  static const int FLASH_MS = 400;

  /// Returns the list, for tests.
  QTableWidget* table() const { return _table; }

  /// Returns true while the row of the link at index is lit, for tests.
  bool isLit(int index) const;

public slots:
  /// Shows the links again, as when an element was renamed or deleted.
  void refresh();

  /// Lights the row of the link at index for a moment: its trigger just arrived.
  void flash(int index);

protected:
  void showEvent(QShowEvent* event) override;

private:
  /// Paints the row of the link at index lit or not.
  void _paintRow(int index, bool lit);

  MappingManager* _manager;
  /// The triggers whose rows are lit, as written in project files.
  QStringList _lit;
  QLabel* _empty;
  QTableWidget* _table;
};

} // namespace mmp

#endif /* LINKS_WINDOW_H_ */
