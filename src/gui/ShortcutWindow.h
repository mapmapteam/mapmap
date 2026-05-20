/*
 * ShortcutWindow.h
 *
 * (c) 2020 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef SHORTCUTWINDOW_H
#define SHORTCUTWINDOW_H
#include <QDialog>
#include <QTextBrowser>
#include <QFile>
#include <QFontDatabase>

#include "MM.h"

namespace mmp {

class ShortcutWindow : public QDialog
{
  Q_OBJECT
public:
  ShortcutWindow();
  ~ShortcutWindow() {}
  void reload();

private:

  // Constantes
  static const int SHORTCUT_WINDOW_WIDTH = 960;
  static const int SHORTCUT_WINDOW_HEIGHT = 640;
  QTextBrowser *_textBrowser;

};

}

#endif // SHORTCUTWINDOW_H
