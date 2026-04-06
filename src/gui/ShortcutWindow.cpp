/*
 * ShortcutWindow.cpp
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

#include "ShortcutWindow.h"

#include "ConsoleWindow.h"
#include "MainWindow.h"
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

namespace mmp {

ShortcutWindow::ShortcutWindow() :
  _textBrowser(new QTextBrowser(this))
{
  // Set window size
  resize(SHORTCUT_WINDOW_WIDTH, SHORTCUT_WINDOW_HEIGHT);
  // Set window title
  setWindowTitle(tr("%1 - Keyboard Shortcuts").arg(MM::APPLICATION_NAME));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_textBrowser);

  _textBrowser->setOpenExternalLinks(true);
  _textBrowser->setContextMenuPolicy(Qt::NoContextMenu);

  // Create and customize font
  int sansSerif = QFontDatabase::addApplicationFont(":/base-font");
  int serif = QFontDatabase::addApplicationFont(":/console-font");
  if (sansSerif >= 0)
  {
    QFont sansSerifFont(QFont(QFontDatabase::applicationFontFamilies(sansSerif).at(0), 11, QFont::Normal));
    _textBrowser->document()->setDefaultFont(sansSerifFont);
  }
  if (serif >= 0)
  {
    QFont serifFont(QFont(QFontDatabase::applicationFontFamilies(serif).at(0), 10, QFont::Normal));
    _textBrowser->setStyleSheet(QString("pre, code { font-family: '%1'; }").arg(serifFont.family()));
  }

  reload();
}

void ShortcutWindow::reload()
{
  // Build HTML file to render
  QString htmlContent("<!DOCTYPE html>\n<html>\n<head>\n");
  htmlContent.append("<meta charset=\"utf-8\">\n");
  htmlContent.append("<style>\n");

  // Load CSS file.
  QFile cssFile(":/shortcut-css");
  if (cssFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    htmlContent.append(QTextCodec::codecForName("UTF-8")->toUnicode(cssFile.readAll()));
  }
  htmlContent.append("\n</style>\n");
  htmlContent.append("</head>\n<body>");

  // Beginning of body content.
  QFile htmlFile(":/index-html");
  if (htmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    htmlContent.append(QTextCodec::codecForName("UTF-8")->toUnicode(htmlFile.readAll()));
  }
  htmlContent.append("</body></html>");

  _textBrowser->setHtml(htmlContent);
  _textBrowser->moveCursor(QTextCursor::Start);

}

}
