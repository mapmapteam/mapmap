/*
 * SettingsBrowser.h
 *
 * (c) 2026 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef SETTINGS_BROWSER_H_
#define SETTINGS_BROWSER_H_

#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "Setting.h"

class QtProperty;
class QtVariantProperty;
class QtTreePropertyBrowser;
class VariantManager;

namespace mmp {

/**
 * Fills a property browser from a settings tree and keeps both in step.
 *
 * Each visible setting gets one row, in the order of the tree; a group of the tree and a
 * display group each give a group of rows, folded when the display group says so. The rows
 * made here go before any row added to the browser by other code, which is left alone.
 *
 * No Setting is kept across calls: a row remembers the path of its setting and looks it up
 * in the tree each time, since a node may disappear when the structure changes.
 */
class SettingsBrowser : public QObject {
  Q_OBJECT

public:
  /// Number of decimals of a number row; values are compared at this precision.
  static constexpr int PANEL_DECIMALS = 1;

  /// Fills browser, through manager, with the settings of root, the topmost group of a tree.
  SettingsBrowser(SettingGroup* root, QtTreePropertyBrowser* browser,
                  VariantManager* manager, QObject* parent = nullptr);

  /// Removes the rows made here from the browser.
  ~SettingsBrowser() override;

  /// Titles the group of rows of the tree group at path, instead of its id, now and after rebuilds.
  void setGroupLabel(const QString& path, const QString& label);

public slots:
  /// Makes the rows again from the tree; call after a structure change. When it comes from an
  /// edit made in the panel, it waits for the edit to end, then runs from the event loop.
  void rebuild();

  /// Shows the current value of the setting, or of the list, at path; call after a value
  /// change. The row reads the value from the tree, as shown to the user, so value is not
  /// needed; a row already showing that value is not written again.
  void refresh(const QString& path, const QVariant& value);

signals:
  /// The user changed a value through the panel and the tree accepted it.
  void edited();

private slots:
  /// Writes to the tree a value the user entered in a row.
  void _writeRow(QtProperty* property, const QVariant& value);

private:
  /// What a row shows: a setting, or one bound of a Range.
  struct Row {
    /// Path of the setting, relative to the root.
    QString path;
    /// Type of the setting when the row was made.
    SettingInfo::Type type;
    /// For a Range, the bound shown: RANGE_LOW or RANGE_HIGH; WHOLE_VALUE otherwise.
    int part;
    /// For a Choice or a Reference, the id of each entry of the menu.
    QStringList optionIds;
  };

  /// Returns the rows and groups of rows for the children of group, in their order.
  QList<QtProperty*> _buildGroup(SettingGroup* group);

  /// Makes the row, or the group of rows of a Range, for a setting.
  QtProperty* _buildRow(Setting* setting, const QString& label);

  /// Makes a number row for one bound of a Range.
  QtVariantProperty* _buildRangePart(Setting* setting, const QString& label, int part);

  /// Makes a group of rows, remembered under key for its folded state.
  QtProperty* _buildFold(const QString& key, const QString& label, bool collapsedByDefault);

  /// Makes a property of the manager, kept to be deleted by _clear().
  QtVariantProperty* _addProperty(int propertyType, const QString& label);

  /// Remembers a row showing a setting and shows its value.
  void _addRow(QtVariantProperty* property, Setting* setting, int part);

  /// Returns the label of a row, followed by the unit of a number between parentheses.
  static QString _labelWithUnit(const SettingInfo& info, const QString& label);

  /// Returns the short text of a unit, e.g. "%"; empty for NoUnit.
  static QString _unitSymbol(SettingInfo::Unit unit);

  /// Shows on every row of the setting its current value, unless a row already shows it.
  void _showSetting(Setting* setting);

  /// Shows on the row the current value of setting, unless it already shows it.
  void _showRow(QtVariantProperty* property, Row& row, Setting* setting);

  /// Returns the value the row should show; for a menu, also updates its entries.
  QVariant _rowValue(QtVariantProperty* property, Row& row, Setting* setting);

  /// Returns true when a row showing shown already shows wanted, at the panel's precision.
  bool _showsValue(const Row& row, const QVariant& shown, const QVariant& wanted) const;

  /// Returns the setting the row was made for, or nullptr when the tree no longer has it.
  Setting* _findSetting(const Row& row) const;

  /// Remembers which groups of rows are open, then deletes every row made here.
  void _clear();

  QPointer<QtTreePropertyBrowser> _browser;
  QPointer<VariantManager> _manager;
  SettingGroup* _root;

  /// Every property made here, groups included.
  QList<QtProperty*> _properties;
  /// The rows showing a setting, by property.
  QHash<QtProperty*, Row> _rows;
  /// The rows showing a setting, by path of the setting.
  QMultiHash<QString, QtVariantProperty*> _rowsByPath;
  /// The groups of rows, by key.
  QHash<QString, QtProperty*> _folds;
  /// Whether each group of rows is open, by key, kept across rebuilds.
  QHash<QString, bool> _expanded;
  /// Titles of the tree groups, by path.
  QHash<QString, QString> _groupLabels;

  /// Set while a row is written by this class, so that the write is not sent to the tree.
  bool _showing;
  /// Set while an edit is being written to the tree and edited() is being sent.
  bool _editing;
  /// Set when rebuild() was called during an edit.
  bool _rebuildPending;
};

} // namespace mmp

#endif /* SETTINGS_BROWSER_H_ */
