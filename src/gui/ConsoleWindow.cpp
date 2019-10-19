/*
 * ConsoleWindow.cpp
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

#include "ConsoleWindow.h"
#include <QtWidgets>

namespace mmp {

ConsoleWindow* ConsoleWindow::instance = NULL;

ConsoleWindow::ConsoleWindow(QWidget *parent) : QMainWindow(parent)
{
  // Set Fixed size
  resize(CONSOLE_WINDOW_DEFAULT_WIDTH, CONSOLE_WINDOW_DEFAULT_HEIGHT);
  setMinimumSize(CONSOLE_WINDOW_DEFAULT_WIDTH, CONSOLE_WINDOW_DEFAULT_HEIGHT);
  // Create console
  _console = new QPlainTextEdit(this);
  // Make read-only but allow copy of text
  _console->setReadOnly(true);
  // Create and customize font
  int id = QFontDatabase::addApplicationFont(":/console-font");
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  QFont font(QFont(family, 10, QFont::Normal));
  _console->setFont(font);

  // Set color scheme
  QPalette scheme = palette();
  scheme.setColor(QPalette::Base, QColor("#00020E"));
  scheme.setColor(QPalette::Text, Qt::white);
  _console->setPalette(scheme);

  // Create view elements
  createActions();
  createMenu();

  // Set window title
  setWindowTitle(tr("Message Log Output - Mapmap"));
  // Set window icon
  setWindowIcon(QIcon(":/mapmap-logo"));
  // Set main widget
  setCentralWidget(_console);
}

ConsoleWindow *ConsoleWindow::console()
{
  if (!instance)
    instance = new ConsoleWindow;

  return instance;
}

void ConsoleWindow::createActions()
{
  // Quit
  quitAction = new QAction(tr("&Close"), this);
  quitAction->setShortcut(QKeySequence::Close);
  quitAction->setStatusTip(tr("Close the console"));
  connect(quitAction, SIGNAL(triggered(bool)), this, SLOT(close()));
}

void ConsoleWindow::createMenu()
{
  // File menu
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addSeparator();
  fileMenu->addAction(quitAction);
}

void ConsoleWindow::writeLogFile(const QString &message)
{
  QString logFilePath = QDir(QDir::tempPath()).filePath("mapmap.log");
  QFile logFile(logFilePath);
  logFile.open(QIODevice::Append);
  QTextStream stream(&logFile);
  stream << message << endl;
}

void ConsoleWindow::printMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  // Message
  QByteArray message = msg.toLocal8Bit();
  // Context
  QString contexts(QStringLiteral("%1:%2").arg(context.file).arg(context.line)),
      // Date and time
      time(QDateTime::currentDateTime().toString(tr("MMM dd yy HH:mm"))),
      debug = "<strong style=\"color: #00FF00;\">Debug:</strong>",
      info = "<strong style=\"color: #1E90FF;\">Info:</strong>",
      warning = "<strong style=\"color: #FFFF00;\">Warning:</strong>",
      critical = "<strong style=\"color: #FF6600;\">Critical:</strong>",
      fatal = "<strong style=\"background: #FF0000;\">Fatal!</strong>";
  // Output
  QString output;

  switch (type) {
  case QtDebugMsg:
    output = time + " | " + debug + " " + QString(message.constData()) + " - <strong>" + contexts + "</strong>";
    break;
#if QT_VERSION >= 0x050500
  case QtInfoMsg:
    output = time + " | " + info + " " + QString(message.constData()) + " - <strong>" + contexts + "</strong>";
    break;
#endif
  case QtWarningMsg:
    output = time + " | " + warning + " " + QString(message.constData()) + " - <strong>" + contexts + "</strong>";
    break;
  case QtCriticalMsg:
    output = time + " | " + critical + " " + QString(message.constData()) + " - <strong>" + contexts + "</strong>";
    break;
  case QtFatalMsg:
    output = time + " | " + fatal + " " + QString(message.constData()) + " - <strong>" + contexts + "</strong>";
    abort();
  }
  // Print in console
  _console->appendHtml(output);

  // Write also on log file
  writeLogFile(output.remove(QRegExp("<[^>]*>")));
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
  // Send signal if the window is closed
  emit windowClosed();
  event->accept();
}

void ConsoleWindow::kill()
{
  if (instance) {
    delete instance;
    instance = NULL;
  }
}

ConsoleWindow::~ConsoleWindow()
{
  kill();
}

}
