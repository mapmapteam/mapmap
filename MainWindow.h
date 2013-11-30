/*
 * MainWindow.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QtGui>

#include "Common.h"

#include "DestinationGLCanvas.h"
#include "SourceGLCanvas.h"

class MainWindow: public QMainWindow
{
Q_OBJECT

public:
  MainWindow();

protected:
  // Events.
  void closeEvent(QCloseEvent *event);

private slots:
  // Slots.
  void newFile();
  void open();
  bool save();
  bool saveAs();
  void import();
  void about();
  void updateStatusBar();
  void handleSourceItemSelectionChanged();

  void windowModified();

private:
  // Methods.
  void createLayout();
  void createActions();
  void createMenus();
  void createContextMenu();
  void createToolBars();
  void createStatusBar();
  void readSettings();
  void writeSettings();
  bool okToContinue();
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  bool importFile(const QString &fileName);
  void clearWindow();
  QString strippedName(const QString &fullFileName);

  // Variables.
  QString curFile;

  // GUI elements.
  QAction *separatorAction;

  QMenu *fileMenu;
//  QMenu *editMenu;
//  QMenu *selectSubMenu;
//  QMenu *toolsMenu;
//  QMenu *optionsMenu;
  QMenu *helpMenu;
  QToolBar *fileToolBar;
//  QToolBar *editToolBar;
  QAction *newAction;
  QAction *openAction;
  QAction *importAction;
  QAction *saveAction;
  QAction *saveAsAction;
  QAction *exitAction;
//  QAction *cutAction;
//  QAction *copyAction;
//  QAction *pasteAction;
//  QAction *deleteAction;
  QAction *aboutAction;

  QListWidget* sourceList;
  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;

  QSplitter* mainSplitter;
  QSplitter* canvasSplitter;
};

#endif /* MAIN_WINDOW_H_ */
