/*
 * MainWindow.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2014 Dame Diongue -- baydamd(@)gmail(.)com
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
#include <QtWidgets>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariant>
#include <QMap>
#include <QMessageLogger>

#include "MM.h"

#include "MapperGLCanvas.h"
#include "MapperGLCanvasToolbar.h"
#include "OscInterface.h"
#ifdef HAVE_MCP
#include "McpServer.h"
#endif

#include "OutputGLWindow.h"
#include "ConsoleWindow.h"

#include "MappingManager.h"
#include "LayerItemDelegate.h"
#include "LayerListModel.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "SourceGui.h"

namespace mmp {

class PreferenceDialog;
class AboutDialog;
class ShortcutWindow;

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
  bool eventFilter(QObject *object, QEvent *event);

  void dragEnterEvent(QDragEnterEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);

  // Slots ////////////////////////////////////////////////////////////////////////////////////////////////////
private slots:

  // Menus slots.
  // File menu.
  void newFile();
  void open();
  bool save();
  bool saveAs();
  void importMedia();
  void openCameraDevice();
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
  void duplicateLayerItem();
  void deleteLayerItem();
  void renameLayerItem();
  void setLayerItemLocked(bool locked);
  void setLayerItemHide(bool hide);
  void setLayerItemSolo(bool solo);
  void loadLayerMedia();
  void transformActionLayerItem();
  void reorderLayerItem();
  // Context menu for sources
  void deleteSourceItem();
  void renameSourceItem();
  void sourceListEditEnd(QWidget* editor);
  // Output menu
  void setupOutputScreen();
  void updateScreenCount();

  // Widget callbacks.
  void handleSourceItemSelectionChanged();
//  void handleItemDoubleClicked(QListWidgetItem* item);
  void handleLayerItemSelectionChanged(const QModelIndex &index);
  void handleLayerItemChanged(const QModelIndex &index);
  void handleLayerIndexesMoved();
  void handleSourceItemSelected(QListWidgetItem* item);
  void handleSourceChanged(Source::ptr source);

  void layerPropertyChanged(uid id, QString propertyName, QVariant value);
  void sourcePropertyChanged(uid id, QString propertyName, QVariant value);


  // Other.
  void windowModified();
  void pollOscInterface();
  void exitFullScreen();

  // Some help links
  void documentation() {
      QDesktopServices::openUrl(QUrl(MM::WEBSITE_URL));
  }
  // Send us feedback
  void sendFeedback() {
      QDesktopServices::openUrl(QUrl("mailto:mapmap@artpluscode.com"));
  }
  // Technical support
  void technicalSupport() {
      QDesktopServices::openUrl(QUrl(MM::WEBSITE_URL));
  }
  // Report an issues
  void reportBug() {
      QDesktopServices::openUrl(QUrl("https://github.com/mapmapteam/mapmap/issues/new"));
  }

  void openShortcutWindow();

  void updateSettings();

  void updateLayerListColumnWidth();

public slots:

  // Layer creation.
  void addMesh();
  void addTriangle();
  void addEllipse();

  // CRUD.

  /// Clears all mappings and sources.
  bool clearProject();

  /// Create or replace a media source (or image).
  uid createMediaSource(uid sourceId, QString uri, float x, float y, bool isImage, VideoType type, double rate=1.0);

  /// Create or replace a color source.
  uid createColorSource(uid sourceId, QColor color);

  // TODO: Remove all these unsed fonctions below

  /*======= Start of Unsed fonctions =======*/
  /// Creates a textured mesh.
  uid createMeshTextureLayer(uid mappingId,
                               uid sourceId,
                               int nColumns, int nRows,
                               const QVector<QPointF> &src, const QVector<QPointF> &dst);

  /// Creates a textured triangle.
  uid createTriangleTextureLayer(uid mappingId,
                                   uid sourceId,
                                   const QVector<QPointF> &src, const QVector<QPointF> &dst);


  /// Creates a textured ellipse.
  uid createEllipseTextureLayer(uid mappingId,
                                  uid sourceId,
                                  const QVector<QPointF> &src, const QVector<QPointF> &dst);

  /// Creates a color quad.
  uid createQuadColorLayer(uid mappingId,
                             uid sourceId,
                             const QVector<QPointF> &dst);

  /// Creates a color triangle.
  uid createTriangleColorLayer(uid mappingId,
                                 uid sourceId,
                                 const QVector<QPointF> &dst);

  /// Creates a color ellipse.
  uid createEllipseColorLayer(uid mappingId,
                                uid sourceId,
                                const QVector<QPointF> &dst);
  /*======= End of Unsed fonctions =======*/

  /// Sets visibility of mapping.
  void setLayerVisible(uid mappingId, bool visible);

  /// Sets solo status of mapping.
  void setLayerSolo(uid mappingId, bool solo);

  /// Sets locked attribute of mapping.
  void setLayerLocked(uid mappingId, bool locked);

  /// Deletes/removes a mapping.
  void deleteLayer(uid mappingId);

  /// Moves a mapping to given index.
  void moveLayer(uid mappingId, int idx);

  /// Clone/duplicate a mapping
  void duplicateLayer(uid mappingId);

  /// Deletes/removes a source and all associated mappigns.
  void deleteSource(uid sourceId, bool replace = false);

  /// Updates all canvases.
  void updateCanvases();

	/// Update all mapping guis.
	void updateMappers();

  /**
   * This function is triggered framesPerSeconds() times per second. It makes sure
   * the image is refreshed (updateCanvases()) and performs other necessary operations.
   */
  void processFrame();

  /**
   * Performs operations related to the playing state, such as making sure to play only sources
   * that are visible.
   */
  void updatePlayingState();

  // Editing toggles.
  void setFramesPerSecond(qreal fps);
  void enableDisplayControls(bool display);
  void enableDisplaySourceControls(bool display);
  void enableStickyVertices(bool display);
  void displayUndoHistory(bool display);

  // Show Mapping Context Menu
  void showLayerContextMenu(const QPoint &point);
  // Show Source Context Menu
  void showSourceContextMenu(const QPoint &point);

  /// Start playback.
  void play(bool updatePlayPauseActions=true);

  /// Pause playback.
  void pause(bool updatePlayPauseActions=true);

  /// Reset playback.
  void rewind();

private:
  // Internal methods. //////////////////////////////////////////////////////////////////////////////////////

  // Creation of view elements.
  void createLayout();
  void createActions();
  void createMenus();
  void createLayerContextMenu();
  void createSourceContextMenu();
  void createToolBars();
  void createStatusBar();
  void updateRecentFileActions();
  void updateRecentVideoActions();
  void updateScreenActions();
  void updateMediaListActions();
  void updateLayerActions();

  // Settings.
  void readSettings();
  void writeSettings();

  // OSC.
  void startOscReceiver();

#ifdef HAVE_MCP
  // MCP server.
  void startMcpServer();
#endif

  // Actions-related.
  bool okToContinue();

public:
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  void setCurrentVideo(const QString &filename);
  bool importMediaFile(const QString &fileName, bool isImage = false, bool isCamera = false);
  bool addColorSource(const QColor& color);
  void addLayerItem(uid mappingId);
  void removeLayerItem(uid mappingId);
  void moveLayerItem(uid mappingId, int steps);
  void addSourceItem(uid sourceId, const QIcon& icon, const QString& name);
  void updateSourceItem(uid sourceId, const QIcon& icon, const QString& name);
  void removeSourceItem(uid sourceId);
  void renameLayer(uid mappingId, const QString& name);
  void renameSource(uid sourceId, const QString& name);
  void clearWindow();
  // Resync mapping manager order the same as the GUI.
  void syncLayerManager();
  // Check if the file exists
  bool fileExists(const QString& file);
  // Check if the file is supported
  bool fileSupported(const QString& file, bool isImage);
  bool fileSupported(const QString &file, const QString &extension);
  // Locate the file not found
  QString locateMediaFile(const QString& uri, bool isImage);

  static MainWindow* window();

  // Returns a short version of filename.
  static QString strippedName(const QString &fullFileName);

  // Returns the source icon depending on play/pause state.
  static const QIcon getSourceIcon(Source::ptr source);

private:
  // Connects/disconnects project-specific widgets (sources and mappings).
  void connectProjectWidgets();
  void disconnectProjectWidgets();

  // Get/set id from list item.
  static uid getItemId(const QListWidgetItem& item);
  static void setItemId(QListWidgetItem& item, uid id);
  static QListWidgetItem* getItemFromId(const QListWidget& list, uid id);
  static int getItemRowFromId(const QListWidget& list, uid id);
  uid currentLayerItemId() const;

  static QIcon createColorIcon(const QColor& color);
  static QIcon createFileIcon(const QString& filename);
  static QIcon createImageIcon(const QString& filename);

  // GUI elements. ////////////////////////////////////////////////////////////////////////////////////////

  // Menu actions.
  QMenu *fileMenu;
  QMenu *editMenu;
  QMenu *toolsMenu;
  QMenu *viewMenu;
  QMenu *windowMenu;
  QMenu *helpMenu;

  // Sub-menus.
  QMenu *outputScreenMenu;
  QMenu *recentFileMenu;
  QMenu *recentVideoMenu;
  QMenu *layerContextMenu;
  QMenu *sourceContextMenu;

  // Some menus when need to be separated
  QMenu *sourceMenu;
  QMenu *destinationMenu;

  QMenu *_changeLayerMediaMenu;
  QAction *_importLayerMediaAction;

  // Toolbar.
  QToolBar *mainToolBar;

  // Actions.
  QAction *separatorAction;
  QAction *newAction;
  QAction *openAction;
  QAction *importMediaAction;
  QAction *AddCameraAction;
  QAction *addColorAction;
  QAction *saveAction;
  QAction *saveAsAction;
  QAction *exitAction;
  QAction *undoAction;
  QAction *redoAction;
  // Mappings context menu actions
  QAction *duplicateLayerAction;
  QAction *deleteLayerAction;
  QAction *renameLayerAction;
  QAction *layerSoloAction;
  QAction *layerLockedAction;
  QAction *layerHideAction;
  // Transform.
  QAction *layerRotate90CWAction;
  QAction *layerRotate90CCWAction;
  QAction *layerRotate180Action;
  QAction *layerHorizontalFlipAction;
  QAction *layerVerticalFlipAction;
  // Layer reordering.
  QAction *layerRaiseAction;
  QAction *layerLowerAction;
  QAction *layerRaiseToTopAction;
  QAction *layerLowerToBottomAction;

  // Sources context menu action
  QAction *deleteSourceAction;
  QAction *renameSourceAction;
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
  QAction *displaySourceControlsAction;
  QAction *displayTestSignalAction;
  QAction *stickyVerticesAction;
  QAction *displayUndoHistoryAction;
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

  // help actions
  QAction *bugReportAction;
  QAction *supportAction;
  QAction *docAction;
  QAction *feedbackAction;
  QAction *shortcutAction;

  // Screen output action
  QList<QAction *> screenActions;
  QActionGroup *screenActionGroup;

  // Canvas zoom actions
  QAction *zoomInAction;
  QAction *zoomOutAction;
  QAction *resetZoomAction;
  QAction *fitToViewAction;

  // Widgets and layout.
  QTabWidget* contentTab;

  QSplitter* sourceSplitter;
  QListWidget* sourceList;
  QStackedWidget* sourcePropertyPanel;

  QSplitter* layerSplitter;
  QTableView* layerList;
  QStackedWidget* layerPropertyPanel;

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
  LayerListModel *layerListModel;
  LayerItemDelegate *layerItemDelegate;

  // OSC.
  OscInterface::ptr osc_interface;
  int oscListeningPort;
  QTimer *osc_timer;

#ifdef HAVE_MCP
  // MCP server.
  QScopedPointer<McpServer> mcp_server;
  int mcpListeningPort;
#endif

  // View.

  // The view counterpart of Mappings.
  QMap<uid, LayerGui::ptr> layerGuis;
  QMap<uid, SourceGui::ptr> sourceGuis;

  // Current selected source/mapping.
  uid currentSourceId;
  uid currentLayerId;
  bool _hasCurrentLayer;
  bool _hasCurrentSource;

  // Number of frames per second.
  qreal _framesPerSecond;

  // True iff the play button is currently pressed.
  bool _isPlaying;

  // True iff we are displaying the controls.
  bool _displayControls;

  // True iff we are displaying the borders of all controls of all shapes related to a source.
  bool _displaySourceControls;

  // True iff we want vertices to stick to each other.
  bool _stickyVertices;

  bool _displayUndoStack;

  // Menu bar hidden state
  bool _showMenuBar;

  // Keeps track of the current selected item, wether it's a source or mapping.
  QListWidgetItem* currentSelectedItem;
  QModelIndex currentSelectedIndex;
  QTimer *videoTimer;
  QElapsedTimer *systemTimer;
  // Preference dialog
  PreferenceDialog* _preferenceDialog;
  // About dialog
  AboutDialog *_aboutDialog;
  // Shortcut Windows
  ShortcutWindow *_shortcutWindow;

  // UndoStack
  QUndoStack *undoStack;

  // Labels for status bar
  QLabel *destinationZoomLabel;
  QLabel *sourceZoomLabel;
  QLabel *lastActionLabel;
  QLabel *currentMessageLabel;
  QLabel *mousePosLabel;
  QLabel *trueFramesPerSecondsLabel;

  typedef Source::SourceType SourceType;
  typedef MShape::ShapeType ShapeType ;

public:
  // Accessor/mutators for the view. ///////////////////////////////////////////////////////////////////
  MappingManager& getMappingManager() const { return *mappingManager; }

  LayerGui::ptr getLayerGuiByLayerId(uint id) const { return layerGuis[id]; }
  SourceGui::ptr getSourceGuiBySourceId(uint id) const { return sourceGuis[id]; }

  uid getCurrentSourceId() const { return currentSourceId; }
  uid getCurrentLayerId() const { return currentLayerId; }

  Layer::ptr getCurrentLayer() const { return mappingManager->getLayerById(currentLayerId); }
  Source::ptr getCurrentSource() const { return mappingManager->getSourceById(currentSourceId); }

  bool hasCurrentSource() const { return _hasCurrentSource; }
  bool hasCurrentLayer() const { return _hasCurrentLayer; }
  void setCurrentSource(int uid);
  void setCurrentLayer(int uid);
  void removeCurrentSource();
  void removeCurrentLayer();

  OutputGLWindow* getOutputWindow() const { return outputWindow; }
  MapperGLCanvas* getSourceCanvas() const { return sourceCanvas; }
  MapperGLCanvas* getDestinationCanvas() const { return destinationCanvas; }
  int getPreferredScreen() const { return outputWindow->getPreferredScreen(); }

  /// Returns the number of frames per second.
  qreal framesPerSecond() const { return _framesPerSecond; }

  /// Returns true iff MapMap is currently playing (ie. not in pause).
  bool isPlaying() const { return _isPlaying; }

  /// Returns true iff we should display the controls.
  bool displayControls() const { return _displayControls; }

  /// Returns true iff we should display all of the shapes related to a source.
  bool displaySourceControls() const { return _displaySourceControls; }

  /// Returns true iff we want vertices to stick to each other.
  bool isStickyVertices() const { return _stickyVertices; }

  // Use the same undoStack for whole program
  QUndoStack* getUndoStack() const { return undoStack; }

  void startFullScreen();
  bool setOscPort(QString portNumber);
  bool setOscPort(int portNumber);
  int getOscPort() const;
  void setVerbose(bool verbose);
#ifdef HAVE_MCP
  bool setMcpPort(QString portNumber);
  bool setMcpPort(int portNumber);
  int getMcpPort() const;
#endif
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

}

#endif /* MAIN_WINDOW_H_ */
