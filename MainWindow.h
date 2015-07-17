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

#include "MM.h"

#include "SourceGLCanvas.h"
#ifdef HAVE_OSC
#include "OscInterface.h"
#endif
#include "DestinationGLCanvas.h"

#include "OutputGLWindow.h"
#include "PreferencesDialog.h"

#include "MappingManager.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "PaintGui.h"

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
  bool eventFilter(QObject *obj, QEvent *event);

  // Slots ////////////////////////////////////////////////////////////////////////////////////////////////////
private slots:

  // Menus slots.
  // File menu.
  void newFile();
  void open();
  void preferences();
  bool save();
  bool saveAs();
  void importVideo();
  void importImage();
  void addColor();
  void about();
  void updateStatusBar();
  void openRecentFile();
  void clearRecentFileList();
  void openRecentVideo();
  // Edit menu.
  void deleteItem();
  void cloneItem();
  void renameItem();

  // Widget callbacks.
  void handlePaintItemSelectionChanged();
//  void handleItemDoubleClicked(QListWidgetItem* item);
  void handleMappingItemSelectionChanged();
  void handleMappingItemChanged(QListWidgetItem* item);
  void handleMappingIndexesMoved();
  void handleItemSelected(QListWidgetItem* item);
  void handlePaintChanged(Paint::ptr paint);

  void addMesh();
  void addTriangle();
  void addEllipse();

  void play();
  void pause();
  void rewind();

  // Other.
  void windowModified();
  void pollOscInterface();

public slots:

  // CRUD.

  /// Clears all mappings and paints.
  bool clearProject();

  /// Create or replace a media paint (or image).
  uid createMediaPaint(uid paintId, QString uri, float x, float y, bool isImage, bool live=false, double rate=100.0);

  /// Create or replace a color paint.
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

  /// Sets visibility of mapping.
  void setMappingVisible(uid mappingId, bool visible);

  /// Sets solo status of mapping.
  void setMappingSolo(uid mappingId, bool solo);

  /// Sets locked attribute of mapping.
  void setMappingLocked(uid mappingId, bool locked);

  /// Deletes/removes a mapping.
  void deleteMapping(uid mappingId);

  /// Clone/duplicate a mapping
  void cloneMappingItem(uid mappingId);

  /// Deletes/removes a paint and all associated mappigns.
  void deletePaint(uid paintId, bool replace);

  /// Updates all canvases.
  void updateCanvases();

  // Editing toggles.
  void enableDisplayControls(bool display);
  void enableStickyVertices(bool display);
  void enableTestSignal(bool enable);

  // Show Context Menu
  void showMappingContextMenu(const QPoint &point);

public:
  bool setTextureUri(int texture_id, const std::string &uri);
  bool setTextureRate(int texture_id, double rate);

private:
  // Internal methods. //////////////////////////////////////////////////////////////////////////////////////

  // Creation of view elements.
  void createLayout();
  void createActions();
  void createMenus();
  void createContextMenu();
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
  void clearWindow();

  static MainWindow* instance();

  // Returns a short version of filename.
  static QString strippedName(const QString &fullFileName);
  void setMappingItemVisibility(uid mappingId, bool visible);

private:
  // Connects/disconnects project-specific widgets (paints and mappings).
  void connectProjectWidgets();
  void disconnectProjectWidgets();

  // Get/set id from list item.
  static uid getItemId(const QListWidgetItem& item);
  static void setItemId(QListWidgetItem& item, uid id);
  static QListWidgetItem* getItemFromId(const QListWidget& list, uid id);
  static int getItemRowFromId(const QListWidget& list, uid id);
  static QIcon createColorIcon(const QColor& color);
  static QIcon createFileIcon(const QString& filename);
  static QIcon createImageIcon(const QString& filename);

  // GUI elements. ////////////////////////////////////////////////////////////////////////////////////////

  // Menu actions.
  QMenu *fileMenu;
  QMenu *editMenu;
  QMenu *viewMenu;
  QMenu *runMenu;
  QMenu *helpMenu;
  QMenu *recentFileMenu;
  QMenu *recentVideoMenu;
  QMenu *contextMenu;

  // Toolbar.
  QToolBar *mainToolBar;
  QToolBar *runToolBar;

  // Actions.
  QAction *separatorAction;
  QAction *newAction;
  QAction *openAction;
  QAction *importVideoAction;
  QAction *importImageAction;
  QAction *addColorAction;
  QAction *saveAction;
  QAction *saveAsAction;
  QAction *exitAction;
  QAction *undoAction;
  QAction *redoAction;
  QAction *cloneAction;
  QAction *deleteAction;
  QAction *renameAction;
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

  QAction *displayOutputWindowAction;
  //QAction *outputWindowHasCursor;
  QAction *outputWindowFullScreenAction;
  QAction *displayControlsAction;
  QAction *displayTestSignalAction;
  QAction *stickyVerticesAction;

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
  QListWidget* mappingList;
  QStackedWidget* mappingPropertyPanel;

  SourceGLCanvas* sourceCanvas;
  DestinationGLCanvas* destinationCanvas;
  OutputGLWindow* outputWindow;

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

  // OSC.
#ifdef HAVE_OSC
  OscInterface::ptr osc_interface;
#endif
  int config_osc_receive_port;
  QTimer *osc_timer;

  // View.

  // The view counterpart of Mappings.
  QMap<uid, Mapper::ptr> mappers;
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

  // True iff we are displaying the test signal (grid)
  bool _displayTestSignal;

  // True iff we want vertices to stick to each other.
  bool _stickyVertices;

  // Keeps track of the current selected item, wether it's a paint or mapping.
  QListWidgetItem* currentSelectedItem;
  QTimer *videoTimer;

  PreferencesDialog* _preferences_dialog;

  // UndoStack
  QUndoStack *undoStack;



public:
  // Accessor/mutators for the view. ///////////////////////////////////////////////////////////////////
  MappingManager& getMappingManager() const { return *mappingManager; }
  Mapper::ptr getMapperByMappingId(uint id) const { return mappers[id]; }
  uid getCurrentPaintId() const { return currentPaintId; }
  uid getCurrentMappingId() const { return currentMappingId; }
  OutputGLWindow* getOutputWindow() const { return outputWindow; }
  bool hasCurrentPaint() const { return _hasCurrentPaint; }
  bool hasCurrentMapping() const { return _hasCurrentMapping; }
  void setCurrentPaint(int uid);
  void setCurrentMapping(int uid);
  void removeCurrentPaint();
  void removeCurrentMapping();

  MapperGLCanvas* getSourceCanvas() const { return sourceCanvas; }
  MapperGLCanvas* getDestinationCanvas() const { return destinationCanvas; }

  /// Returns true iff we should display the controls.
  bool displayControls() const { return _displayControls; }

  /// Returns true iff we should display the test signal
  bool displayTestSignal() const { return _displayTestSignal; }

  /// Returns true iff we want vertices to stick to each other.
  bool stickyVertices() const { return _stickyVertices; }

  // Use the same undoStack for whole program
  QUndoStack* getUndoStack() const { return undoStack; }

  void startFullScreen();
  bool setOscPort(QString portNumber);
  bool setOscPort(int portNumber);
  int getOscPort() const;
  void setOutputWindowFullScreen(bool enable);
  void quitMapMap();
public:
  // Constants. ///////////////////////////////////////////////////////////////////////////////////////
  static const int DEFAULT_WIDTH = 1600;
  static const int DEFAULT_HEIGHT = 800;
  static const int PAINT_LIST_ITEM_HEIGHT = 40;
  static const int SHAPE_LIST_ITEM_HEIGHT = 40;
  static const int PAINT_LIST_MINIMUM_HEIGHT = 320;
  static const int MAPPING_LIST_MINIMUM_HEIGHT = 320;
  static const int PAINT_PROPERTY_PANEL_MINIMUM_HEIGHT = 320;
  static const int MAPPING_PROPERTY_PANEL_MINIMUM_HEIGHT = 320;
  static const int CANVAS_MINIMUM_WIDTH  = 480;
  static const int CANVAS_MINIMUM_HEIGHT = 270;
  static const int OUTPUT_WINDOW_MINIMUM_WIDTH = 480;
  static const int OUTPUT_WINDOW_MINIMUM_HEIGHT = 270;
};

#endif /* MAIN_WINDOW_H_ */
