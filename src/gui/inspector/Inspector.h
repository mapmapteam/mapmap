/*
 * Inspector.h
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

#ifndef INSPECTOR_H_
#define INSPECTOR_H_

#include <QHash>
#include <QList>
#include <QScrollArea>
#include <QSet>
#include <QWidget>

#include "SettingControl.h"

class QLabel;
class QMenu;
class QTabBar;
class QTabWidget;
class QTimer;
class QToolButton;
class QVBoxLayout;

namespace mmp {

/// The label of a row: dragging it moves the value of the row by steps.
class InspectorRowLabel : public QWidget {
  Q_OBJECT

public:
  /// Pixels the mouse moves for one step; with Shift, ten times more for one step.
  static const int PIXELS_PER_STEP = 1;

  /// Creates the label.
  InspectorRowLabel(const QString& text, QWidget* parent = nullptr);

  /// Sets the control the label adjusts, or nullptr for none.
  void setControl(SettingControl* control);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  QString _text;
  SettingControl* _control;
  int _lastX;
  bool _dragging;
};

/// One row of the inspector: a label, the control of one setting or of several settings
/// sharing the row, and room for the button of a link.
class InspectorRow : public QWidget {
  Q_OBJECT

public:
  /// Width of the column of the labels, in pixels.
  static const int LABEL_WIDTH = 104;

  /// Creates a row for controls already made.
  InspectorRow(const QString& label, const QList<SettingControl*>& controls, QWidget* parent = nullptr);

  /// Returns the controls of the row.
  QList<SettingControl*> controls() const { return _controls; }

  /// Returns the label, for tests.
  InspectorRowLabel* label() const { return _label; }

  /// Greys the row out when no setting of it can be changed.
  void updateEnabled();

  /// Returns the button showing that a setting of the row is linked, for tests.
  QToolButton* linkButton() const { return _linkButton; }

  /// Returns the menu to learn and remove the keys of the settings of the row; nullptr when
  /// no setting of the row can be linked. The caller owns it.
  QMenu* createLinkMenu(QWidget* parent);

signals:
  /// The user changed a value of the row.
  void edited();

protected:
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

private:
  /// Shows the triggers linked to the settings of the row: the button and the tooltip.
  void _showLinks();

  InspectorRowLabel* _label;
  QList<SettingControl*> _controls;
  QToolButton* _linkButton;
  QString _description;
};

/// A group of rows with a title that folds and unfolds it.
class InspectorGroup : public QWidget {
  Q_OBJECT

public:
  /// Creates a group; key names it in the preferences, where its folded state is kept.
  InspectorGroup(const QString& key, const QString& title, bool foldedByDefault, QWidget* parent = nullptr);

  /// Adds a row or a group under the title.
  void addWidget(QWidget* widget);

  /// Returns true when the group is folded.
  bool isFolded() const;

  /// Folds or unfolds the group, and remembers it in the preferences.
  void setFolded(bool folded);

  /// Returns the key of the group in the preferences.
  QString key() const { return _key; }

  /// Returns the key under which the folded state of a group is kept in the preferences.
  static QString preferenceKey(const QString& key);

private:
  QString _key;
  QToolButton* _header;
  QWidget* _body;
  QVBoxLayout* _bodyLayout;
};

/**
 * The rows of one tree of settings, in a scrolling area.
 *
 * A change of structure makes the rows again, once the current event is over, keeping the
 * folded groups; a change of value only shows the new value in its control. Values that
 * come from elsewhere (OSC, MIDI, the canvas) are shown at most ten times per second.
 */
class InspectorPage : public QScrollArea {
  Q_OBJECT

public:
  /// Delay between two showings of values that come from elsewhere, in milliseconds.
  static const int REFRESH_INTERVAL_MS = 100;

  /// Creates the page of the tree whose topmost group is root; key names its groups in the
  /// preferences.
  InspectorPage(SettingGroup* root, const QString& key, QWidget* parent = nullptr);

  /// Titles the group of the tree at path, instead of its id.
  void setGroupLabel(const QString& path, const QString& label);

  /// Shows another tree, such as the source a layer now uses; makes the rows again.
  void setRoot(SettingGroup* root);

  /// Returns the topmost group of the tree shown.
  SettingGroup* root() const { return _root; }

  /// Adds a row made by hand under the rows of the tree, kept when they are made again.
  /// For what the tree does not describe yet, such as the servers of a Syphon source.
  void addExtraRow(const QString& label, QWidget* widget);

  /// Shows a sentence above the rows, or hides it when text is empty.
  void setNotice(const QString& text);

  /// Returns the sentence shown above the rows.
  QString notice() const;

  /// Returns every control of the page, in the order of the rows.
  QList<SettingControl*> controls() const;

  /// Returns the control of the setting at path, or nullptr.
  SettingControl* control(const QString& path) const;

  /// Returns the row holding the control of the setting at path, or nullptr.
  InspectorRow* row(const QString& path) const;

  /// Returns the groups of the page.
  QList<InspectorGroup*> groups() const { return _groups; }

  /// Sets the function reading the live value of every control, now and after rebuilds.
  void setLiveValueReader(SettingControl::LiveValueReader reader);

  /// Returns how many times the rows were made, for tests.
  int buildCount() const { return _buildCount; }

public slots:
  /// Makes the rows again from the tree, once the current event is over.
  void rebuild();

  /// Shows the current value of the setting, or of the list, at path.
  void refresh(const QString& path);

signals:
  /// The user changed a value in the page.
  void edited();

private:
  /// Makes the rows now.
  void _build();

  /// Makes the rows of the children of group into layout.
  void _buildGroup(SettingGroup* group, QVBoxLayout* layout);

  /// Makes a row for settings sharing it.
  InspectorRow* _buildRow(const QString& label, const QList<Setting*>& settings);

  /// Shows the values waiting to be shown.
  void _showPending();

  /// Shows the live values of the modulable settings.
  void _showLiveValues();

  SettingGroup* _root;
  QString _key;
  QLabel* _notice;
  QWidget* _content;
  QWidget* _extras;
  QVBoxLayout* _rowsLayout;
  QWidget* _rows;
  QList<InspectorGroup*> _groups;
  QList<InspectorRow*> _rowList;
  QHash<QString, SettingControl*> _controls;
  QHash<QString, QString> _groupLabels;
  SettingControl::LiveValueReader _liveValueReader;
  QTimer* _refreshTimer;
  QTimer* _liveTimer;
  QSet<QString> _pending;
  bool _rebuildPending;
  int _buildCount;
};

/**
 * The inspector: a header naming the element, a tab per object (the surface of a layer, its
 * media, its effects) and the rows of each object.
 */
class Inspector : public QWidget {
  Q_OBJECT

public:
  /// Creates an empty inspector.
  explicit Inspector(QWidget* parent = nullptr);

  /// Gives a widget and its children the colours of MapMap's style sheet: the controls fill
  /// with its white rather than with the highlight colour of the system. A child added later
  /// needs another call.
  static void useMapMapColors(QWidget* widget);

  /// Shows the name of the element and its kind, such as "Video".
  void setHeader(const QString& name, const QString& kind);

  /// Returns the name shown in the header.
  QString headerName() const;

  /// Adds a tab showing a tree of settings; key names its groups in the preferences.
  InspectorPage* addPage(const QString& title, SettingGroup* root, const QString& key);

  /// Removes and deletes every tab.
  void clearPages();

  /// Returns the number of tabs.
  int pageCount() const;

  /// Returns the page of a tab.
  InspectorPage* page(int index) const;

  /// Returns the tab bar, for tests.
  QTabBar* tabBar() const;

signals:
  /// The user changed a value in one of the tabs.
  void edited();

private:
  QLabel* _name;
  QLabel* _kind;
  /// The tabs and their pages: the same widget as the other tabs of MapMap, so that they look
  /// the same.
  QTabWidget* _tabs;
};

} // namespace mmp

#endif /* INSPECTOR_H_ */
