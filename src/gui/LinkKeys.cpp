/*
 * LinkKeys.cpp
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

#include "LinkKeys.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTextEdit>

#include "Link.h"

namespace mmp {

const char* const ACCEPTS_LEARNED_KEYS_PROPERTY = "mapmapAcceptsLearnedKeys";

namespace {

/// The modifiers a learned key keeps; the keypad one is dropped.
const Qt::KeyboardModifiers KEPT_MODIFIERS =
  Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

/// A key of an editor, which is not the shortcut of an action.
struct EditorKey {
  /// The key.
  Qt::Key key;
  /// True when the key is taken with any modifiers, false when alone only.
  bool anyModifiers;
};

/// The keys of the editors and of the interface, which no action declares.
const EditorKey EDITOR_KEYS[] = {
  { Qt::Key_R, false }, { Qt::Key_S, false }, { Qt::Key_T, false },
  { Qt::Key_Left, true }, { Qt::Key_Right, true }, { Qt::Key_Up, true }, { Qt::Key_Down, true },
  { Qt::Key_Space, true },
  { Qt::Key_Escape, true }, { Qt::Key_Delete, true }, { Qt::Key_Backspace, true },
  { Qt::Key_Tab, true }, { Qt::Key_Backtab, true }, { Qt::Key_Return, true }, { Qt::Key_Enter, true },
};

/// Returns the translated name of what an editor key does.
QString editorKeyCommand(Qt::Key key)
{
  switch (key) {
  case Qt::Key_R: return QObject::tr("the rotation mode of the handle");
  case Qt::Key_S: return QObject::tr("the scaling mode of the handle");
  case Qt::Key_T: return QObject::tr("the free mode of the handle");
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Up:
  case Qt::Key_Down:
    return QObject::tr("moving the selected vertex");
  case Qt::Key_Space:
    return QObject::tr("choosing the next vertex");
  default:
    return QObject::tr("the keys of the interface");
  }
}

/// Returns the text of an action as shown in a sentence: without its mnemonic.
QString commandName(const QAction* action)
{
  QString text = action->text();
  text.remove(QRegularExpression(QStringLiteral("&(?!&)")));
  text.replace(QStringLiteral("&&"), QStringLiteral("&"));
  if (text.endsWith(QStringLiteral("...")))
    text.chop(3);
  return text;
}

} // namespace

QString keyLinkText(const QKeyEvent* event)
{
  const int key = event->key();
  switch (key) {
  case Qt::Key_Shift:
  case Qt::Key_Control:
  case Qt::Key_Alt:
  case Qt::Key_Meta:
  case Qt::Key_AltGr:
  case Qt::Key_CapsLock:
  case Qt::Key_NumLock:
  case Qt::Key_unknown:
  case 0:
    return QString();
  default:
    break;
  }
  const QKeyCombination combination(event->modifiers() & KEPT_MODIFIERS, Qt::Key(key));
  return QKeySequence(combination).toString(QKeySequence::PortableText);
}

QString commandOfKey(const QString& key, const QList<QAction*>& actions)
{
  const QKeySequence sequence = QKeySequence::fromString(key, QKeySequence::PortableText);
  if (sequence.count() != 1)
    return QString();

  for (const QAction* action : actions) {
    for (const QKeySequence& shortcut : action->shortcuts()) {
      if (shortcut.matches(sequence) == QKeySequence::ExactMatch)
        return commandName(action);
    }
  }

  const QKeyCombination combination = sequence[0];
  for (const EditorKey& editorKey : EDITOR_KEYS) {
    if (combination.key() != editorKey.key)
      continue;
    if (editorKey.anyModifiers || combination.keyboardModifiers() == Qt::NoModifier)
      return editorKeyCommand(editorKey.key);
  }
  return QString();
}

bool focusIsInTextField()
{
  QWidget* focus = QApplication::focusWidget();
  return qobject_cast<QLineEdit*>(focus) || qobject_cast<QTextEdit*>(focus)
      || qobject_cast<QPlainTextEdit*>(focus) || qobject_cast<QAbstractSpinBox*>(focus);
}

bool keysAreForLinks(QObject* receiver)
{
  if (QApplication::activeModalWidget() || focusIsInTextField())
    return false;
  QWidget* widget = qobject_cast<QWidget*>(receiver);
  if (!widget)
    widget = QApplication::focusWidget();
  QWidget* window = widget ? widget->window() : QApplication::activeWindow();
  if (window && qobject_cast<QDialog*>(window))
    return window->property(ACCEPTS_LEARNED_KEYS_PROPERTY).toBool();
  return true;
}

KeyLinkFilter::KeyLinkFilter(LinkTable* table, QObject* parent)
  : QObject(parent),
    _table(table),
    _learning(false),
    _linksActive(true),
    _lastTimestamp(0),
    _lastKey(0)
{
}

bool KeyLinkFilter::_isHandledAlready(const QKeyEvent* event)
{
  // A press without a time cannot be told from another one: it is never taken for a repeat.
  if (event->timestamp() == 0)
    return false;
  if (event->timestamp() == _lastTimestamp && event->key() == _lastKey)
    return true;
  _lastTimestamp = event->timestamp();
  _lastKey = event->key();
  return false;
}

bool KeyLinkFilter::_learn(QKeyEvent* event)
{
  // A dialog opened while learning, such as the preferences, keeps its keys.
  if (QApplication::activeModalWidget())
    return false;

  // Taken before the shortcuts: while learning, H is offered, it does not flip the layer.
  if (event->type() == QEvent::ShortcutOverride) {
    event->accept();
    return true;
  }
  // Taken and handled, so that the system does not send it again another way.
  event->accept();
  if (event->isAutoRepeat() || _isHandledAlready(event))
    return true;

  switch (event->key()) {
  case Qt::Key_Escape:
    emit learningCancelled();
    return true;
  case Qt::Key_Delete:
  case Qt::Key_Backspace:
    emit removeRequested();
    return true;
  default:
    break;
  }

  const QString key = keyLinkText(event);
  if (key.isEmpty())
    return false;
  emit keyOffered(key);
  return true;
}

bool KeyLinkFilter::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride)
    return QObject::eventFilter(watched, event);

  QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
  if (_learning)
    return _learn(keyEvent) || QObject::eventFilter(watched, event);

  if (!_linksActive || !_table || _table->isEmpty() || !keysAreForLinks(watched))
    return QObject::eventFilter(watched, event);

  const QString key = keyLinkText(keyEvent);
  const int index = key.isEmpty() ? -1 : _table->indexOf(LinkTrigger::forKey(key));
  if (index < 0)
    return QObject::eventFilter(watched, event);

  // A learned key belongs to its link: a list does not search for the letter, and a repeated
  // key does nothing more.
  if (event->type() == QEvent::ShortcutOverride) {
    event->accept();
    return true;
  }
  event->accept();
  if (!keyEvent->isAutoRepeat() && !_isHandledAlready(keyEvent))
    emit linkTriggered(index);
  return true;
}

} // namespace mmp
