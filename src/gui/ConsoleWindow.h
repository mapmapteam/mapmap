/*
 * ConsoleWindow.h
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include "MM.h"
#include <QFont>

namespace mmp {

class ConsoleWindow : public QMainWindow
{
  Q_OBJECT

private:
  // Private constructor
  ConsoleWindow(QWidget *parent = 0);

public:
  // Using a singleton instance
  static ConsoleWindow *console();
  // Console log message handler
  void printMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);
  // Destructor
  ~ConsoleWindow();
  // This instance killer
  static void kill();

signals:
  void windowClosed();

protected:
  void closeEvent(QCloseEvent *event);

private:
  // This instance
  static ConsoleWindow *instance;

  // Console logger
  QPlainTextEdit *_console;

  // Create view elements
  void createActions();
  void createMenu();

  // Write log file
  void writeLogFile(const QString &message);

  // Actions
  QAction *quitAction;

  // Menus
  QMenu *fileMenu;

  // Constants
  static const int CONSOLE_WINDOW_DEFAULT_WIDTH = 1024;
  static const int CONSOLE_WINDOW_DEFAULT_HEIGHT = 768;
};

}

#endif // CONSOLE_H
