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

namespace mmp {

ShortcutWindow::ShortcutWindow()
{
  // Set window size
  resize(SHORTCUT_WINDOW_WIDTH, SHORTCUT_WINDOW_HEIGHT);
  // Set window title
  setWindowTitle(tr("%1 - Keyboard Shortcuts").arg(MM::APPLICATION_NAME));
//  setGeometry();

  // Build HTML file to render
  QString htmlContent("<!DOCTYPE html>\n<html>\n<head>\n");
  htmlContent.append("<meta charset=\"utf-8\">\n");
  htmlContent.append("<style>\n");
  // load CSS file
  QFile cssFile(":/shortcut-css");
  cssFile.open(QIODevice::ReadOnly | QIODevice::Text);
  htmlContent.append(QTextCodec::codecForName("UTF-8")->toUnicode(cssFile.readAll()));
  htmlContent.append("\n</style>\n");
  htmlContent.append("</head>\n<body>");

  // Begining of the body content
  // Load another HTML file
  QFile htmlFile(":/index-html");
  htmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
  htmlContent.append(QTextCodec::codecForName("UTF-8")->toUnicode(htmlFile.readAll()));
  // End of body content

  htmlContent.append("</body></html>");

  // Set up web page
  QWebEnginePage *shortcutWebPage = new QWebEnginePage;
  shortcutWebPage->setHtml(htmlContent);
  // Set main page
  setPage(shortcutWebPage);

  // Disable context menu
  setContextMenuPolicy(Qt::NoContextMenu);

  // Create and customize font
  int sansSerif = QFontDatabase::addApplicationFont(":/base-font");
  int serif = QFontDatabase::addApplicationFont(":/console-font");
  QFont sansSerifFont(QFont(QFontDatabase::applicationFontFamilies(sansSerif).at(0), 11, QFont::Normal));
  QFont serifFont(QFont(QFontDatabase::applicationFontFamilies(serif).at(0), 10, QFont::Normal));
  // Apply font to the document
  settings()->setFontFamily(QWebEngineSettings::SansSerifFont, sansSerifFont.family());
  settings()->setFontFamily(QWebEngineSettings::SerifFont, serifFont.family());

}

}
