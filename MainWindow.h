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
#if QT_VERSION >= 0x050000
  #include <QtWidgets>
#endif
#include <QTimer>
#include <QVariant>
#include <QMap>
#include <QMessageLogger>

#include "MM.h"

#include "MapperGLCanvas.h"
#include "MapperGLCanvasToolbar.h"
#ifdef HAVE_OSC
#include "OscInterface.h"
#endif

#include "OutputGLWindow.h"
#include "PreferencesDialog.h"
#include "ConsoleWindow.h"

#include "MappingManager.h"
#include "MappingItemDelegate.h"
#include "MappingListModel.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "PaintGui.h"

MM_BEGIN_NAMESPACE

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
  //void applyOscCommand(const QVariantList& command);

protected:
  // Events ///////////////////////////////////////////////////////////////////////////////////////////////////
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

  // Slots ////////////////////////////////////////////////////////////////////////////////////////////////////
private slots:

  // Menus slots.
  // File menu.
  void newFile();
  void open();
  void preferences();
  bool save();
  bool saveAs();
  void importMedia();
  void addColor();
  void about();
  void updateStatusBar();
  void showMenuBar(bool shown);
  void openRecentFile();
  void clearRecentFileList();
  void openRecentVideo();
  void quitMapMap();
  // Edit menu.
  void deleteItem();
  // Context menu for mappings.
  void duplicateMappingItem();
  void deleteMappingItem();
  void renameMappingItem();
  void setMappingItemLocked(bool locked);
  void setMappingItemHide(bool hide);
  void setMappingItemSolo(bool solo);
  // Context menu for paints
  void deletePaintItem();
  void renamePaintItem();
  void paintListEditEnd(QWidget* editor);

  // Widget callbacks.
  void handlePaintItemSelectionChanged();
//  void handleItemDoubleClicked(QListWidgetItem* item);
  void handleMappingItemSelectionChanged(const QModelIndex &index);
  void handleMappingItemChanged(const QModelIndex &index);
  void handleMappingIndexesMoved();
  void handlePaintItemSelected(QListWidgetItem* item);
  void handlePaintChanged(Paint::ptr paint);

  void mappingPropertyChanged(uid id, QString propertyName, QVariant value);
  void paintPropertyChanged(uid id, QString propertyName, QVariant value);

  void addMesh();
  void addTriangle();
  void addEllipse();

  // Other.
  void windowModified();
  void pollOscInterface();
  void exitFullScreen();

public slots:

  // CRUD.

  /// Clears all mappings and paints.
  bool clearProject();

  /// Create or replace a media paint (or image).
  uid createMediaPaint(uid paintId, QString uri, float x, float y, bool isImage, bool live=false, double rate=1.0);

  /// Create or replace a color paint.
  uid createColorPaint(uid paintId, QColor color);

  // TODO: Remove all these unsed fonctions below

  /*======= Start of Unsed fonctions =======*/
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

  /// Creates a color ellipse.
  uid createEllipseColorMapping(uid mappingId,
                                uid paintId,
                                const QVector<QPointF> &dst);
  /*======= End of Unsed fonctions =======*/

  /// Sets visibility of mapping.
  void setMappingVisible(uid mappingId, bool visible);

  /// Sets solo status of mapping.
  void setMappingSolo(uid mappingId, bool solo);

  /// Sets locked attribute of mapping.
  void setMappingLocked(uid mappingId, bool locked);

  /// Deletes/removes a mapping.
  void deleteMapping(uid mappingId);

  /// Clone/duplicate a mapping
  void duplicateMapping(uid mappingId);

  /// Deletes/removes a paint and all associated mappigns.
  void deletePaint(uid paintId, bool replace = false);

  /// Updates all canvases.
  void updateCanvases();

  // Editing toggles.
  void enableDisplayControls(bool display);
  void enableStickyVertices(bool display);
  void enableTestSignal(bool enable);
  void displayUndoStack(bool display);

  // Show Mapping Context Menu
  void showMappingContextMenu(const QPoint &point);
  // Show Paint Context Menu
  void showPaintContextMenu(const QPoint &point);

  /// Start playback.
  void play();

  /// Pause playback.
  void pause();

  /// Reset playback.
  void rewind();

public:
  bool setTextureUri(int texture_id, const std::string &uri);
  bool setTextureRate(int texture_id, double rate);
  bool setTextureVolume(int texture_id, double volume);
  void setTexturePlayState(int texture_id, bool played);

private:
  // Internal methods. //////////////////////////////////////////////////////////////////////////////////////

  // Creation of view elements.
  void createLayout();
  void createActions();
  void createMenus();
  void createMappingContextMenu();
  void createPaintContextMenu();
  void createToolBars();
  void createStatusBar();
  void updateRecentFileActions();
  void updateRecentVideoActions();

  // Settings.
  void readSettings();
  void writeSettings();

  // OSC.
  void startOscReceiver();

  // Actions-related.
  bool okToContinue();

public:
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  void setCurrentVideo(const QString &filename);
  bool importMediaFile(const QString &fileName, bool isImage);
  bool addColorPaint(const QColor& color);
  void addMappingItem(uid mappingId);
  void removeMappingItem(uid mappingId);
  void addPaintItem(uid paintId, const QIcon& icon, const QString& name);
  void updatePaintItem(uid paintId, const QIcon& icon, const QString& name);
  void removePaintItem(uid paintId);
  void renameMapping(uid mappingId, const QString& name);
  void renamePaint(uid paintId, const QString& name);
  void clearWindow();
  // Check if the file exists
  bool fileExists(const QString& file);
  // Check if the file is supported
  bool fileSupported(const QString& file, bool isImage);
  // Locate the file not found
  QString locateMediaFile(const QString& uri, bool isImage);

  static MainWindow* instance();

  // Returns a short version of filename.
  static QString strippedName(const QString &fullFileName);

private:
  // Connects/disconnects project-specific widgets (paints and mappings).
  void connectProjectWidgets();
  void disconnectProjectWidgets();

  // Get/set id from list item.
  static uid getItemId(const QListWidgetItem& item);
  static void setItemId(QListWidgetItem& item, uid id);
  static QListWidgetItem* getItemFromId(const QListWidget& list, uid id);
  static int getItemRowFromId(const QListWidget& list, uid id);
  uid currentMappingItemId() const;

  static QIcon createColorIcon(const QColor& color);
  static QIcon createFileIcon(const QString& filename);
  static QIcon createImageIcon(const QString& filename);

  // GUI elements. ////////////////////////////////////////////////////////////////////////////////////////

  // Menu actions.
  QMenu *fileMenu;
  QMenu *editMenu;
  QMenu *viewMenu;
  QMenu *toolsMenu;
  QMenu *windowMenu;
  QMenu *playbackMenu;
  QMenu *helpMenu;
  QMenu *recentFileMenu;
  QMenu *recentVideoMenu;
  QMenu *mappingContextMenu;
  QMenu *paintContextMenu;
  // Some menus when need to be separated
  QMenu *sourceMenu;
  QMenu *destinationMenu;
  QMenu *toolBarsMenu;

  // Toolbar.
  QToolBar *mainToolBar;

  // Actions.
  QAction *separatorAction;
  QAction *newAction;
  QAction *openAction;
  QAction *importMediaAction;
  QAction *addColorAction;
  QAction *saveAction;
  QAction *saveAsAction;
  QAction *exitAction;
  QAction *undoAction;
  QAction *redoAction;
  // Mappings context menu actions
  QAction *cloneMappingAction;
  QAction *deleteMappingAction;
  QAction *renameMappingAction;
  QAction *mappingSoloAction;
  QAction *mappingLockedAction;
  QAction *mappingHideAction;
  // Paints context menu action
  QAction *deletePaintAction;
  QAction *renamePaintAction;
  QAction *preferencesAction;
  QAction *aboutAction;
  QAction *clearRecentFileActions;
  QAction *emptyRecentVideos;

  QAction *addMeshAction;
  QAction *addTriangleAction;
  QAction *addEllipseAction;

  QAction *playAction;
  QAction *pauseAction;
  QAction *rewindAction;

  QAction *outputFullScreenAction;
  QAction *displayControlsAction;
  QAction *displayTestSignalAction;
  QAction *stickyVerticesAction;
  QAction *displayUndoStackAction;
  QAction *displayZoomToolAction;
  QAction *openConsoleAction;
  QAction *showMenuBarAction;
  QAction *showToolBarAction;

  QActionGroup *perspectiveActionGroup;
  QAction *mainViewAction;
  QAction *sourceViewAction;
  QAction *destViewAction;

  enum { MaxRecentFiles = 10 };
  enum { MaxRecentVideo = 5 };
  QAction *recentFileActions[MaxRecentFiles];
  QAction *recentVideoActions[MaxRecentVideo];

  // Widgets and layout.
  QTabWidget* contentTab;

  QSplitter* paintSplitter;
  QListWidget* paintList;
  QStackedWidget* paintPropertyPanel;

  QSplitter* mappingSplitter;
  QTableView* mappingList;
  QStackedWidget* mappingPropertyPanel;

  QUndoView* undoView;

  MapperGLCanvas* sourceCanvas;
  MapperGLCanvasToolbar* sourceCanvasToolbar;
  QWidget* sourcePanel;
  MapperGLCanvas* destinationCanvas;
  MapperGLCanvasToolbar* destinationCanvasToolbar;
  QWidget* destinationPanel;

  OutputGLWindow* outputWindow;
  ConsoleWindow* consoleWindow;

  QSplitter* mainSplitter;
  QSplitter* canvasSplitter;

  // Internal variables. ///////////////////////////////////////////////////////////////////////////////////

  // Recent files
  QStringList recentFiles;
  QStringList recentVideos;

  // Current filename.
  QString curFile;

  // Current video name
  QString curVideo;

  // Settings
  QSettings settings;

  // Model.
  MappingManager* mappingManager;
  MappingListModel *mappingListModel;
  MappingItemDelegate *mappingItemDelegate;

  // OSC.
#ifdef HAVE_OSC
  OscInterface::ptr osc_interface;
#endif
  int config_osc_receive_port;
  QTimer *osc_timer;

  // View.

  // The view counterpart of Mappings.
  QMap<uid, MappingGui::ptr> mappers;
  QMap<uid, PaintGui::ptr> paintGuis;

  // Current selected paint/mapping.
  uid currentPaintId;
  uid currentMappingId;
  bool _hasCurrentMapping;
  bool _hasCurrentPaint;

  // True iff the play button is currently pressed.
  bool _isPlaying;

  // True iff we are displaying the controls.
  bool _displayControls;

  // True iff we want vertices to stick to each other.
  bool _stickyVertices;

  bool _displayUndoStack;

  // Menu bar hidden state
  bool _showMenuBar;

  // Keeps track of the current selected item, wether it's a paint or mapping.
  QListWidgetItem* currentSelectedItem;
  QModelIndex currentSelectedIndex;
  QTimer *videoTimer;

  PreferencesDialog* _preferences_dialog;

  // UndoStack
  QUndoStack *undoStack;

  // Labels for status bar
  QLabel *destinationZoomLabel;
  QLabel *sourceZoomLabel;
  QLabel *undoLabel;
  QLabel *currentMessageLabel;
  QLabel *mousePosLabel;


public:
  // Accessor/mutators for the view. ///////////////////////////////////////////////////////////////////
  MappingManager& getMappingManager() const { return *mappingManager; }

  MappingGui::ptr getMappingGuiByMappingId(uint id) const { return mappers[id]; }
  PaintGui::ptr getPaintGuiByPaintId(uint id) const { return paintGuis[id]; }

  uid getCurrentPaintId() const { return currentPaintId; }
  uid getCurrentMappingId() const { return currentMappingId; }

  Mapping::ptr getCurrentMapping() const { return mappingManager->getMappingById(currentMappingId); }
  Paint::ptr getCurrentPaint() const { return mappingManager->getPaintById(currentPaintId); }

  bool hasCurrentPaint() const { return _hasCurrentPaint; }
  bool hasCurrentMapping() const { return _hasCurrentMapping; }
  void setCurrentPaint(int uid);
  void setCurrentMapping(int uid);
  void removeCurrentPaint();
  void removeCurrentMapping();

  OutputGLWindow* getOutputWindow() const { return outputWindow; }
  MapperGLCanvas* getSourceCanvas() const { return sourceCanvas; }
  MapperGLCanvas* getDestinationCanvas() const { return destinationCanvas; }

  /// Returns true iff we should display the controls.
  bool displayControls() const { return _displayControls; }

  /// Returns true iff we want vertices to stick to each other.
  bool stickyVertices() const { return _stickyVertices; }

  // Use the same undoStack for whole program
  QUndoStack* getUndoStack() const { return undoStack; }

  void startFullScreen();
  bool setOscPort(QString portNumber);
  bool setOscPort(int portNumber);
  int getOscPort() const;
  void setOutputWindowFullScreen(bool enable);

public:
  // Constants. ///////////////////////////////////////////////////////////////////////////////////////
  static const int DEFAULT_WIDTH = 1360;
  static const int DEFAULT_HEIGHT = 768;
  static const int PAINT_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int PAINT_LIST_MINIMUM_HEIGHT = 290;
  static const int MAPPING_LIST_MINIMUM_HEIGHT = 290;
  static const int PAINT_PROPERTY_PANEL_MINIMUM_HEIGHT = 290;
  static const int MAPPING_PROPERTY_PANEL_MINIMUM_HEIGHT = 290;
  static const int CANVAS_MINIMUM_WIDTH  = 480;
  static const int CANVAS_MINIMUM_HEIGHT = 270;
  static const int OUTPUT_WINDOW_MINIMUM_WIDTH = 480;
  static const int OUTPUT_WINDOW_MINIMUM_HEIGHT = 270;
};

MM_END_NAMESPACE

#endif /* MAIN_WINDOW_H_ */
