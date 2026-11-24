/*
 * HandleModeKeys.cpp
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

#include "HandleModeKeys.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>

namespace mmp {

namespace {

/// Modifiers that make R, S and T a shortcut outside a drag.
const Qt::KeyboardModifiers SHORTCUT_MODIFIERS =
  Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

}

HandleModeKeys::HandleModeKeys(QObject* parent)
  : QObject(parent),
    _mode(HandleMode::Free),
    _dragActive(false)
{}

void HandleModeKeys::setMode(HandleMode mode)
{
  if (mode == _mode)
    return;
  _mode = mode;
  emit modeChanged();
}

bool HandleModeKeys::_modeOfKey(int key, HandleMode* mode)
{
  switch (key)
  {
  case Qt::Key_R: *mode = HandleMode::Rotate; return true;
  case Qt::Key_S: *mode = HandleMode::Scale;  return true;
  case Qt::Key_T: *mode = HandleMode::Free;   return true;
  default:        return false;
  }
}

bool HandleModeKeys::_focusIsInTextField()
{
  QWidget* focus = QApplication::focusWidget();
  return qobject_cast<QLineEdit*>(focus) || qobject_cast<QTextEdit*>(focus)
      || qobject_cast<QPlainTextEdit*>(focus) || qobject_cast<QAbstractSpinBox*>(focus);
}

bool HandleModeKeys::_keysAreForTheHandle()
{
  // Not in a modal dialog (file dialogs, preferences) nor in any dialog, where
  // the letters are the dialog's, and not while typing text.
  if (QApplication::activeModalWidget() || _focusIsInTextField())
    return false;
  QWidget* focus = QApplication::focusWidget();
  return !(focus && qobject_cast<QDialog*>(focus->window()));
}

bool HandleModeKeys::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride)
    return QObject::eventFilter(watched, event);

  QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
  HandleMode mode;
  if (!_modeOfKey(keyEvent->key(), &mode) || !_keysAreForTheHandle())
    return QObject::eventFilter(watched, event);

  // During a drag, and whenever a held key repeats, R, S and T belong to the
  // handle: no shortcut fires (Shift + R, Ctrl + S...) and no widget gets the
  // key (the list of layers would search for a name).
  const bool taken = _dragActive || keyEvent->isAutoRepeat();
  if (event->type() == QEvent::ShortcutOverride)
  {
    if (!taken)
      return QObject::eventFilter(watched, event);
    event->accept();
    return true;
  }

  // Key press: alone, or with modifiers during a drag.
  if (keyEvent->isAutoRepeat())
    return true;
  if (!_dragActive && (keyEvent->modifiers() & SHORTCUT_MODIFIERS))
    return QObject::eventFilter(watched, event);
  setMode(mode);

  // In the list of layers, the letter changes the mode instead of searching.
  if (taken || qobject_cast<QAbstractItemView*>(QApplication::focusWidget()))
    return true;
  return QObject::eventFilter(watched, event);
}

}
