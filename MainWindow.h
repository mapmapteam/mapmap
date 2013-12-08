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

#include "SourceGLCanvas.h"
#include "DestinationGLCanvas.h"

#include "MappingManager.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#define LIBREMAPPING_VERSION "0.1"

class MainWindow: public QMainWindow
{
Q_OBJECT

public:
  MainWindow();
  ~MainWindow();
  static MainWindow& getInstance();
  static void setInstance(MainWindow* inst);

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
  void handleLayerItemSelectionChanged();
  void handleLayerItemChanged(QListWidgetItem* item);
  void handleLayerIndexesMoved();
  void addQuad();
  void addTriangle();

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
  void addLayerItem(uint layerId);
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
  QListWidget* layerList;
  QStackedWidget* propertyPanel;

  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;

  QSplitter* mainSplitter;
  QSplitter* resourceSplitter;
  QSplitter* canvasSplitter;

  // Maps from Mapping id to corresponding mapper.
  std::map<uint, Mapper::ptr> mappers;

private:
  // Model.
  MappingManager* mappingManager;

  // View.
  uint currentPaintId;
  uint currentMappingId;
  bool _hasCurrentMapping;
  bool _hasCurrentPaint;

  static MainWindow* instance;

public:
  MappingManager& getMappingManager() { return *mappingManager; }
  Mapper::ptr getMapperByMappingId(uint id) { return mappers[id]; }
  uint getCurrentPaintId() const { return currentPaintId; }
  uint getCurrentMappingId() const { return currentMappingId; }
  bool hasCurrentPaint() const { return _hasCurrentPaint; }
  bool hasCurrentMapping() const { return _hasCurrentMapping; }
  void setCurrentPaint(int uid)
  {
    currentPaintId = uid;
    _hasCurrentPaint = true;
  }
  void setCurrentMapping(int uid)
  {
    currentMappingId = uid;
    if (uid != -1)
      propertyPanel->setCurrentWidget(mappers[uid]->getPropertiesEditor());
    _hasCurrentMapping = true;
  }
  void removeCurrentPaint() {
    _hasCurrentPaint = false;
  }
  void removeCurrentMapping() {
    _hasCurrentMapping = false;
  }

public slots:
  void updateAll();


public:
  // Constants.
  static const int DEFAULT_WIDTH = 1600;
  static const int DEFAULT_HEIGHT = 800;
  static const int SOURCE_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int SOURCE_LIST_MINIMUM_WIDTH = 100;
  static const int LAYER_LIST_MINIMUM_WIDTH  = 300;
  static const int PROPERTY_PANEL_MINIMUM_WIDTH  = 400;
  static const int CANVAS_MINIMUM_WIDTH  = 480;
  static const int CANVAS_MINIMUM_HEIGHT = 270;
};

#endif /* MAIN_WINDOW_H_ */
