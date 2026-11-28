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
#include "Generators.h"
#include "LayerItemDelegate.h"
#include "LayerListModel.h"

#include "SourceGui.h"
#include "HandleModeKeys.h"

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
  void addSyphon();
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

  // Fits a Syphon source's input shapes to its real resolution once known.
  void autoFitSyphonInputShapes(int sourceId, int width, int height);

  // Fits a Video/Camera source's input shapes to its real resolution once known.
  void autoFitVideoInputShapes(int sourceId, int width, int height);

  // Fits the input shapes of a generator to the new size of its image.
  void autoFitGeneratorInputShapes(int sourceId, int oldWidth, int oldHeight, int newWidth, int newHeight);

  // Called with the path of a setting that changed (the property name for a top-level setting).
  void layerPropertyChanged(uid id, QString propertyName, QVariant value);
  void sourcePropertyChanged(uid id, QString propertyName, QVariant value);

  // Shows the new icon of a source in the list of sources.
  void sourceIconChanged(uid id);


  // Other.
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
  // Professional services and custom development by Art Plus Code, the company
  // that sponsors MapMap (video mapping installations, custom features,
  // integration, training and paid support).
  void professionalServices() {
      QDesktopServices::openUrl(QUrl(MM::SERVICES_URL));
  }
  // Support the project with a donation (consolidated on Open Collective).
  void donate() {
      QDesktopServices::openUrl(QUrl(MM::DONATE_URL));
  }
  // Report an issues
  void reportBug() {
      QDesktopServices::openUrl(QUrl("https://github.com/mapmapteam/mapmap/issues/new"));
  }

  void openShortcutWindow();

  /// Shows the first-run / onboarding welcome dialog.
  void showWelcomeDialog();

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

  /// Marks the project as modified (eg. by an undoable command).
  void windowModified();

  /// Asks the user for the output resolution of the project.
  void showOutputResolutionDialog();

  /**
   * Sets the output resolution of the project (see
   * MappingManager::getOutputResolution()) and updates the views. An invalid
   * size gives the default resolution.
   */
  void setOutputResolution(const QSize& resolution);

  /**
   * Returns the output resolution of new projects, set in the preferences:
   * a fixed size, or the resolution of the output screen.
   */
  QSize defaultOutputResolution() const;

  /**
   * Returns the resolution, in physical pixels, of the screen chosen for the
   * output window, or an invalid size if there is none.
   */
  QSize outputScreenResolution() const;

  /// Makes the input editor frame the image of the current source.
  void updateSourceCanvasContent();

  /// Returns the output frame: the rectangle shown by the output window.
  QRectF getOutputFrame() const;

  /**
   * Returns where a new shape goes in the output frame: in its middle, at a
   * fraction of its size, with the proportions of an image of the given size
   * (any proportions for an empty size: color sources).
   */
  QRectF newShapeOutputRect(const QSizeF& contentSize) const;

  /**
   * Moves the shapes of a source placed while its frame size was unknown
   * (defaultSize was assumed) to where they belong with the real frame size.
   * Shapes the user has moved since are left alone.
   */
  void fitDefaultShapesToFrameSize(Source::ptr source, const QSizeF& defaultSize, const QSizeF& frameSize);

  /// Create or replace a media source (or image).
  uid createMediaSource(uid sourceId, QString uri, float x, float y, bool isImage, VideoType type, double rate=1.0);

  /// Create or replace a color source.
  uid createColorSource(uid sourceId, QColor color);

  /// Create a Syphon source pointing at the given server (macOS only).
  uid createSyphonSource(uid sourceId, const QString& serverUUID,
                         const QString& serverName, const QString& appName);

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
   * The canvases are left alone when nothing can have changed (see canvasesNeedRedraw()).
   */
  void processFrame();

  /// Arms the frame timer for the next frame's deadline on the frame clock.
  void scheduleNextFrame();
  /// Restarts the frame clock from now (eg. after a frame rate change) and
  /// schedules the next frame.
  void restartFrameClock();

  /**
   * Tells the window that something which may change what is displayed just
   * happened (user input, OSC or MCP command): the canvases are then redrawn
   * at every tick for MM::REDRAW_GRACE_PERIOD_AFTER_ACTIVITY milliseconds.
   */
  void notifyActivity();

  /**
   * Returns true if the canvases have to be redrawn at this tick: there was
   * recent activity (see notifyActivity()), or a source shown by a visible
   * layer has a new image (Source::needsRedraw()).
   */
  bool canvasesNeedRedraw() const;

  /**
   * Performs operations related to the playing state, such as making sure to play only sources
   * that are visible.
   */
  void updatePlayingState();

  // Editing toggles.
  /// Sets the frame rate, clamped between MM::MIN_FRAMES_PER_SECOND and
  /// MM::MAX_FRAMES_PER_SECOND. Not saved: see the preferences for that.
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

  // Returns the screen chosen for the output window, or null.
  QScreen* _outputScreen() const;

  /// Shows the Media tab of a layer the user just made.
  void showNewLayerMedia(uid layerId);

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

  /// Adds a generator to the project, as a command that can be undone; takes ownership.
  uid addGenerator(Generator* generator);
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

  /// The keys that restrict the handle of the current shape to rotating or scaling.
  HandleModeKeys* handleModeKeys() const { return _handleModeKeys; }

private:
  // Follows R and S for the handles of the shapes (owned by the window).
  HandleModeKeys* _handleModeKeys = nullptr;

  // The singleton, published as soon as construction starts so that anything
  // triggered from inside the constructor (e.g. restoring the output window in
  // fullscreen repaints OutputGLCanvas, which asks for the main window) gets
  // this instance instead of constructing a second one recursively.
  static MainWindow* _instance;

public:

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
  QMenu *addGeneratorMenu;
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
  QAction *addCheckerAction;
  QAction *addGridAction;
  QAction *addColorWheelAction;
  QAction *addSyphonAction;
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
  QAction *publishSyphonOutputAction;
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
  QAction *servicesAction;
  QAction *donateAction;
  QAction *docAction;
  QAction *feedbackAction;
  QAction *shortcutAction;
  QAction *welcomeAction;

  // Screen output action
  QList<QAction *> screenActions;
  QActionGroup *screenActionGroup;

  // Canvas zoom actions
  QAction *zoomInAction;
  QAction *zoomOutAction;
  QAction *resetZoomAction;
  QAction *fitToViewAction;
  QAction *outputResolutionAction;

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

  OutputGLWindow* outputWindow = nullptr;
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
  // When false (default), OSC binds to loopback only; true widens it to all
  // network interfaces. OSC is unauthenticated, so network access is opt-in.
  bool oscAcceptNetwork;
  // When false (default), the OSC /mapmap/quit command is ignored so a remote
  // sender cannot close the application mid-show.
  bool oscAllowQuit;
  QTimer *osc_timer = nullptr;

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

  // True iff the input (source) view is the last view the user worked in.
  // Flip and rotate commands then turn the image inside the layer instead of
  // moving the shape in the output.
  bool _transformInInputView;

  // Canvas that flip and rotate commands act on (see _transformInInputView).
  MapperGLCanvas* transformTargetCanvas() const;

  // Keeps _transformInInputView in sync with keyboard focus.
  void updateTransformTarget(QWidget* now);

  // Menu bar hidden state
  bool _showMenuBar;

  // Keeps track of the current selected item, wether it's a source or mapping.
  QListWidgetItem* currentSelectedItem;
  QModelIndex currentSelectedIndex;
  QTimer *videoTimer;
  // Time since the frame schedule started, and frames scheduled since then.
  QElapsedTimer _frameClock;
  qint64 _frameCount;
  QElapsedTimer *systemTimer;
  // Time elapsed since the last call to notifyActivity().
  QElapsedTimer _activityTimer;
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
  /// Shows the position of the mouse in the output, when it is over it.
  void updateMouseCoordinates();
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
  // Stores whether OSC accepts datagrams from the network. Takes effect the
  // next time startOscReceiver() runs (setOscPort triggers it on Apply).
  void setOscAcceptNetwork(bool accept) { oscAcceptNetwork = accept; }
  // Whether the OSC /mapmap/quit command may close the application.
  bool isOscQuitAllowed() const { return oscAllowQuit; }
  void setOscAllowQuit(bool allow) { oscAllowQuit = allow; }
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
  // Size of the area a new shape is placed in, relative to the output frame.
  static constexpr qreal NEW_SHAPE_OUTPUT_SCALE = 0.5;

  // How far (in pixels) a shape may be from where it was placed and still
  // count as untouched (see fitDefaultShapesToFrameSize()).
  static constexpr qreal UNTOUCHED_SHAPE_TOLERANCE = 1.0;

  /// How long a message of the status bar stays, in milliseconds.
  static constexpr int STATUS_MESSAGE_MS = 2000;

  static const int OUTPUT_WINDOW_MINIMUM_WIDTH = 480;
  static const int OUTPUT_WINDOW_MINIMUM_HEIGHT = 270;
};

}

#endif /* MAIN_WINDOW_H_ */
