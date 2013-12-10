/*
 * MainWindow.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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
#include <QTimer>
#include <QVariant>
#include "SourceGLCanvas.h"
#ifdef HAVE_OSC
#include "OscInterface.h"
#endif
#include "DestinationGLCanvas.h"
#include "MappingManager.h"

#define LIBREMAPPING_VERSION "0.1"

class MainWindow: public QMainWindow
{
Q_OBJECT

public:
  ~MainWindow();
  static MainWindow& getInstance();
  void applyOscCommand(QVariantList & command);

private:
  MainWindow();
  MainWindow(MainWindow const&);
  void operator=(MainWindow const&);

protected:
  // Events.
  void closeEvent(QCloseEvent *event);

private slots:
  // Slots.
  void newFile();
  void open();
  bool save();
  bool saveAs();
  /**
   * Action that will call importMediaFile.
   */
  void import();
  void about();
  void updateStatusBar();

  void handleSourceItemSelectionChanged();
  void handleShapeItemSelectionChanged();
  void addQuad();
  void addTriangle();

  void windowModified();
  void pollOscInterface();

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
  void startOscReceiver();
  bool okToContinue();
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  bool importMediaFile(const QString &fileName);
  void addMappingItem(int mappingId);
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

  QAction *addQuadAction;
  QAction *addTriangleAction;

  QListWidget* sourceList;
  QListWidget* shapeList;

  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;

  QSplitter* mainSplitter;
  QSplitter* sourceSplitter;
  QSplitter* canvasSplitter;

#ifdef HAVE_OSC
  OscInterface::ptr osc_interface;
#endif
  int config_osc_receive_port;
  QTimer *osc_timer;

private:
  // Model.
  MappingManager* mappingManager;

  // View.
  int currentPaintId;
  int currentMappingId;

public:
  MappingManager& getMappingManager() { return *mappingManager; }
  int getCurrentPaintId() const { return currentPaintId; }
  int getCurrentMappingId() const { return currentMappingId; }
  void setCurrentPaint(int id)
  {
    currentPaintId = id;
  }
  void setCurrentMapping(int id)
  {
    currentMappingId = id;
  }

public:
  // Constants.
  static const int DEFAULT_WIDTH = 1600;
  static const int DEFAULT_HEIGHT = 800;
  static const int SOURCE_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int CANVAS_MINIMUM_WIDTH  = 320;
  static const int CANVAS_MINIMUM_HEIGHT = 240;
};

#endif /* MAIN_WINDOW_H_ */
