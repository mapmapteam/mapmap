/*
 * LinkKeys.h
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

#ifndef LINK_KEYS_H_
#define LINK_KEYS_H_

#include <QList>
#include <QObject>
#include <QString>

class QAction;
class QKeyEvent;

namespace mmp {

class LinkTable;

/// Name of the property that lets a dialog, such as the output window, receive learned keys.
extern const char* const ACCEPTS_LEARNED_KEYS_PROPERTY;

/// Returns the key of an event, with its modifiers, in the portable text of QKeySequence,
/// such as "M" or "Shift+M"; an empty text for a modifier pressed alone or an unknown key.
/// A key of the numeric keypad is the same key as on the main keyboard.
QString keyLinkText(const QKeyEvent* event);

/**
 * Returns the translated name of the command of MapMap that already uses a key, given in the
 * portable text of QKeySequence, or an empty text when the key is free for learning.
 *
 * The list is computed: the shortcuts of the actions given (the commands of the menus), and
 * the keys of the editors, which are not actions (R, S and T choose the mode of the handle;
 * the arrows and Space move and choose the vertices, with or without modifiers; Escape,
 * Delete, Backspace, Tab and Return are kept for the interface).
 */
QString commandOfKey(const QString& key, const QList<QAction*>& actions);

/// Returns true when the focus is in a field where one types text.
bool focusIsInTextField();

/// Returns true when the keys received by receiver, or by the focus widget when it is not a
/// widget, are the application's: no modal dialog, no text field, no dialog, except a window
/// with ACCEPTS_LEARNED_KEYS_PROPERTY set, such as the output window.
bool keysAreForLinks(QObject* receiver);

/**
 * Listens to the keys of the whole application for the learned ones; install it with
 * qApp->installEventFilter().
 *
 * A learned key acts once per press, not while it repeats; it acts nowhere keys are for typing
 * (see keysAreForLinks()) and never while the keys are being learned. While learning, the
 * filter takes every key: Escape stops learning, Delete and Backspace remove the link of the
 * target chosen, and any other key is offered for it, even a key that is a shortcut.
 */
class KeyLinkFilter : public QObject
{
  Q_OBJECT

public:
  /// Creates a filter listening to the links of table.
  explicit KeyLinkFilter(LinkTable* table, QObject* parent = nullptr);

  /// Turns learning on or off.
  void setLearning(bool learning) { _learning = learning; }

  /// Returns true while learning.
  bool isLearning() const { return _learning; }

  /// Turns the learned keys on or off; learning works either way.
  void setLinksActive(bool active) { _linksActive = active; }

  /// Returns true when learned keys act.
  bool linksActive() const { return _linksActive; }

signals:
  /// A learned key was pressed: the link at index of the table must act.
  void linkTriggered(int index);

  /// While learning, a key was pressed, in the portable text of QKeySequence.
  void keyOffered(const QString& key);

  /// While learning, Delete or Backspace was pressed.
  void removeRequested();

  /// While learning, Escape was pressed.
  void learningCancelled();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Handles a key while learning; returns true when it is taken.
  bool _learn(QKeyEvent* event);

  /// Returns true when a key press is the one just handled, reaching the filter a second time
  /// (as on macOS, where the same press may be delivered by two ways); remembers it otherwise.
  bool _isHandledAlready(const QKeyEvent* event);

  LinkTable* _table;
  bool _learning;
  bool _linksActive;
  /// The time and the key of the last press handled.
  quint64 _lastTimestamp;
  int _lastKey;
};

} // namespace mmp

#endif /* LINK_KEYS_H_ */
