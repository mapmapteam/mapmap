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
#include <QMap>
#include "SourceGLCanvas.h"
#ifdef HAVE_OSC
#include "OscInterface.h"
#endif
#include "DestinationGLCanvas.h"

#include "OutputGLWindow.h"

#include "MappingManager.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#define MAPMAP_VERSION "0.0.9"

/**
 * This is the main window of MapMap. It acts as both a view and a controller interface.
 */
class MainWindow: public QMainWindow
{
Q_OBJECT

public:
  // Constructor.
  MainWindow();

  // Destructor.
  ~MainWindow();

  // XXX Unused.
  void applyOscCommand(const QVariantList& command);

protected:
  // Events ///////////////////////////////////////////////////////////////////////////////////////////////////
  void closeEvent(QCloseEvent *event);

  // Slots ////////////////////////////////////////////////////////////////////////////////////////////////////
private slots:

  // Menus slots.
  // File menu.
  void newFile();
  void open();
  bool save();
  bool saveAs();
  void import();
  void addColor();
  void about();
  void updateStatusBar();
  // Edit menu.
  void deleteItem();

  // Widget callbacks.
  void handlePaintItemSelectionChanged();
  void handleMappingItemSelectionChanged();
  void handleMappingItemChanged(QListWidgetItem* item);
  void handleMappingIndexesMoved();
  void handleItemSelected(QListWidgetItem* item);
  void addMesh();
  void addTriangle();
  void addEllipse();

  // Other.
  void windowModified();
  void pollOscInterface();

public slots:

  // CRUD.

  /// Clears all mappings and paints.
  bool clearProject();

  /// Create an image paint.
  uid createImagePaint(uid paintId, QString uri, float x, float y);

  /// Create a color paint.
  uid createColorPaint(uid paintId, QColor color);

  /// Creates a textured mesh.
  uid createMeshTextureMapping(uid mappingId,
                               uid paintId,
                               int nColumns, int nRows,
                               const QVector<QPointF> &src, const QVector<QPointF> &dst);

  /// Creates a textured triangle.
  uid createTriangleTextureMapping(uid mappingId,
                                   uid paintId,
                                   const QVector<QPointF> &src, const QVector<QPointF> &dst);


  /// Creates a textured ellipse.
  uid createEllipseTextureMapping(uid mappingId,
                                  uid paintId,
                                  const QVector<QPointF> &src, const QVector<QPointF> &dst);

  /// Creates a color quad.
  uid createQuadColorMapping(uid mappingId,
                             uid paintId,
                             const QVector<QPointF> &dst);

  /// Creates a color triangle.
  uid createTriangleColorMapping(uid mappingId,
                                 uid paintId,
                                 const QVector<QPointF> &dst);

  uid createEllipseColorMapping(uid mappingId,
                                uid paintId,
                                const QVector<QPointF> &dst);

  /// Deletes/removes a mapping.
  void deleteMapping(uid mappingId);

  /// Deletes/removes a paint and all associated mappigns.
  void deletePaint(uid paintId);

  /// Updates all canvases.
  void updateCanvases();

private:
  // Internal methods. //////////////////////////////////////////////////////////////////////////////////////

  // Creation of view elements.
  void createLayout();
  void createActions();
  void createMenus();
  void createContextMenu();
  void createToolBars();
  void createStatusBar();

  // Settings.
  void readSettings();
  void writeSettings();

  // OSC.
  void startOscReceiver();

  // Actions-related.
  bool okToContinue();
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  bool importMediaFile(const QString &fileName);
  bool addColorPaint(const QColor& color);
  void addMappingItem(uid mappingId);
  void removeMappingItem(uid mappingId);
  void addPaintItem(uid paintId, const QIcon& icon, const QString& name);
  void removePaintItem(uid paintId);
  void clearWindow();

  // Returns a short version of filename.
  static QString strippedName(const QString &fullFileName);

  // Connects/disconnects project-specific widgets (paints and mappings).
  void connectProjectWidgets();
  void disconnectProjectWidgets();

  // Get/set id from list item.
  static uid getItemId(const QListWidgetItem& item);
  static void setItemId(QListWidgetItem& item, uid id);
  static int getItemRowFromId(const QListWidget& list, uid id);

  // GUI elements. ////////////////////////////////////////////////////////////////////////////////////////

  // Menu actions.
  QMenu *fileMenu;
//  QMenu *editMenu;
//  QMenu *selectSubMenu;
//  QMenu *toolsMenu;
//  QMenu *optionsMenu;
  QMenu *viewMenu;
  QMenu *editMenu;
  QMenu *helpMenu;

  // Toolbar.
  QToolBar *fileToolBar;

  // Actions.
  QAction *separatorAction;
  QAction *newAction;
  QAction *openAction;
  QAction *importAction;
  QAction *addColorAction;
  QAction *saveAction;
  QAction *saveAsAction;
  QAction *exitAction;
//  QAction *cutAction;
//  QAction *copyAction;
//  QAction *pasteAction;
  QAction *deleteAction;
  QAction *aboutAction;

  QAction *addMeshAction;
  QAction *addTriangleAction;
  QAction *addEllipseAction;

  QAction *displayOutputWindow;

  // Widgets and layout.

  QListWidget* paintList;
  QListWidget* mappingList;
  QStackedWidget* propertyPanel;

  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;
  OutputGLWindow* outputWindow;

  QSplitter* mainSplitter;
  QSplitter* resourceSplitter;
  QSplitter* canvasSplitter;

  // Internal variables. ///////////////////////////////////////////////////////////////////////////////////

  // Current filename.
  QString curFile;

  // Model.
  MappingManager* mappingManager;

  // OSC.
#ifdef HAVE_OSC
  OscInterface::ptr osc_interface;
#endif
  int config_osc_receive_port;
  QTimer *osc_timer;

  // View.

  // The view counterpart of Mappings.
  QMap<uint, Mapper::ptr> mappers;

  // Current selected paint/mapping.
  uid currentPaintId;
  uid currentMappingId;
  bool _hasCurrentMapping;
  bool _hasCurrentPaint;

  // Keeps track of the current selected item, wether it's a paint or mapping.
  QListWidgetItem* currentSelectedItem;

public:
  // Accessor/mutators for the view. ///////////////////////////////////////////////////////////////////
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
    currentPaintId = NULL_UID;
  }
  void removeCurrentMapping() {
    _hasCurrentMapping = false;
    currentMappingId = NULL_UID;
  }

public:
  // Constants. ///////////////////////////////////////////////////////////////////////////////////////
  static const int DEFAULT_WIDTH = 1600;
  static const int DEFAULT_HEIGHT = 800;
  static const int PAINT_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int PAINT_LIST_MINIMUM_WIDTH = 100;
  static const int MAPPING_LIST_MINIMUM_WIDTH  = 300;
  static const int PROPERTY_PANEL_MINIMUM_WIDTH  = 400;
  static const int CANVAS_MINIMUM_WIDTH  = 480;
  static const int CANVAS_MINIMUM_HEIGHT = 270;
  static const int OUTPUT_WINDOW_MINIMUM_WIDTH = 480;
  static const int OUTPUT_WINDOW_MINIMUM_HEIGHT = 270;
};

#endif /* MAIN_WINDOW_H_ */
