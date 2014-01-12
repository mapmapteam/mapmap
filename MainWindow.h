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

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#define LIBREMAPPING_VERSION "0.1"

// Forward declaration:
//class Facade;

class MainWindow: public QMainWindow
{
Q_OBJECT

public:
  MainWindow();
  ~MainWindow();
  static MainWindow& getInstance();
  static void setInstance(MainWindow* inst);
  void applyOscCommand(QVariantList & command);

protected:
  // Events.
  void closeEvent(QCloseEvent *event);

private slots:

  // Menu.
  void newFile();
  void open();
  bool save();
  bool saveAs();
  void import();
  void about();
  void updateStatusBar();

  // CRUD.

  /**
   * Create an image paint.
   */

  // Widget callbacks.
  void handlePaintItemSelectionChanged();
  void handleMappingItemSelectionChanged();
  void handleMappingItemChanged(QListWidgetItem* item);
  void handleMappingIndexesMoved();
  void addMesh();
  void addTriangle();

  void windowModified();
  void pollOscInterface();

public slots:
  /**
   * Create an image paint.
   */
  uid createImagePaint(uid paintId, QString uri, float x, float y);
  /**
   * Creates a textured triangle.
   */
  uid createTriangleTextureMapping(uid mappingId,
                                   uid paintId,
                                   const QList<QPointF> &src, const QList<QPointF> &dst);


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
  void addMappingItem(uint mappingId);
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

  QListWidget* paintList;
  QListWidget* mappingList;
  QStackedWidget* propertyPanel;

  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;

  QSplitter* mainSplitter;
  QSplitter* resourceSplitter;
  QSplitter* canvasSplitter;

#ifdef HAVE_OSC
  OscInterface::ptr osc_interface;
#endif
  int config_osc_receive_port;
  QTimer *osc_timer;

  // Maps from Mapping id to corresponding mapper.
  std::map<uint, Mapper::ptr> mappers;

private:
  // Model.
  MappingManager* mappingManager;
  //Facade* _facade;

  // View.
  uid currentPaintId;
  uid currentMappingId;
  bool _hasCurrentMapping;
  bool _hasCurrentPaint;

  static MainWindow* instance;

public:
  MappingManager& getMappingManager() { return *mappingManager; }
  Mapper::ptr getMapperByMappingId(uint id) { return mappers[id]; }
  uid getCurrentPaintId() const { return currentPaintId; }
  uid getCurrentMappingId() const { return currentMappingId; }
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
    if (uid != NULL_UID)
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
  static const int PAINT_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int PAINT_LIST_MINIMUM_WIDTH = 100;
  static const int MAPPING_LIST_MINIMUM_WIDTH  = 300;
  static const int PROPERTY_PANEL_MINIMUM_WIDTH  = 400;
  static const int CANVAS_MINIMUM_WIDTH  = 480;
  static const int CANVAS_MINIMUM_HEIGHT = 270;
};

#endif /* MAIN_WINDOW_H_ */
