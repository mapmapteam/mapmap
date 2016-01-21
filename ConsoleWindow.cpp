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

ConsoleWindow* ConsoleWindow::_singleton = NULL;

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
  QFont font(QFont(":/base-font", 10, QFont::Medium));
  font.setStyleHint(QFont::Monospace, QFont::PreferAntialias);
  _console->setFont(font);
  // Set color scheme
  QPalette scheme = palette();
  scheme.setColor(QPalette::Base, Qt::black);
  scheme.setColor(QPalette::Text, Qt::white);
  _console->setPalette(scheme);

  // Create view elements
  createActions();
  createMenu();

  // Set window title
  setWindowTitle(tr("Message Log Output - Mapmap"));
  // Set main widget
  setCentralWidget(_console);
}

ConsoleWindow *ConsoleWindow::getInstance()
{
  if (!_singleton)
    _singleton = new ConsoleWindow;

  return _singleton;
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

void ConsoleWindow::messageLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  // Message
  QByteArray message = msg.toLocal8Bit();
  // Context
  QString contexts(QStringLiteral("%1:%2").arg(context.file).arg(context.line));
  // Date and time
  QString time(QDateTime::currentDateTime().toString(tr("MMM dd yy HH:mm")));
  // Output
  QString output;

  switch (type) {
  case QtDebugMsg:
    output = time + " | Debug: " + QString(message.constData()) + " - " + contexts;
    break;
  case QtInfoMsg:
    output = time + " | Info: " + QString(message.constData()) + " - " + contexts;
    break;
  case QtWarningMsg:
    output = time + " | Warning: " + QString(message.constData()) + " - " + contexts;
    break;
  case QtCriticalMsg:
    output = time + " | Critical: " + QString(message.constData()) + " - " + contexts;
    break;
  case QtFatalMsg:
    output = time + " | Fatal: " + QString(message.constData()) + " - " + contexts;
    abort();
  }
  // Print in console
  _console->appendPlainText(output);
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
  // Send signal if the window is closed
  emit windowClosed();
  event->accept();
}

void ConsoleWindow::kill()
{
  if (_singleton) {
    delete _singleton;
    _singleton = NULL;
  }
}

ConsoleWindow::~ConsoleWindow()
{
  kill();
}



