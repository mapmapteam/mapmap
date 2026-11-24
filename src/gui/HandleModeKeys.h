/*
 * HandleModeKeys.h
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

#ifndef HANDLE_MODE_KEYS_H_
#define HANDLE_MODE_KEYS_H_

#include <QObject>

#include "ShapeHandles.h"

namespace mmp {

/**
 * Holds the mode of the handle of the current shape, which the R (rotate
 * only), S (scale only) and T (both: free) keys choose. It is an
 * application-wide event filter. Outside a drag, the keys act alone (Shift + R
 * keeps turning the layer by 90 degrees); during a drag, they act with Shift,
 * Ctrl or Alt held, and never trigger a shortcut, not even by key repetition.
 * They are ignored in text fields and dialogs; in item views (the list of
 * layers), they change the mode instead of searching the list.
 */
class HandleModeKeys : public QObject
{
  Q_OBJECT

public:
  /// Creates the filter, in free mode; install it with qApp->installEventFilter().
  explicit HandleModeKeys(QObject* parent = nullptr);

  /// The current mode.
  HandleMode mode() const { return _mode; }

  /// Changes the mode (from a key or a click on the handle).
  void setMode(HandleMode mode);

  /// Tells whether a handle drag is in progress.
  void setDragActive(bool active) { _dragActive = active; }

signals:
  /// The mode changed: the handle must be drawn again.
  void modeChanged();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Returns true iff key chooses a mode, which is then put in mode.
  static bool _modeOfKey(int key, HandleMode* mode);

  /// Returns true iff the focus is in a widget where one types text.
  static bool _focusIsInTextField();

  /// Returns true iff R, S and T may be taken for the handle: not in a dialog
  /// (modal or not) and not while typing text.
  static bool _keysAreForTheHandle();

  HandleMode _mode;
  bool _dragActive;
};

}

#endif /* HANDLE_MODE_KEYS_H_ */
