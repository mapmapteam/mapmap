/*
 * MainWindow.cpp
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

#include "MainWindow.h"
#include "PreferenceDialog.h"
#include "AboutDialog.h"
#include "ShortcutWindow.h"
#include "Commands.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include <sstream>
#include <string>
#include <QOpenGLWidget>
#include <QGuiApplication>
#include <QScreen>

namespace mmp {

MainWindow::MainWindow()
{
  // Create model.
#if QT_VERSION >= 0x050500
  QMessageLogger(__FILE__, __LINE__, nullptr).info() << "Video support: " <<
      (Video::hasVideoSupport() ? "yes" : "no");
#else
  QMessageLogger(__FILE__, __LINE__, 0).debug() << "Video support: " <<
      (Video::hasVideoSupport() ? "yes" : "no");
#endif

  mappingManager = new MappingManager;


  // Initialize internal variables.
  currentSourceId = NULL_UID;
  currentLayerId = NULL_UID;
  // TODO: not sure we need this anymore since we have NULL_UID
  _hasCurrentSource = false;
  _hasCurrentLayer = false;
  currentSelectedItem = NULL;

  // Frames per second.
  _framesPerSecond = (-1);

  // Play state.
  _isPlaying = false;

  // Editing toggles.
  _displayControls = true;
  _displaySourceControls = true;
  _stickyVertices = true;
  _displayUndoStack = false;
  _showMenuBar = true; // Show menubar by default

  // UndoStack
  undoStack = new QUndoStack(this);

  // Create everything.
  createLayout();
  createActions();
  createMenus();
  createLayerContextMenu();
  createSourceContextMenu();
  createToolBars();
  createStatusBar();
  updateRecentFileActions();
  updateRecentVideoActions();

  // Load settings.
  readSettings();

  // Start osc.
  startOscReceiver();

  // Defaults.
  setWindowIcon(QIcon(":/mapmap-logo"));
  setCurrentFile("");

  // Allow drag n drop
  setAcceptDrops(true);

  // Create and start timer.
  videoTimer = new QTimer(this);
  connect(videoTimer, SIGNAL(timeout()), this, SLOT(processFrame()));
  setFramesPerSecond(MM::DEFAULT_FRAMES_PER_SECOND);
  videoTimer->start();

  // Create elapsed timer.
  systemTimer = new QElapsedTimer;
  systemTimer->start();

  // Start playing by default.
  play();
}

MainWindow::~MainWindow()
{
  delete mappingManager;
  //  delete _facade;
  delete osc_timer;
  delete systemTimer;
}

void MainWindow::handleSourceItemSelectionChanged()
{
  // Set current source.
  QListWidgetItem* item = sourceList->currentItem();
  currentSelectedItem = item;

  // Is a source item selected?
  bool sourceItemSelected = (item ? true : false);

  if (sourceItemSelected)
  {
    // Set current source.
    uid sourceId = getItemId(*item);
    // Unselect current mapping.
    if (currentSourceId != sourceId)
      removeCurrentLayer();
    // Set current source.
    setCurrentSource(sourceId);
  }
  else
    removeCurrentSource();

  // Enable/disable creation of mappings depending on whether a source is selected.
  addMeshAction->setEnabled(sourceItemSelected);
  addTriangleAction->setEnabled(sourceItemSelected);
  addEllipseAction->setEnabled(sourceItemSelected);
  deleteSourceAction->setEnabled(sourceItemSelected);
  renameSourceAction->setEnabled(sourceItemSelected);

  // Update canvases.
  updateCanvases();
}

void MainWindow::handleLayerItemSelectionChanged(const QModelIndex &index)
{
  // Set current source and mappings.
  uid layerId = layerListModel->getItemId(index);
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  uid sourceId = layer->getSource()->getId();
  // Set current mapping and source
  setCurrentLayer(layerId);
  setCurrentSource(sourceId);
  // Enable destination zoom toolbar buttons and avoid loop
  if (!destinationCanvasToolbar->buttonsAreEnable()) {
    // Enable destination toolbar
   destinationCanvasToolbar->enableZoomToolBar(true);
   // Enable source toolbar
   sourceCanvasToolbar->enableZoomToolBar(true);
   // Enable source and mapping edit action
   duplicateLayerAction->setEnabled(true);
   deleteLayerAction->setEnabled(true);
   renameLayerAction->setEnabled(true);
   layerLockedAction->setEnabled(true);
   layerHideAction->setEnabled(true);
   layerSoloAction->setEnabled(true);
   layerRotate90CWAction->setEnabled(true);
   layerRotate90CCWAction->setEnabled(true);
   layerRotate180Action->setEnabled(true);
   layerHorizontalFlipAction->setEnabled(true);
   layerVerticalFlipAction->setEnabled(true);
   layerRaiseAction->setEnabled(true);
   layerLowerAction->setEnabled(true);
   layerRaiseToTopAction->setEnabled(true);
   layerLowerToBottomAction->setEnabled(true);
   // Enable zoom action
   zoomInAction->setEnabled(true);
   zoomOutAction->setEnabled(true);
   resetZoomAction->setEnabled(true);
   fitToViewAction->setEnabled(true);
  }

  // Update canvases.
  updateCanvases();
  updateLayerListColumnWidth();
}

void MainWindow::handleLayerItemChanged(const QModelIndex &index)
{
  // Get item.
  uid layerId = layerListModel->getItemId(index);

  // Sync name.
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  // Change properties.
  layer->setName(index.data(Qt::EditRole).toString());
  layer->setVisible(index.data(Qt::CheckStateRole).toBool());
  layer->setSolo(index.data(Qt::CheckStateRole + 1).toBool());
  layer->setLocked(index.data(Qt::CheckStateRole + 2).toBool());

  // Update model (important to make sure icons get updated in the interface).
  layerListModel->updateModel();

  updatePlayingState();
 }

void MainWindow::handleLayerIndexesMoved()
{
  // Resync mapping manager.
  syncLayerManager();

  // Update canvases according to new order.
  updateCanvases();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::handleSourceItemSelected(QListWidgetItem* item)
{
  Q_UNUSED(item);
  // Change currently selected item.
  currentSelectedItem = item;
}

void MainWindow::handleSourceChanged(Source::ptr source)
{
  // Change currently selected item.
  uid curLayerId = getCurrentLayerId();
  removeCurrentLayer();
  removeCurrentSource();

  uid sourceId = mappingManager->getSourceId(source);

//  QSharedPointer<Texture> texture;

  if (source->getSourceType() == SourceType::Video)
  {
    QSharedPointer<Video> media = qSharedPointerCast<Video>(source);
    Q_CHECK_PTR(media);
    updateSourceItem(sourceId, getSourceIcon(source), strippedName(media->getUri()));
    //    QString fileName = QFileDialog::getOpenFileName(this,
    //        tr("Import media source file"), ".");
    //    // Restart video playback. XXX Hack
    //    if (!fileName.isEmpty())
    //      importMediaFile(fileName, source, false);
  }
  if (source->getSourceType() == SourceType::Image)
  {
    QSharedPointer<Image> image = qSharedPointerCast<Image>(source);
    Q_CHECK_PTR(image);
    updateSourceItem(sourceId, getSourceIcon(source), strippedName(image->getUri()));
    //    QString fileName = QFileDialog::getOpenFileName(this,
    //        tr("Import media source file"), ".");
    //    // Restart video playback. XXX Hack
    //    if (!fileName.isEmpty())
    //      importMediaFile(fileName, source, true);
  }
  else if (source->getSourceType() == SourceType::Color)
  {
    // Pop-up color-choosing dialog to choose color source.
    QSharedPointer<Color> color = qSharedPointerCast<Color>(source);
    Q_CHECK_PTR(color);
    updateSourceItem(sourceId, getSourceIcon(source), strippedName(color->getColor().name()));
  }

  if (curLayerId != NULL_UID)
  {
    setCurrentLayer(curLayerId);
  }

//  updatePlayingState();
}

void MainWindow::layerPropertyChanged(uid id, QString propertyName, QVariant value)
{
  // Retrieve mapping.
  Layer::ptr layer = mappingManager->getLayerById(id);
  Q_CHECK_PTR(layer);

  // Send to mapping gui.
  LayerGui::ptr layerGui = getLayerGuiByLayerId(id);
  Q_CHECK_PTR(layerGui);
  layerGui->setValue(propertyName, value);

  // Send to actions.
  if (layer == getCurrentLayer())
  {
    if (propertyName == "visible")
    {
      layerHideAction->setChecked(!value.toBool());
      updatePlayingState();
    }
    else if (propertyName == "solo")
    {
      layerSoloAction->setChecked(value.toBool());
      updatePlayingState();
    }
    else if (propertyName == "locked")
    {
      layerLockedAction->setChecked(value.toBool());
    }
    else if (propertyName == "sourceId")
    {
      layerGui->updateSources();
      updatePlayingState();
    }
  }

  // Send to list items.
  const QModelIndex& index = layerListModel->getIndexFromId(layer->getId());
  if (propertyName == "name")
  {
    layerListModel->setData(index, layer->getName(), Qt::EditRole);
  }
  else if (propertyName == "visible")
  {
    layerListModel->setData(index, layer->isVisible(), Qt::CheckStateRole);
  }
  else if (propertyName == "solo")
  {
    layerListModel->setData(index, layer->isSolo(), Qt::CheckStateRole + 1);
  }
  else if (propertyName == "locked")
  {
    layerListModel->setData(index, layer->isLocked(), Qt::CheckStateRole + 2);
  }
}

void MainWindow::sourcePropertyChanged(uid id, QString propertyName, QVariant value)
{
  // Retrieve source.
  Source::ptr source = mappingManager->getSourceById(id);
  Q_CHECK_PTR(source);

  // Send to source gui.
  SourceGui::ptr sourceGui = getSourceGuiBySourceId(id);
  Q_CHECK_PTR(sourceGui);

  sourceGui->setValue(propertyName, value);

  // Send to list items.
  QListWidgetItem* sourceItem = getItemFromId(*sourceList, id);
  if (propertyName == "name")
    sourceItem->setText(source->getName());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Stop video playback to avoid lags. XXX Hack
  pause(false);

  // Popup dialog allowing the user to save before closing.
  if (okToContinue())
  {
    // Save settings
    writeSettings();
//    _preferenceDialog->saveSettings();
    // Close all top level widgets
    for (QWidget *widget: QApplication::topLevelWidgets()) {
      if (widget != this) { // Avoid recursion
        widget->close();
      }
    }
    event->accept();
  }
  else
  {
    event->ignore();
  }

  // Restart video playback. XXX Hack
  play(false);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
#ifdef Q_OS_MACOS // On Mac OS X
  Q_UNUSED(event);
  // Do nothing
#endif

#ifdef Q_OS_LINUX // On Linux
  if (event->modifiers() & Qt::AltModifier) {
    QString currentDesktop = QString(getenv("XDG_CURRENT_DESKTOP")).toLower();
    if (currentDesktop != "unity" && !_showMenuBar) {
      menuBar()->setHidden(!menuBar()->isHidden());
      menuBar()->setFocus(Qt::MenuBarFocusReason);
    }
  }
#endif
#ifdef Q_OS_WIN32
  if (event->modifiers() & Qt::AltModifier) {
    if (!_showMenuBar) {
      menuBar()->setHidden(!menuBar()->isHidden());
      menuBar()->setFocus(Qt::MenuBarFocusReason);
    }
  }
#endif
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
  QMenu *menu = static_cast<QMenu*>(object);

  if (menu && (event->type() == QEvent::MouseButtonPress
      || event->type() == QEvent::MouseButtonDblClick))
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    // Disable right click on context menu actions
    if (mouseEvent->buttons() & Qt::RightButton) {
      mouseEvent->ignore();
      return true;
    }
    return false;
  }

  return QMainWindow::eventFilter(object, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  const QMimeData *mimeData = event->mimeData();
  bool allowDrag = true;

  if (mimeData->hasUrls()) {
    for (const QUrl& url : mimeData->urls()) {
      QString fileName = url.toLocalFile();
      // Don't allow drag if file is not supported
      if (!fileSupported(fileName, MM::FILE_EXTENSION) &&
          !fileSupported(fileName, MM::IMAGE_FILES_FILTER) &&
          !fileSupported(fileName, MM::VIDEO_FILES_FILTER)) {
        allowDrag = false;
      }
    }
  }

  if (allowDrag)
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
  event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
  const QMimeData *mimeData = event->mimeData();

  if (mimeData->hasUrls()) {
    // In case that dragged many files
    for (const QUrl& url : mimeData->urls()) {
      QString fileName = url.toLocalFile();

      if (!fileName.isEmpty()) {
        // Test if is mmp file and exit loop
        if (fileSupported(fileName, MM::FILE_EXTENSION)) {
          if (okToContinue()) {
            loadFile(fileName);
          }
          // Exit for prevent drag to many project files
          break;
        }
        // Allow to drag too many videos or images
        else {
          // Check if file is image or not
          // according to file extension
          if (fileSupported(fileName, MM::IMAGE_FILES_FILTER))
            importMediaFile(fileName, true);
          else
            importMediaFile(fileName, false);
        }
      }
    }
  }
  event->acceptProposedAction();
}

void MainWindow::setOutputWindowFullScreen(bool enable)
{
  outputWindow->setFullScreen(enable);
  // setCheckState
  displayControlsAction->setChecked(enable);
  displaySourceControlsAction->setChecked(enable);
 }

void MainWindow::newFile()
{
  // Stop video playback to avoid lags. XXX Hack
  pause(false);

  // Popup dialog allowing the user to save before creating a new file.
  if (okToContinue())
  {
    clearWindow();
    setCurrentFile("");
    undoStack->clear();
  }

  // Restart video playback. XXX Hack
  play(false);
}

void MainWindow::open()
{
  // Stop video playback to avoid lags. XXX Hack
  pause(false);

  // Popup dialog allowing the user to save before opening a new file.
  if (okToContinue())
  {
// Temporary fix of QFileDialog on GTK
#ifdef Q_OS_LINUX
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open project"),
                                                    settings.value("defaultProjectDir").toString(),
                                                    tr("MapMap files (*.%1)").arg(MM::FILE_EXTENSION),
                                                    nullptr, QFileDialog::DontUseNativeDialog);
#else
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open project"),
                                                    settings.value("defaultProjectDir").toString(),
                                                    tr("MapMap files (*.%1)").arg(MM::FILE_EXTENSION));
#endif

    if (! fileName.isEmpty())
      loadFile(fileName);
  }

  // Restart video playback. XXX Hack
  play(false);
}

bool MainWindow::save()
{
  // Popup save-as dialog if file has never been saved.
  if (curFile.isEmpty())
  {
    return saveAs();
  }
  else
  {
    return saveFile(curFile);
  }
}

bool MainWindow::saveAs()
{
  // Stop video playback to avoid lags. XXX Hack
  pause(false);

#ifdef Q_OS_LINUX
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  tr("Save project"), settings.value("defaultProjectDir").toString(),
                                                  tr("MapMap files (*.%1)").arg(MM::FILE_EXTENSION),
                                                  nullptr, QFileDialog::DontUseNativeDialog);
#else
  // Popul file dialog to choose filename.
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  tr("Save project"), settings.value("defaultProjectDir").toString(),
                                                  tr("MapMap files (*.%1)").arg(MM::FILE_EXTENSION));
#endif

  // Restart video playback. XXX Hack
  play(false);

  if (fileName.isEmpty())
    return false;

  if (! fileName.endsWith(MM::FILE_EXTENSION))
  {
    std::cout << "filename doesn't end with expected extension: " <<
                 fileName.toStdString() << std::endl;
    fileName.append(".");
    fileName.append(MM::FILE_EXTENSION);
  }

  // Save to filename.
  return saveFile(fileName);
}

void MainWindow::importMedia()
{
  // Stop video playback, if it is playing, to avoid lags. XXX Hack
  pause(!pauseAction->isVisible());

  // Pop-up file-choosing dialog to choose media file.
  // TODO: restrict the type of files that can be imported
#ifdef Q_OS_LINUX
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Import media source file"),
                                                  settings.value("defaultVideoDir").toString(),
                                                  tr("Media files (%1 %2);;All files (*)")
                                                  .arg(MM::VIDEO_FILES_FILTER)
                                                  .arg(MM::IMAGE_FILES_FILTER),
                                                  nullptr, QFileDialog::DontUseNativeDialog);
#else
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Import media source file"),
                                                  settings.value("defaultVideoDir").toString(),
                                                  tr("Media files (%1 %2);;All files (*)")
                                                  .arg(MM::VIDEO_FILES_FILTER)
                                                  .arg(MM::IMAGE_FILES_FILTER));
#endif
  // Restart video playback if it was previously playing. XXX Hack
  play(!pauseAction->isVisible());

  // Check if file is image or not
  // according to file extension
  if (!fileName.isEmpty()) {
    if (!QFileInfo(fileName).suffix().isEmpty() && MM::IMAGE_FILES_FILTER.contains(QFileInfo(fileName).suffix(), Qt::CaseInsensitive))
      importMediaFile(fileName, true);
    else
      importMediaFile(fileName, false);
  }
}

void MainWindow::openCameraDevice()
{
#if QT_VERSION >= 0x050300
  // Stop video playback, if it is playing, to avoid lags. XXX Hack
  pause(!pauseAction->isVisible());

  QString device;
  QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

  if (cameras.count() > 1)
  {
    QStringList devicesList;
    QMap<QString, QString> devices;

    for (const QCameraDevice &cameraInfo: cameras)
    {
      devicesList << cameraInfo.description();
      devices.insert(cameraInfo.description(), QString::fromUtf8(cameraInfo.id()));
    }

    bool ok;
    QString deviceName = QInputDialog::getItem(this, tr("Camera device"),
                                               tr("Select camera"), devicesList, 0, false, &ok);

    if (ok && !deviceName.isEmpty())
    {
      if (devices.contains(deviceName))
        device = devices.value(deviceName);
    }

  }

  else
  {
    if (QMediaDevices::defaultVideoInput().isNull())
    {
      QMessageBox::warning(this, tr("No camera available"), tr("You can not use this feature!\nNo camera available in your system"));

    }
    else
    {
      device = QString::fromUtf8(QMediaDevices::defaultVideoInput().id());
    }
  }

  // Restart video playback if it was previously playing. XXX Hack
  play(!pauseAction->isVisible());

  if (!device.isEmpty())
    importMediaFile(device, false, true);
#else
    QMessageBox::warning(this, tr("No camera available"), tr("You can not use this feature!\nNo camera available in your system"));
#endif
}

void MainWindow::addColor()
{
  // Stop video playback, if it is playing, to avoid lags. XXX Hack
  if (pauseAction->isVisible())
    pause(false);

  // Pop-up color-choosing dialog to choose color source.
  // FIXME: we use a static variable to store the last chosen color
  // it should rather be a member of this class, or so.
  static QColor color = QColor(0, 255, 0, 255);
#ifdef Q_OS_LINUX
  color = QColorDialog::getColor(color, this, tr("Select Color"),
                                  QColorDialog::DontUseNativeDialog |
                                 QColorDialog::ShowAlphaChannel);
#else
  color = QColorDialog::getColor(color, this, tr("Select Color"),
                                 // QColorDialog::DontUseNativeDialog |
                                 QColorDialog::ShowAlphaChannel);
#endif
  if (color.isValid())
  {
    addColorSource(color);
  }

  // Restart video playback if it was previously playing. XXX Hack
  if (pauseAction->isVisible())
    play(false);
}

void MainWindow::addMesh()
{
  // A source must be selected to add a mapping.
  if (getCurrentSourceId() == NULL_UID)
    return;

  // Retrieve current source (as texture).
  Source::ptr source = getMappingManager().getSourceById(getCurrentSourceId());
  Q_CHECK_PTR(source);

  // Create input and output quads.
  Layer* layerPtr;
  if (source->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputQuad = MShape::ptr(Util::createMeshForColor(sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new ColorLayer(source, outputQuad);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(source);
    Q_CHECK_PTR(texture);

    MShape::ptr outputQuad = MShape::ptr(Util::createMeshForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr  inputQuad = MShape::ptr(Util::createMeshForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new TextureLayer(source, outputQuad, inputQuad);
  }

  // Create texture mapping.
  Layer::ptr layer(layerPtr);
  uint layerId = mappingManager->addLayer(layer);

  // Lets the undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddLayerCommand(this, layerId));
}

void MainWindow::addTriangle()
{
  // A source must be selected to add a mapping.
  if (getCurrentSourceId() == NULL_UID)
    return;

  // Retrieve current source (as texture).
  Source::ptr source = getMappingManager().getSourceById(getCurrentSourceId());
  Q_CHECK_PTR(source);

  // Create input and output quads.
  Layer* layerPtr;
  if (source->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputTriangle = MShape::ptr(Util::createTriangleForColor(sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new ColorLayer(source, outputTriangle);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(source);
    Q_CHECK_PTR(texture);

    MShape::ptr outputTriangle = MShape::ptr(Util::createTriangleForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr inputTriangle = MShape::ptr(Util::createTriangleForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new TextureLayer(source, inputTriangle, outputTriangle);
  }

  // Create mapping.
  Layer::ptr layer(layerPtr);
  uint layerId = mappingManager->addLayer(layer);

  // Lets undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddLayerCommand(this, layerId));
}

void MainWindow::addEllipse()
{
  // A source must be selected to add a mapping.
  if (getCurrentSourceId() == NULL_UID)
    return;

  // Retrieve current source (as texture).
  Source::ptr source = getMappingManager().getSourceById(getCurrentSourceId());
  Q_CHECK_PTR(source);

  // Create input and output ellipses.
  Layer* layerPtr;
  if (source->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputEllipse = MShape::ptr(Util::createEllipseForColor(sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new ColorLayer(source, outputEllipse);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(source);
    Q_CHECK_PTR(texture);

    MShape::ptr outputEllipse = MShape::ptr(Util::createEllipseForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr inputEllipse = MShape::ptr(Util::createEllipseForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    layerPtr = new TextureLayer(source, inputEllipse, outputEllipse);
  }

  // Create mapping.
  Layer::ptr layer(layerPtr);
  uint layerId = mappingManager->addLayer(layer);

  // Lets undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddLayerCommand(this, layerId));
}

void MainWindow::about()
{
  // Stop video playback to avoid lags. XXX Hack
  pause(false);
  _aboutDialog = new AboutDialog(this);
  _aboutDialog->setAttribute(Qt::WA_DeleteOnClose); // Important for ressource management
  _aboutDialog->show();

  // Restart video playback. XXX Hack
  play(false);
}

void MainWindow::updateStatusBar()
{
  QPointF mousePos = destinationCanvas->mapToScene(destinationCanvas->mapFromGlobal(destinationCanvas->cursor().pos()));
  if (currentSelectedItem) // Show mouse coordinate only if layerList is not empty
    mousePosLabel->setText("Mouse coordinate:   X " + QString::number(mousePos.x()) + "   Y " + QString::number(mousePos.y()));
  else
    mousePosLabel->setText(""); // Otherwise set empty text.
  currentMessageLabel->setText(statusBar()->currentMessage());
  sourceZoomLabel->setText("Input Editor: " + QString::number(int(sourceCanvas->getZoomFactor() * 100)).append(QChar('%')));
  destinationZoomLabel->setText("Output Editor: " + QString::number(int(destinationCanvas->getZoomFactor() * 100)).append(QChar('%')));
  lastActionLabel->setText(undoStack->text(undoStack->count() - 1));
}

void MainWindow::showMenuBar(bool shown)
{
  _showMenuBar = shown;

#ifdef Q_OS_MACOS // On Mac OS X
  // Do nothing
#endif
#ifdef Q_OS_LINUX // On Linux
  QString currentDesktop = QString(getenv("XDG_CURRENT_DESKTOP")).toLower();
  if (currentDesktop != "unity")
    menuBar()->setVisible(shown);
#endif
#ifdef Q_OS_WIN32 // On Windows
    menuBar()->setVisible(shown);
#endif
}

/**
 * Called when the user wants to delete an item.
 *
 * Deletes either a Source or a Mapping.
 */
void MainWindow::deleteItem()
{
  bool isLayerTabSelected = (layerSplitter == contentTab->currentWidget());
  bool isSourceTabSelected = (sourceSplitter == contentTab->currentWidget());

  if (currentSelectedItem)
  {
    if (isLayerTabSelected) //currentSelectedItem->listWidget() == layerList)
    {
      // Delete mapping.
      undoStack->push(new DeleteLayerCommand(this, getCurrentLayerId()));
      //currentSelectedItem = NULL;
    }
    else if (isSourceTabSelected) //currentSelectedItem->listWidget() == sourceList)
    {
      // Delete source.
      undoStack->push(new RemoveSourceCommand(this, getItemId(*sourceList->currentItem())));
      //currentSelectedItem = NULL;
    }
    else
    {
      qCritical() << "Selected item neither a mapping nor a source." << Qt::endl;
    }
  }
}

void MainWindow::duplicateLayerItem()
{
  if (currentSelectedIndex.isValid())
  {
    duplicateLayer(currentLayerItemId());
  }
  else
  {
    qCritical() << "No selected mapping" << Qt::endl;
  }
}

void MainWindow::deleteLayerItem()
{
  if (hasCurrentLayer())
  {
    undoStack->push(new DeleteLayerCommand(this, getCurrentLayerId()));
  }
  else
  {
    qCritical() << "No selected mapping" << Qt::endl;
  }
}

void MainWindow::renameLayerItem()
{
  // Set current item editable and rename it
  QModelIndex index = layerList->currentIndex();
  // Used by context menu
  layerList->edit(index);
  // Switch to mapping tab.
  contentTab->setCurrentWidget(layerSplitter);
}

void MainWindow::setLayerItemLocked(bool locked)
{
  setLayerLocked(currentLayerItemId(), locked);
}

void MainWindow::setLayerItemHide(bool hide)
{
  setLayerVisible(currentLayerItemId(), !hide);
}

void MainWindow::setLayerItemSolo(bool solo)
{
  setLayerSolo(currentLayerItemId(), solo);
}

void MainWindow::loadLayerMedia()
{
  QAction *action = qobject_cast<QAction *>(sender());
  Source::ptr media;
  uid currentLayerId = getCurrentLayer()->getId();

  if (action) {
    if (action->data().toString() == "import-new-media") {
      // Due to the fact that we can't assign a media/source without adding a mesh
      importMedia();
      addMesh(); // Creating a temporary mesh
      media = mappingManager->getSourceById(currentSourceId); // The last imported video is current ID
      deleteLayer(getCurrentLayer()->getId()); // Delete the temporary mesh
      setCurrentLayer(currentLayerId); // Set the previous selected layer as the current
    } else {
      media = mappingManager->getSourceById(action->data().toInt());
    }

    if (media && media != getCurrentLayer()->getSource() &&
        getCurrentLayer()->sourceIsCompatible(media)) {
      // Change layer source
      getCurrentLayer()->setSource(media);
    }
  }
}

void MainWindow::transformActionLayerItem()
{
  QAction *actionSender = qobject_cast<QAction *>(sender());

  if (actionSender == layerRotate90CWAction) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate90CW));
  }
  else if (actionSender == layerRotate90CCWAction) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate90CCW));
  }
  else if (actionSender == layerRotate180Action) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate180));
  }

  else if (actionSender == layerHorizontalFlipAction) {
    undoStack->push(new FlipShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Horizontal));
  }
  else if (actionSender == layerVerticalFlipAction) {
    undoStack->push(new FlipShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Vertical));
  }

}

void MainWindow::reorderLayerItem()
{
  QAction *actionSender = qobject_cast<QAction *>(sender());

  if (actionSender == layerRaiseAction) {
    undoStack->push(new MoveLayerCommand(this, getCurrentLayerId(), MM::Raise));
  }
  else if (actionSender == layerLowerAction) {
    undoStack->push(new MoveLayerCommand(this, getCurrentLayerId(), MM::Lower));
  }
  else if (actionSender == layerRaiseToTopAction) {
    undoStack->push(new MoveLayerCommand(this, getCurrentLayerId(), MM::Top));
  }
  else if (actionSender == layerLowerToBottomAction) {
    undoStack->push(new MoveLayerCommand(this, getCurrentLayerId(), MM::Bottom));
  }
}

void MainWindow::renameLayer(uid layerId, const QString &name)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  if (!layer.isNull()) {
    QModelIndex index = layerListModel->getIndexFromId(layerId);
    layerListModel->setData(index, name, Qt::EditRole);
    layer->setName(name);
  }
}

//void MainWindow::layerListEditEnd(QWidget *editor)
//{
//  QString name = reinterpret_cast<QLineEdit*>(editor)->text();
//  renameMapping(getItemId(*layerList->currentItem()), name);
//}

void MainWindow::deleteSourceItem()
{
  if(hasCurrentSource())
  {
    undoStack->push(new RemoveSourceCommand(this, getCurrentSourceId()));
  }
  else
  {
    qCritical() << "No selected source" << Qt::endl;
  }
}

void MainWindow::renameSourceItem()
{
  // Set current item editable and rename it
  QListWidgetItem* item = sourceList->currentItem();
  item->setFlags(item->flags() | Qt::ItemIsEditable);
  // Used by context menu
  sourceList->editItem(item);
  // Switch to source tab
  contentTab->setCurrentWidget(sourceSplitter);
}

void MainWindow::renameSource(uid sourceId, const QString &name)
{
  Source::ptr source = mappingManager->getSourceById(sourceId);
  Q_CHECK_PTR(source);
  if (!source.isNull()) {
    source->setName(name);
  }
}

void MainWindow::sourceListEditEnd(QWidget *editor)
{
  QString name = reinterpret_cast<QLineEdit*>(editor)->text();
  renameSource(getItemId(*sourceList->currentItem()), name);
}

void MainWindow::setupOutputScreen()
{
  QAction *actionSender = qobject_cast<QAction *>(sender());

  if (actionSender)
    outputWindow->setPreferredScreen(actionSender->data().toInt());
  // If want that the changes take effect immediatelly
  // when the output is in fullscreen mode
  if (outputFullScreenAction->isChecked()) {
    // XXX: Close and reopen // It's not the best way to do
    outputFullScreenAction->toggle();
    outputFullScreenAction->trigger();
  }
}

void MainWindow::updateScreenCount()
{
  // Clear action list before
  screenActions.clear();
  // Refresh screen action
  updateScreenActions();
  // Update Output menu
  outputScreenMenu->clear();
  outputScreenMenu->addActions(screenActions);
}

void MainWindow::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    loadFile(action->data().toString());
}

void MainWindow::openRecentVideo()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    importMediaFile(action->data().toString(), false);
}

bool MainWindow::clearProject()
{
  // Disconnect signals to avoid problems when clearning layerList and sourceList.
  disconnectProjectWidgets();

  // Clear current source / mapping.
  removeCurrentSource();
  removeCurrentLayer();

  // Empty list widgets.
  layerListModel->clear();
  sourceList->clear();

  // Clear property panel.
  for (int i=layerPropertyPanel->count()-1; i>=0; i--)
    layerPropertyPanel->removeWidget(layerPropertyPanel->widget(i));

  // Disable property panel.
  layerPropertyPanel->setDisabled(true);

  // Clear list of layerGuis.
  layerGuis.clear();

  // Clear list of source guis.
  sourceGuis.clear();

  // Clear model.
  mappingManager->clearAll();

  // Refresh GL canvases to clear them out.
  sourceCanvas->repaint();
  destinationCanvas->repaint();

  // Reconnect everything.
  connectProjectWidgets();

  // Window was modified.
  windowModified();

  return true;
}

uid MainWindow::createMediaSource(uid sourceId, QString uri, float x, float y,
                                 bool isImage, VideoType type, double rate)
{
  // Cannot create image with already existing id.
  if (Source::getUidAllocator().exists(sourceId))
    return NULL_UID;

  else
  {
    Texture* tex = nullptr;
    if (isImage)
      tex = new Image(uri, sourceId);
    else {
      tex = new Video(uri, type, rate, sourceId);
    }

    // Create new image with corresponding ID.
    tex->setPosition(x, y);

    // Add it to the manager.
    Source::ptr source(tex);

    if (type == VIDEO_WEBCAM) {
      source->setName(tex->getCameraNameFromUri(uri));
    } else {
      source->setName(strippedName(uri));
    }

    // Add source to model and return its uid.
    uid id = mappingManager->addSource(source);

    // Add source widget item.
    undoStack->push(new AddSourceCommand(this, id, source->getIcon(), source->getName()));
    return id;
  }
}

uid MainWindow::createColorSource(uid sourceId, QColor color)
{
  // Cannot create image with already existing id.
  if (Source::getUidAllocator().exists(sourceId))
    return NULL_UID;

  else
  {
    Color* img = new Color(color, sourceId);

    // Add it to the manager.
    Source::ptr source(img);
    source->setName(strippedName(color.name()));

    // Add source to model and return its uid.
    uid id = mappingManager->addSource(source);

    // Add source widget item.
    undoStack->push(new AddSourceCommand(this, id, source->getIcon(), source->getName()));

    return id;
  }
}

uid MainWindow::createMeshTextureLayer(uid layerId,
                                         uid sourceId,
                                         int nColumns, int nRows,
                                         const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    int nVertices = nColumns * nRows;
    qDebug() << nVertices << " vs " << nColumns << "x" << nRows << " vs " << src.size() << " " << dst.size() << Qt::endl;
    Q_ASSERT(src.size() == nVertices && dst.size() == nVertices);

    MShape::ptr inputMesh( new Mesh(src, nColumns, nRows));
    MShape::ptr outputMesh(new Mesh(dst, nColumns, nRows));

    // Add it to the manager.
    Layer::ptr layer(new TextureLayer(source, outputMesh, inputMesh, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createTriangleTextureLayer(uid layerId,
                                             uid sourceId,
                                             const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    Q_ASSERT(src.size() == 3 && dst.size() == 3);

    MShape::ptr inputTriangle( new Triangle(src[0], src[1], src[2]));
    MShape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

    // Add it to the manager.
    Layer::ptr layer(new TextureLayer(source, outputTriangle, inputTriangle, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createEllipseTextureLayer(uid layerId,
                                            uid sourceId,
                                            const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    Q_ASSERT(src.size() == 5 && dst.size() == 5);

    MShape::ptr inputEllipse( new Ellipse(src[0], src[1], src[2], src[3], src[4]));
    MShape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3], dst[4]));

    // Add it to the manager.
    Layer::ptr layer(new TextureLayer(source, outputEllipse, inputEllipse, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createQuadColorLayer(uid layerId,
                                       uid sourceId,
                                       const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    Q_ASSERT(dst.size() == 4);

    MShape::ptr outputQuad(new Quad(dst[0], dst[1], dst[2], dst[3]));

    // Add it to the manager.
    Layer::ptr layer(new ColorLayer(source, outputQuad, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createTriangleColorLayer(uid layerId,
                                           uid sourceId,
                                           const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    Q_ASSERT(dst.size() == 3);

    MShape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

    // Add it to the manager.
    Layer::ptr layer(new ColorLayer(source, outputTriangle, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createEllipseColorLayer(uid layerId,
                                          uid sourceId,
                                          const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no source exists.
  if (Layer::getUidAllocator().exists(layerId) ||
      !Source::getUidAllocator().exists(sourceId) ||
      sourceId == NULL_UID)
    return NULL_UID;

  else
  {
    Source::ptr source = mappingManager->getSourceById(sourceId);
    Q_ASSERT(dst.size() == 4);

    MShape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3]));

    // Add it to the manager.
    Layer::ptr layer(new ColorLayer(source, outputEllipse, layerId));
    uid id = mappingManager->addLayer(layer);

    // Add it to the GUI.
    addLayerItem(layerId);

    // Return the id.
    return id;
  }
}


void MainWindow::setLayerVisible(uid layerId, bool visible)
{
  // Set mapping visibility
  Layer::ptr layer = mappingManager->getLayerById(layerId);

  if (layer.isNull())
  {
    qDebug() << "No such mapping id" << Qt::endl;
  }
  else
  {
    layer->setVisible(visible);
    // Change list item check state
    QModelIndex index = layerListModel->getIndexFromId(layerId);
    layerListModel->setData(index, visible, Qt::CheckStateRole);
    // Update canvases.
    updateCanvases();
  }
}

void MainWindow::setLayerSolo(uid layerId, bool solo)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  if (!layer.isNull()) {
    // Turn this mapping into solo mode
    layer->setSolo(solo);
    // Change list item check state
    QModelIndex index = layerListModel->getIndexFromId(layerId);
    layerListModel->setData(index, solo, Qt::CheckStateRole + 1);
    // Update canvases
    updateCanvases();
  }
}

void MainWindow::setLayerLocked(uid layerId, bool locked)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);

  if (!layer.isNull()) {
    // Lock position of mapping
    layer->setLocked(locked);
    // Lock shape too.
    layer->getShape()->setLocked(locked);
    // Change list item check state
    QModelIndex index = layerListModel->getIndexFromId(layerId);
    layerListModel->setData(index, locked, Qt::CheckStateRole + 2);
    // Update canvases
    updateCanvases();
  }
}

void MainWindow::deleteLayer(uid layerId)
{
  // Cannot delete unexisting mapping.
  if (Layer::getUidAllocator().exists(layerId))
  {
    removeLayerItem(layerId);
  }
}

void MainWindow::moveLayer(uid layerId, int idx)
{
  // Cannot delete unexisting mapping.
  if (Layer::getUidAllocator().exists(layerId))
  {
    moveLayerItem(layerId, idx);
  }
}

void MainWindow::duplicateLayer(uid layerId)
{
  // Clone current Mapping.
  Layer::ptr clonedMappingPtr(mappingManager->getLayerById(layerId)->clone());

  // Get duplicated mapping id
  uid cloneId = mappingManager->addLayer(clonedMappingPtr);

  // Lets the undo-stack handle Undo/Redo the duplication of mapping item.
  undoStack->push(new DuplicateLayerCommand(this, cloneId));
}

/// Deletes/removes a source and all associated mappigns.
void MainWindow::deleteSource(uid sourceId, bool replace)
{
  // Cannot delete unexisting source.
  if (Source::getUidAllocator().exists(sourceId))
  {
    if (replace == false) {
      int r = QMessageBox::warning(this, tr("MapMap"),
                                   tr("Remove this source and all its associated layers?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
      if (r == QMessageBox::Ok)
      {
        removeSourceItem(sourceId);
      }
    }
    else
      removeSourceItem(sourceId);
  }
}

void MainWindow::windowModified()
{
  setWindowModified(true);
  updateStatusBar();
  updateLayerActions();
}

void MainWindow::createLayout()
{
  // Create source list.
  sourceList = new QListWidget;
  sourceList->setSelectionMode(QAbstractItemView::SingleSelection);
  sourceList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sourceList->setDefaultDropAction(Qt::MoveAction);
  sourceList->setDragDropMode(QAbstractItemView::InternalMove);
  sourceList->setMinimumWidth(PAINT_LIST_MINIMUM_HEIGHT);

  // Create source panel.
  sourcePropertyPanel = new QStackedWidget;
  sourcePropertyPanel->setDisabled(true);
  sourcePropertyPanel->setMinimumHeight(PAINT_PROPERTY_PANEL_MINIMUM_HEIGHT);

  // Create mapping list.
  layerList = new QTableView;
  layerList->setSelectionMode(QAbstractItemView::SingleSelection);
  layerList->setSelectionBehavior(QAbstractItemView::SelectRows);
  layerList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  layerList->setDragEnabled(true);
  layerList->setAcceptDrops(true);
  layerList->setDropIndicatorShown(true);
  layerList->setEditTriggers(QAbstractItemView::DoubleClicked);
  layerList->setMinimumHeight(MAPPING_LIST_MINIMUM_HEIGHT);
  layerList->setContentsMargins(0, 0, 0, 0);
  // Set view delegate
  layerListModel = new LayerListModel;
  layerItemDelegate = new LayerItemDelegate;
  layerList->setModel(layerListModel);
  layerList->setItemDelegate(layerItemDelegate);
  // Pimp Mapping table widget
  layerList->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  layerList->setShowGrid(false);
  layerList->horizontalHeader()->hide();
  layerList->verticalHeader()->hide();
  layerList->setMouseTracking(true);// Important
  layerList->setColumnWidth(0, MM::MAPPING_LIST_HIDE_COLUMN);
  layerList->setColumnWidth(1, MM::MAPPING_LIST_NAME_COLUMN);
  layerList->setColumnWidth(2, MM::MAPPING_LIST_BUTTONS_COLUMN);

  // Create property panel.
  layerPropertyPanel = new QStackedWidget;
  layerPropertyPanel->setDisabled(true);
  layerPropertyPanel->setMinimumHeight(MAPPING_PROPERTY_PANEL_MINIMUM_HEIGHT);

  // Create canvases.
  sourceCanvas = new MapperGLCanvas(this, false);
  sourceCanvas->setFocusPolicy(Qt::ClickFocus);
  sourceCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sourceCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);

  sourceCanvasToolbar = new MapperGLCanvasToolbar(sourceCanvas);
  sourceCanvasToolbar->setToolbarTitle(tr("Input Editor"));
  QVBoxLayout* sourceLayout = new QVBoxLayout;
  sourceLayout->setContentsMargins(0, 0, 0, 0);
  sourceLayout->setSpacing(0);
  sourcePanel = new QWidget(this);

  sourceLayout->addWidget(sourceCanvas);
  sourceLayout->addWidget(sourceCanvasToolbar, 0, Qt::AlignRight);
  sourcePanel->setLayout(sourceLayout);

  destinationCanvas = new MapperGLCanvas(this, true, nullptr, qobject_cast<QOpenGLWidget*>(sourceCanvas->viewport()));
  destinationCanvas->setFocusPolicy(Qt::ClickFocus);
  destinationCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  destinationCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);

  destinationCanvasToolbar = new MapperGLCanvasToolbar(destinationCanvas);
  destinationCanvasToolbar->setToolbarTitle(tr("Output Editor"));
  QVBoxLayout* destinationLayout = new QVBoxLayout;
  destinationLayout->setContentsMargins(0, 0, 0, 0);
  destinationLayout->setSpacing(0);
  destinationPanel = new QWidget(this);

  destinationLayout->addWidget(destinationCanvas);
  destinationLayout->addWidget(destinationCanvasToolbar, 0, Qt::AlignRight);
  destinationPanel->setLayout(destinationLayout);

  // Preferences dialog
  _preferenceDialog = new PreferenceDialog(this);

  outputWindow = new OutputGLWindow(this, destinationCanvas);
  outputWindow->installEventFilter(destinationCanvas);

  // Source scene changed -> change destination.
  connect(sourceCanvas->scene(), SIGNAL(changed(const QList<QRectF>&)),
          destinationCanvas,     SLOT(update()));

  // Destination scene changed -> change output window.
  connect(destinationCanvas->scene(), SIGNAL(changed(const QList<QRectF>&)),
          outputWindow->getCanvas(),  SLOT(update()));

  // Output changed -> change destinatioin
  // XXX si je decommente cette ligne alors quand je clique sur ajouter media ca gele...
  //  connect(outputWindow->getCanvas()->scene(), SIGNAL(changed(const QList<QRectF>&)),
  //          destinationCanvas,                  SLOT(updateCanvas()));

  // Create console logging output
  consoleWindow = ConsoleWindow::console();
  consoleWindow->setVisible(false);
  // Create shortcut window
  _shortcutWindow = new ShortcutWindow;
  _shortcutWindow->setVisible(false);

  // Create layout.
  sourceSplitter = new QSplitter(Qt::Vertical);
  sourceSplitter->setChildrenCollapsible(false);
  sourceSplitter->addWidget(sourceList);
  sourceSplitter->addWidget(sourcePropertyPanel);

  layerSplitter = new QSplitter(Qt::Vertical);
  layerSplitter->setChildrenCollapsible(false);
  layerSplitter->addWidget(layerList);
  layerSplitter->addWidget(layerPropertyPanel);

  // Content tab.
  contentTab = new QTabWidget;
  contentTab->addTab(sourceSplitter, QIcon(":/add-video"), tr("Library"));
  contentTab->addTab(layerSplitter, QIcon(":/add-mesh"), tr("Layers"));

  canvasSplitter = new QSplitter(Qt::Vertical);
  canvasSplitter->addWidget(sourcePanel);
  canvasSplitter->addWidget(destinationPanel);

  mainSplitter = new QSplitter(Qt::Horizontal);
  mainSplitter->addWidget(canvasSplitter);
  mainSplitter->addWidget(contentTab);
  connect(mainSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(updateLayerListColumnWidth()));

  // Initialize size to 9:1 proportions.
  QSize sz = mainSplitter->size();
  QList<int> sizes;
  sizes.append(sz.width() * 0.9);
  sizes.append(sz.width() - sizes.at(0));
  mainSplitter->setSizes(sizes);

  // Upon resizing window, give some extra stretch expansion to canvasSplitter.
  mainSplitter->setStretchFactor(0, 1);

  // Final setups.
  setWindowTitle(tr("MapMap"));
  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
  setCentralWidget(mainSplitter);

  // Connect mapping and source lists signals and slots.
  connectProjectWidgets();

  // Reset focus on main window.
  setFocus();
}

void MainWindow::createActions()
{
  // New.
  newAction = new QAction(tr("&New"), this);
  newAction->setIcon(QIcon(":/new"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setToolTip(tr("Create a new project"));
  newAction->setIconVisibleInMenu(false);
  newAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(newAction);
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  // Open.
  openAction = new QAction(tr("&Open..."), this);
  openAction->setIcon(QIcon(":/open"));
  openAction->setShortcut(QKeySequence::Open);
  openAction->setToolTip(tr("Open an existing project"));
  openAction->setIconVisibleInMenu(false);
  openAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(openAction);
  connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

  // Save.
  saveAction = new QAction(tr("&Save"), this);
  saveAction->setIcon(QIcon(":/save"));
  saveAction->setShortcut(QKeySequence::Save);
  saveAction->setToolTip(tr("Save the project"));
  saveAction->setIconVisibleInMenu(false);
  saveAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(saveAction);
  connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

  // Save as.
  saveAsAction = new QAction(tr("Save &As..."), this);
  saveAsAction->setIcon(QIcon(":/save-as"));
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  saveAsAction->setToolTip(tr("Save the project as..."));
  saveAsAction->setIconVisibleInMenu(false);
  saveAsAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(saveAsAction);
  connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

  // Recents file
  for (int i = 0; i < MaxRecentFiles; i++)
  {
    recentFileActions[i] = new QAction(this);
    recentFileActions[i]->setVisible(false);
    connect(recentFileActions[i], SIGNAL(triggered()),
            this, SLOT(openRecentFile()));
  }

  // Recent video
  for (int i = 0; i < MaxRecentVideo; i++)
  {
    recentVideoActions[i] = new QAction(this);
    recentVideoActions[i]->setVisible(false);
    connect(recentVideoActions[i], SIGNAL(triggered()), this, SLOT(openRecentVideo()));
  }

  // Clear recent video list action
  clearRecentFileActions = new QAction(this);
  clearRecentFileActions->setVisible(true);
  connect(clearRecentFileActions, SIGNAL(triggered()), this, SLOT(clearRecentFileList()));

  // Empty list of recent video action
  emptyRecentVideos = new QAction(tr("No Recents Videos"), this);
  emptyRecentVideos->setEnabled(false);


  // Import Media.
  importMediaAction = new QAction(tr("&Import Media File..."), this);
  importMediaAction->setShortcut(Qt::CTRL | Qt::Key_I);
  importMediaAction->setIcon(QIcon(":/add-video"));
  importMediaAction->setToolTip(tr("Import a video or image file..."));
  importMediaAction->setIconVisibleInMenu(false);
  importMediaAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(importMediaAction);
  connect(importMediaAction, SIGNAL(triggered()), this, SLOT(importMedia()));

  // Open camera.
  AddCameraAction = new QAction(tr("Open &Camera Device..."), this);
  AddCameraAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_C);
  AddCameraAction->setIcon(QIcon(":/add-camera"));
  AddCameraAction->setIconVisibleInMenu(false);
  AddCameraAction->setToolTip(tr("Choose your camera device..."));
  AddCameraAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(AddCameraAction);
  connect(AddCameraAction, SIGNAL(triggered()), this, SLOT(openCameraDevice()));

  // Add color.
  addColorAction = new QAction(tr("Add &Color Source..."), this);
  addColorAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
  addColorAction->setIcon(QIcon(":/add-color"));
  addColorAction->setToolTip(tr("Add a color source..."));
  addColorAction->setIconVisibleInMenu(false);
  addColorAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addColorAction);
  connect(addColorAction, SIGNAL(triggered()), this, SLOT(addColor()));

  // Exit/quit.
  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(QKeySequence::Quit);
  exitAction->setToolTip(tr("Exit the application"));
  exitAction->setIconVisibleInMenu(false);
  exitAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(exitAction);
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

  // Undo action
  undoAction = undoStack->createUndoAction(this, tr("&Undo"));
  undoAction->setShortcut(QKeySequence::Undo);
  undoAction->setIconVisibleInMenu(false);
  undoAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(undoAction);

  //Redo action
  redoAction = undoStack->createRedoAction(this, tr("&Redo"));
  redoAction->setShortcut(QKeySequence::Redo);
  redoAction->setIconVisibleInMenu(false);
  redoAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(redoAction);

  // About.
  aboutAction = new QAction(tr("&About MapMap"), this);
  aboutAction->setToolTip(tr("Show the application's About box"));
  aboutAction->setIconVisibleInMenu(false);
  aboutAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(aboutAction);
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

  // Duplicate.
  duplicateLayerAction = new QAction(tr("Duplicate Layer"), this);
  duplicateLayerAction->setShortcut(Qt::CTRL | Qt::Key_D);
  duplicateLayerAction->setToolTip(tr("Duplicate layer item"));
  duplicateLayerAction->setIconVisibleInMenu(false);
  duplicateLayerAction->setEnabled(false);
  duplicateLayerAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(duplicateLayerAction);
  connect(duplicateLayerAction, SIGNAL(triggered()), this, SLOT(duplicateLayerItem()));

  // Delete mapping.
  deleteLayerAction = new QAction(tr("Delete Layer"), this);
  deleteLayerAction->setShortcut(QKeySequence::Delete);
  deleteLayerAction->setToolTip(tr("Delete layer item"));
  deleteLayerAction->setIconVisibleInMenu(false);
  deleteLayerAction->setEnabled(false);
  deleteLayerAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(deleteLayerAction);
  connect(deleteLayerAction, SIGNAL(triggered()), this, SLOT(deleteLayerItem()));

  // Rename mapping.
  renameLayerAction = new QAction(tr("Rename Layer"), this);
  renameLayerAction->setShortcut(Qt::Key_F2);
  renameLayerAction->setToolTip(tr("Rename layer item"));
  renameLayerAction->setIconVisibleInMenu(false);
  renameLayerAction->setEnabled(false);
  renameLayerAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(renameLayerAction);
  connect(renameLayerAction, SIGNAL(triggered()), this, SLOT(renameLayerItem()));

  // Lock mapping.
  layerLockedAction = new QAction(tr("Lock Layer"), this);
  layerLockedAction->setToolTip(tr("Lock layer item"));
  layerLockedAction->setIconVisibleInMenu(false);
  layerLockedAction->setCheckable(true);
  layerLockedAction->setChecked(false);
  layerLockedAction->setEnabled(false);
  layerLockedAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(layerLockedAction);
  connect(layerLockedAction, SIGNAL(triggered(bool)), this, SLOT(setLayerItemLocked(bool)));

  // Hide mapping.
  layerHideAction = new QAction(tr("Hide Layer"), this);
  layerHideAction->setToolTip(tr("Hide layer item"));
  layerHideAction->setIconVisibleInMenu(false);
  layerHideAction->setCheckable(true);
  layerHideAction->setChecked(false);
  layerHideAction->setEnabled(false);
  layerHideAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(layerHideAction);
  connect(layerHideAction, SIGNAL(triggered(bool)), this, SLOT(setLayerItemHide(bool)));

  // Solo mapping.
  layerSoloAction = new QAction(tr("Solo Layer"), this);
  layerSoloAction->setToolTip(tr("Solo layer item"));
  layerSoloAction->setIconVisibleInMenu(false);
  layerSoloAction->setCheckable(true);
  layerSoloAction->setChecked(false);
  layerSoloAction->setEnabled(false);
  layerSoloAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(layerSoloAction);
  connect(layerSoloAction, SIGNAL(triggered(bool)), this, SLOT(setLayerItemSolo(bool)));

  // Rotate 90 degrees CW action.
  layerRotate90CWAction = new QAction(tr("Rotate 90° CW"), this);
  layerRotate90CWAction->setToolTip(tr("Rotate 90° CW"));
  layerRotate90CWAction->setIconVisibleInMenu(true);
  layerRotate90CWAction->setEnabled(false);
  addAction(layerRotate90CWAction);
  connect(layerRotate90CWAction, SIGNAL(triggered()), SLOT(transformActionLayerItem()));

  // Rotate 90 degrees CW action.
  layerRotate90CCWAction = new QAction(tr("Rotate 90° CW"), this);
  layerRotate90CCWAction->setToolTip(tr("Rotate 90° CW"));
  layerRotate90CCWAction->setIconVisibleInMenu(true);
  layerRotate90CCWAction->setEnabled(false);
  addAction(layerRotate90CCWAction);
  connect(layerRotate90CCWAction, SIGNAL(triggered()), SLOT(transformActionLayerItem()));

  // Rotate 180 degrees action.
  layerRotate180Action = new QAction(tr("Rotate 180°"), this);
  layerRotate180Action->setToolTip(tr("Rotate 180°"));
  layerRotate180Action->setIconVisibleInMenu(true);
  layerRotate180Action->setEnabled(false);
  addAction(layerRotate180Action);
  connect(layerRotate180Action, SIGNAL(triggered()), SLOT(transformActionLayerItem()));

  // Horizontal Flip Action
  layerHorizontalFlipAction = new QAction(tr("Flip Horizontally"), this);
  layerHorizontalFlipAction->setShortcut(Qt::Key_H);
  layerHorizontalFlipAction->setToolTip(tr("Flip Horizontally"));
  layerHorizontalFlipAction->setIconVisibleInMenu(true);
  layerHorizontalFlipAction->setEnabled(false);
  addAction(layerHorizontalFlipAction);
  connect(layerHorizontalFlipAction, SIGNAL(triggered()), SLOT(transformActionLayerItem()));

  // Vertical Flip Action
  layerVerticalFlipAction = new QAction(tr("Flip Vertically"), this);
  layerVerticalFlipAction->setShortcut(Qt::Key_V);
  layerVerticalFlipAction->setToolTip(tr("Flip Vertically"));
  layerVerticalFlipAction->setIconVisibleInMenu(true);
  layerVerticalFlipAction->setEnabled(false);
  addAction(layerVerticalFlipAction);
  connect(layerVerticalFlipAction, SIGNAL(triggered()), SLOT(transformActionLayerItem()));

  layerRaiseAction = new QAction(tr("Raise"), this);
  layerRaiseAction->setShortcut(Qt::Key_PageUp);
  layerRaiseAction->setToolTip(tr("Raise"));
  layerRaiseAction->setIconVisibleInMenu(true);
  layerRaiseAction->setEnabled(false);
  addAction(layerRaiseAction);
  connect(layerRaiseAction, SIGNAL(triggered()), SLOT(reorderLayerItem()));

  layerLowerAction = new QAction(tr("Lower"), this);
  layerLowerAction->setShortcut(Qt::Key_PageDown);
  layerLowerAction->setToolTip(tr("Lower"));
  layerLowerAction->setIconVisibleInMenu(true);
  layerLowerAction->setEnabled(false);
  addAction(layerLowerAction);
  connect(layerLowerAction, SIGNAL(triggered()), SLOT(reorderLayerItem()));

  layerRaiseToTopAction = new QAction(tr("Raise to Top"), this);
  layerRaiseToTopAction->setShortcut(Qt::Key_Home); // bottom = end
  layerRaiseToTopAction->setToolTip(tr("Raise to top"));
  layerRaiseToTopAction->setIconVisibleInMenu(true);
  layerRaiseToTopAction->setEnabled(false);
  addAction(layerRaiseToTopAction);
  connect(layerRaiseToTopAction, SIGNAL(triggered()), SLOT(reorderLayerItem()));

  layerLowerToBottomAction = new QAction(tr("Lower to Bottom"), this);
  layerLowerToBottomAction->setShortcut(Qt::Key_End);
  layerLowerToBottomAction->setToolTip(tr("Lower to bottom"));
  layerLowerToBottomAction->setIconVisibleInMenu(true);
  layerLowerToBottomAction->setEnabled(false);
  addAction(layerLowerToBottomAction);
  connect(layerLowerToBottomAction, SIGNAL(triggered()), SLOT(reorderLayerItem()));

  // Delete source.
  deleteSourceAction = new QAction(tr("Delete Source"), this);
  //deleteSourceAction->setShortcut(tr("CTRL+DEL"));
  deleteSourceAction->setToolTip(tr("Delete source item"));
  deleteSourceAction->setIconVisibleInMenu(false);
  deleteSourceAction->setEnabled(false);
  deleteSourceAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(deleteSourceAction);
  connect(deleteSourceAction, SIGNAL(triggered()), this, SLOT(deleteSourceItem()));

  // Rename source.
  renameSourceAction = new QAction(tr("Rename Source"), this);
  //renameSourceAction->setShortcut(Qt::Key_F2);
  renameSourceAction->setToolTip(tr("Rename source item"));
  renameSourceAction->setIconVisibleInMenu(false);
  renameSourceAction->setEnabled(false);
  renameSourceAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(renameSourceAction);
  connect(renameSourceAction, SIGNAL(triggered()), this, SLOT(renameSourceItem()));

  // Import a new media for current layer
  _importLayerMediaAction = new QAction(tr("Import New Media"), this);
  _importLayerMediaAction->setToolTip(tr("Import new media file if not exists on the list"));
  _importLayerMediaAction->setIconVisibleInMenu(false);
  _importLayerMediaAction->setData("import-new-media"); // Important
  connect(_importLayerMediaAction, SIGNAL(triggered()), this, SLOT(loadLayerMedia()));

  // Preferences...
  preferencesAction = new QAction(tr("&Preferences..."), this);
  //preferencesAction->setIcon(QIcon(":/preferences"));
  preferencesAction->setShortcut(Qt::CTRL | Qt::Key_Comma);
  preferencesAction->setToolTip(tr("Configure preferences..."));
  //preferencesAction->setIconVisibleInMenu(false);
  preferencesAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(preferencesAction);
  connect(preferencesAction, SIGNAL(triggered()), _preferenceDialog, SLOT(exec()));

  // Add mesh.
  addMeshAction = new QAction(tr("Add &Mesh Layer"), this);
  addMeshAction->setShortcut(Qt::CTRL | Qt::Key_M);
  addMeshAction->setIcon(QIcon(":/add-mesh"));
  addMeshAction->setToolTip(tr("Add mesh layer"));
  addMeshAction->setIconVisibleInMenu(false);
  addMeshAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addMeshAction);
  connect(addMeshAction, SIGNAL(triggered()), this, SLOT(addMesh()));
  addMeshAction->setEnabled(false);

  // Add triangle.
  addTriangleAction = new QAction(tr("Add &Triangle Layer"), this);
  addTriangleAction->setShortcut(Qt::CTRL | Qt::Key_T);
  addTriangleAction->setIcon(QIcon(":/add-triangle"));
  addTriangleAction->setToolTip(tr("Add triangle layer"));
  addTriangleAction->setIconVisibleInMenu(false);
  addTriangleAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addTriangleAction);
  connect(addTriangleAction, SIGNAL(triggered()), this, SLOT(addTriangle()));
  addTriangleAction->setEnabled(false);

  // Add ellipse.
  addEllipseAction = new QAction(tr("Add &Ellipse Layer"), this);
  addEllipseAction->setShortcut(Qt::CTRL | Qt::Key_E);
  addEllipseAction->setIcon(QIcon(":/add-ellipse"));
  addEllipseAction->setToolTip(tr("Add ellipse layer"));
  addEllipseAction->setIconVisibleInMenu(false);
  addEllipseAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addEllipseAction);
  connect(addEllipseAction, SIGNAL(triggered()), this, SLOT(addEllipse()));
  addEllipseAction->setEnabled(false);

  // Play.
  const QKeySequence PLAY_PAUSE_KEY_SEQUENCE = Qt::CTRL | Qt::SHIFT | Qt::Key_P;
  playAction = new QAction(tr("Play"), this);
  playAction->setShortcut(PLAY_PAUSE_KEY_SEQUENCE);
  playAction->setIcon(QIcon(":/play"));
  playAction->setToolTip(tr("Play"));
  playAction->setIconVisibleInMenu(false);
  playAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(playAction);
  connect(playAction, SIGNAL(triggered()), this, SLOT(play()));
  playAction->setVisible(true);

  // Pause.
  pauseAction = new QAction(tr("Pause"), this);
  pauseAction->setShortcut(PLAY_PAUSE_KEY_SEQUENCE);
  pauseAction->setIcon(QIcon(":/pause"));
  pauseAction->setToolTip(tr("Pause"));
  pauseAction->setIconVisibleInMenu(false);
  pauseAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(pauseAction);
  connect(pauseAction, SIGNAL(triggered()), this, SLOT(pause()));
  pauseAction->setVisible(false);

  // Rewind.
  rewindAction = new QAction(tr("Restart"), this);
  rewindAction->setShortcut(Qt::CTRL | Qt::Key_R);
  rewindAction->setIcon(QIcon(":/rewind"));
  rewindAction->setToolTip(tr("Restart"));
  rewindAction->setIconVisibleInMenu(false);
  rewindAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(rewindAction);
  connect(rewindAction, SIGNAL(triggered()), this, SLOT(rewind()));

  // Toggle display of output window.
  outputFullScreenAction = new QAction(tr("Toggle &Fullscreen"), this);
  outputFullScreenAction->setShortcut(Qt::CTRL | Qt::Key_F);
  outputFullScreenAction->setIcon(QIcon(":/fullscreen"));
  outputFullScreenAction->setToolTip(tr("Toggle Fullscreen"));
  outputFullScreenAction->setIconVisibleInMenu(false);
  outputFullScreenAction->setCheckable(true);
  // Don't be displayed by default
  outputFullScreenAction->setChecked(false);
  outputFullScreenAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(outputFullScreenAction);
  // Manage fullscreen/modal show of GL output window.
  connect(outputFullScreenAction, SIGNAL(toggled(bool)), outputWindow, SLOT(setFullScreen(bool)));
  connect(qApp, &QGuiApplication::screenAdded,   this, [this](QScreen*){ updateScreenCount(); });
  connect(qApp, &QGuiApplication::screenRemoved,  this, [this](QScreen*){ updateScreenCount(); });
  // Create hiden action for closing output window
  QAction *closeOutput = new QAction(this);
  closeOutput->setShortcut(Qt::Key_Escape);
  closeOutput->setShortcutContext(Qt::ApplicationShortcut);
  addAction(closeOutput);
  connect(closeOutput, SIGNAL(triggered(bool)), this, SLOT(exitFullScreen()));

  // Toggle display of canvas controls.
  displayControlsAction = new QAction(tr("&Display Controls"), this);
  displayControlsAction->setShortcut(Qt::ALT | Qt::Key_C);
  displayControlsAction->setIcon(QIcon(":/control-points"));
  displayControlsAction->setToolTip(tr("Display canvas controls"));
  displayControlsAction->setIconVisibleInMenu(false);
  displayControlsAction->setCheckable(true);
  displayControlsAction->setChecked(_displayControls);
  displayControlsAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayControlsAction);
  // Manage show/hide of canvas controls.
  connect(displayControlsAction, SIGNAL(toggled(bool)), this, SLOT(enableDisplayControls(bool)));
  connect(displayControlsAction, SIGNAL(toggled(bool)), outputWindow, SLOT(setCanvasDisplayCrosshair(bool)));

  // Toggle display of canvas controls.
  displaySourceControlsAction = new QAction(tr("&Display Controls of Layers of a Source"), this);
  //displaySourceControlsAction->setShortcut(Qt::ALT | Qt::Key_C);
  displaySourceControlsAction->setIcon(QIcon(":/control-points"));
  displaySourceControlsAction->setToolTip(tr("Display all canvas controls related to current source"));
  displaySourceControlsAction->setIconVisibleInMenu(false);
  displaySourceControlsAction->setCheckable(true);
  displaySourceControlsAction->setChecked(_displaySourceControls);
  displaySourceControlsAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displaySourceControlsAction);
  // Manage show/hide of canvas controls.
  connect(displaySourceControlsAction, SIGNAL(toggled(bool)), this, SLOT(enableDisplaySourceControls(bool)));
//  connect(displaySourceControlsAction, SIGNAL(toggled(bool)), outputWindow, SLOT(setDisplayCrosshair(bool)));
  connect(displayControlsAction, SIGNAL(toggled(bool)), displaySourceControlsAction, SLOT(setEnabled(bool)));

  // Toggle sticky vertices
  stickyVerticesAction = new QAction(tr("&Sticky Vertices"), this);
  stickyVerticesAction->setShortcut(Qt::ALT | Qt::Key_S);
  stickyVerticesAction->setIcon(QIcon(":/control-points"));
  stickyVerticesAction->setToolTip(tr("Enable sticky vertices"));
  stickyVerticesAction->setIconVisibleInMenu(false);
  stickyVerticesAction->setCheckable(true);
  stickyVerticesAction->setChecked(_stickyVertices);
  stickyVerticesAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(stickyVerticesAction);
  // Manage sticky vertices
  connect(stickyVerticesAction, SIGNAL(toggled(bool)), this, SLOT(enableStickyVertices(bool)));

  displayTestSignalAction = new QAction(tr("Show &Test Signal"), this);
  displayTestSignalAction->setShortcut(Qt::ALT | Qt::Key_T);
  displayTestSignalAction->setIcon(QIcon(":/toggle-test-signal"));
  displayTestSignalAction->setToolTip(tr("Show Test signal"));
  displayTestSignalAction->setIconVisibleInMenu(false);
  displayTestSignalAction->setCheckable(true);
  displayTestSignalAction->setChecked(false);
  displayTestSignalAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayTestSignalAction);
  // Manage show/hide of test signal
  connect(displayTestSignalAction, SIGNAL(toggled(bool)), outputWindow, SLOT(setDisplayTestSignal(bool)));
//  connect(displayTestSignalAction, SIGNAL(toggled(bool)), this, SLOT(update()));

  // Toggle display of Undo History
  displayUndoHistoryAction = new QAction(tr("Display &Undo History"), this);
  displayUndoHistoryAction->setShortcut(Qt::ALT | Qt::Key_U);
  displayUndoHistoryAction->setCheckable(true);
  displayUndoHistoryAction->setChecked(_displayUndoStack);
  displayUndoHistoryAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayUndoHistoryAction);
  // Manage show/hide of Undo History
  connect(displayUndoHistoryAction, SIGNAL(toggled(bool)), this, SLOT(displayUndoHistory(bool)));

  // Toggle display of Console output
  openConsoleAction = new QAction(tr("Open Conso&le"), this);
  openConsoleAction->setShortcut(Qt::ALT | Qt::Key_L);
  openConsoleAction->setCheckable(true);
  openConsoleAction->setChecked(false);
  openConsoleAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(openConsoleAction);
  connect(openConsoleAction, SIGNAL(toggled(bool)), consoleWindow, SLOT(setVisible(bool)));
  // uncheck action when window is closed
  connect(consoleWindow, SIGNAL(windowClosed()), openConsoleAction, SLOT(toggle()));

  // Toggle display of zoom tool buttons
  displayZoomToolAction = new QAction(tr("Display &Zoom Toolbar"), this);
  displayZoomToolAction->setShortcut(Qt::ALT | Qt::Key_Z);
  displayZoomToolAction->setCheckable(true);
  displayZoomToolAction->setChecked(true);
  displayZoomToolAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayZoomToolAction);
  connect(displayZoomToolAction, SIGNAL(toggled(bool)), sourceCanvasToolbar, SLOT(showZoomToolBar(bool)));
  connect(displayZoomToolAction, SIGNAL(toggled(bool)), destinationCanvasToolbar, SLOT(showZoomToolBar(bool)));

  // Toggle show/hide menuBar
  showMenuBarAction = new QAction(tr("&Menu Bar"), this);
  showMenuBarAction->setCheckable(true);
  showMenuBarAction->setChecked(_showMenuBar);
  connect(showMenuBarAction, SIGNAL(toggled(bool)), this, SLOT(showMenuBar(bool)));

  // Perspectives
  // Main perspective (Source + destination)
  mainViewAction = new QAction(tr("Main Layout"), this);
  mainViewAction->setCheckable(true);
  mainViewAction->setChecked(true);
  mainViewAction->setShortcut(Qt::CTRL | Qt::Key_1);
  mainViewAction->setToolTip(tr("Switch to the Main layout."));
  connect(mainViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(0), SLOT(setVisible(bool)));
  connect(mainViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(1), SLOT(setVisible(bool)));
  // Source Only
  sourceViewAction = new QAction(tr("Input editor Layout"), this);
  sourceViewAction->setCheckable(true);
  sourceViewAction->setShortcut(Qt::CTRL | Qt::Key_2);
  sourceViewAction->setToolTip(tr("Switch to the Input editor Layout."));
  connect(sourceViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(0), SLOT(setVisible(bool)));
  connect(sourceViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(1), SLOT(setHidden(bool)));
  // Destination Only
  destViewAction = new QAction(tr("Output Editor Layout"), this);
  destViewAction->setCheckable(true);
  destViewAction->setShortcut(Qt::CTRL | Qt::Key_3);
  destViewAction->setToolTip(tr("Switch to the Output Editors Layout."));
  connect(destViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(0), SLOT(setHidden(bool)));
  connect(destViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(1), SLOT(setVisible(bool)));
  // Groups all actions
  perspectiveActionGroup = new QActionGroup(this);
  perspectiveActionGroup->addAction(mainViewAction);
  perspectiveActionGroup->addAction(sourceViewAction);
  perspectiveActionGroup->addAction(destViewAction);

  //Zoom toolbar
  // Zoom In
  zoomInAction = new QAction(tr("Zoom In"), this);
  zoomInAction->setShortcut(QKeySequence::ZoomIn);
  zoomInAction->setToolTip(tr("Zoom In"));
  zoomInAction->setEnabled(false);
  connect(zoomInAction, SIGNAL(triggered()), sourceCanvas, SLOT(increaseZoomLevel()));
  connect(zoomInAction, SIGNAL(triggered()), destinationCanvas, SLOT(increaseZoomLevel()));
  // Zoom Out
  zoomOutAction = new QAction(tr("Zoom Out"), this);
  zoomOutAction->setShortcut(QKeySequence::ZoomOut);
  zoomOutAction->setToolTip(tr("Zoom Out"));
  zoomOutAction->setEnabled(false);
  connect(zoomOutAction, SIGNAL(triggered()), sourceCanvas, SLOT(decreaseZoomLevel()));
  connect(zoomOutAction, SIGNAL(triggered()), destinationCanvas, SLOT(decreaseZoomLevel()));
  // Reset zoom
  resetZoomAction = new QAction(tr("Original Size"), this);
  resetZoomAction->setShortcut(Qt::CTRL | Qt::Key_0);
  resetZoomAction->setToolTip(tr("Reset zoom to original size"));
  resetZoomAction->setEnabled(false);
  connect(resetZoomAction, SIGNAL(triggered()), sourceCanvas, SLOT(resetZoomLevel()));
  connect(resetZoomAction, SIGNAL(triggered()), destinationCanvas, SLOT(resetZoomLevel()));
  // Fit to view
  fitToViewAction = new QAction(tr("Fit To View"), this);
  fitToViewAction->setToolTip(tr("Fit to viewport"));
  fitToViewAction->setEnabled(false);
  connect(fitToViewAction, SIGNAL(triggered()), sourceCanvas, SLOT(fitShapeToView()));
  connect(fitToViewAction, SIGNAL(triggered()), destinationCanvas, SLOT(fitShapeToView()));

  // Helps
  // Bug report
  bugReportAction = new QAction(tr("Report an issue"), this);
  connect(bugReportAction, SIGNAL(triggered()), this, SLOT(reportBug()));
  // Support
  supportAction = new QAction(tr("Technical support"), this);
  connect(supportAction, SIGNAL(triggered()), this, SLOT(technicalSupport()));
  // Documentation
  docAction = new QAction(tr("Documentation"), this);
  connect(docAction, SIGNAL(triggered()), this, SLOT(documentation()));
  // Send us feedback
  feedbackAction = new QAction(tr("Submit feedback via email"), this);
  connect(feedbackAction, SIGNAL(triggered()), this, SLOT(sendFeedback()));
  // Keyboard shortcuts
  shortcutAction = new QAction(tr("&Keyboard shortcuts"), this);
  shortcutAction->setShortcut(Qt::CTRL | Qt::Key_K);
  connect(shortcutAction, SIGNAL(triggered()), this, SLOT(openShortcutWindow()));

  // All available screen as action
  updateScreenActions();
//  outputScreenMenu->addActions(screenActions);
}

void MainWindow::startFullScreen()
{
  // Remove canvas controls.
  displayControlsAction->setChecked(false);
  // Display output window.
  outputFullScreenAction->setChecked(true);
}

void MainWindow::createMenus()
{
  QMenuBar *menuBar = NULL;

#ifdef __MACOSX_CORE__
  menuBar = new QMenuBar(0);
  //this->setMenuBar(menuBar);
#else
  menuBar = this->menuBar();
#endif

  // File
  fileMenu = menuBar->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openAction);
  fileMenu->addAction(saveAction);
  fileMenu->addAction(saveAsAction);
  fileMenu->addSeparator();
  fileMenu->addAction(importMediaAction);
  fileMenu->addAction(AddCameraAction);
  fileMenu->addAction(addColorAction);

  // Recent file separator
  separatorAction = fileMenu->addSeparator();
  recentFileMenu = fileMenu->addMenu(tr("Open Recents Projects"));
  for (int i = 0; i < MaxRecentFiles; ++i) {
    recentFileMenu->addAction(recentFileActions[i]);
  }
  recentFileMenu->addAction(clearRecentFileActions);

  // Recent import video
  recentVideoMenu = fileMenu->addMenu(tr("Open Recents Videos"));
  recentVideoMenu->addAction(emptyRecentVideos);
  for (int i = 0; i < MaxRecentVideo; ++i) {
    recentVideoMenu->addAction(recentVideoActions[i]);
  }

  // Exit
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  // Edit.
  editMenu = menuBar->addMenu(tr("&Edit"));
  // Undo & Redo menu
  editMenu->addAction(undoAction);
  editMenu->addAction(redoAction);
  editMenu->addSeparator();
  // Source canvas menu
  editMenu->addAction(deleteSourceAction);
  editMenu->addAction(renameSourceAction);
  editMenu->addSeparator();
  // Destination canvas menu
  editMenu->addAction(duplicateLayerAction);
  editMenu->addAction(deleteLayerAction);
  editMenu->addAction(renameLayerAction);
  editMenu->addSeparator();
  editMenu->addAction(layerRaiseAction);
  editMenu->addAction(layerLowerAction);
  editMenu->addAction(layerRaiseToTopAction);
  editMenu->addAction(layerLowerToBottomAction);
  editMenu->addSeparator();
  editMenu->addAction(layerRotate90CWAction);
  editMenu->addAction(layerRotate90CCWAction);
  editMenu->addAction(layerRotate180Action);
  editMenu->addSeparator();
  editMenu->addAction(layerHorizontalFlipAction);
  editMenu->addAction(layerVerticalFlipAction);
  editMenu->addSeparator();

  editMenu->addAction(layerLockedAction);
  editMenu->addAction(layerHideAction);
  editMenu->addAction(layerSoloAction);
  editMenu->addSeparator();

  // Sticky vertices
  editMenu->addAction(stickyVerticesAction);
  editMenu->addSeparator();
  // Preferences
  editMenu->addAction(preferencesAction);

  // View.
  viewMenu = menuBar->addMenu(tr("&View"));

  viewMenu->addAction(zoomInAction);
  viewMenu->addAction(zoomOutAction);
  viewMenu->addAction(resetZoomAction);
  viewMenu->addAction(fitToViewAction);
  viewMenu->addSeparator();
  viewMenu->addAction(outputFullScreenAction);
  viewMenu->addAction(displayTestSignalAction);
  viewMenu->addAction(displayControlsAction);
  viewMenu->addAction(displaySourceControlsAction);
  outputScreenMenu = viewMenu->addMenu(tr("&Output screen"));
  outputScreenMenu->addActions(screenActions);
  viewMenu->addSeparator();
  // Playback.
  viewMenu->addAction(playAction);
  viewMenu->addAction(pauseAction);
  viewMenu->addAction(rewindAction);

  // Window
  windowMenu = menuBar->addMenu(tr("&Window"));

  // Perspectives
  windowMenu->addAction(mainViewAction);
  windowMenu->addAction(sourceViewAction);
  windowMenu->addAction(destViewAction);
  windowMenu->addSeparator();

  // Tools.
  windowMenu->addAction(displayUndoHistoryAction);
  windowMenu->addAction(openConsoleAction);
  windowMenu->addSeparator();

  // Menus/toolbars.
  windowMenu->addAction(displayZoomToolAction);
#ifdef Q_OS_LINUX
  if (QString(getenv("XDG_CURRENT_DESKTOP")).toLower() != "unity")
    windowMenu->addAction(showMenuBarAction);
#endif
#ifdef Q_OS_WIN32
  windowMenu->addAction(showMenuBarAction);
#endif

  // Help.
  helpMenu = menuBar->addMenu(tr("&Help"));
  helpMenu->addAction(docAction);
  helpMenu->addAction(shortcutAction);
  helpMenu->addAction(feedbackAction);
  helpMenu->addAction(supportAction);
  helpMenu->addAction(bugReportAction);
  helpMenu->addSeparator();
  helpMenu->addAction(aboutAction);

  //  helpMenu->addAction(aboutQtAction);
}

void MainWindow::createLayerContextMenu()
{
  // Context menu.
  layerContextMenu = new QMenu(this);
  layerContextMenu->installEventFilter(this);

  // Add different Action
  layerContextMenu->addAction(duplicateLayerAction);
  layerContextMenu->addAction(deleteLayerAction);
  layerContextMenu->addAction(renameLayerAction);
  // Add a little separator
  layerContextMenu->addSeparator();

  // Create menu for source list
  _changeLayerMediaMenu = layerContextMenu->addMenu(tr("Change Layer Source"));

  // Add another separator
  layerContextMenu->addSeparator();
  layerContextMenu->addAction(layerRaiseAction);
  layerContextMenu->addAction(layerLowerAction);
  layerContextMenu->addAction(layerRaiseToTopAction);
  layerContextMenu->addAction(layerLowerToBottomAction);
  layerContextMenu->addSeparator();
  layerContextMenu->addAction(layerRotate90CWAction);
  layerContextMenu->addAction(layerRotate90CCWAction);
  layerContextMenu->addAction(layerRotate180Action);
  layerContextMenu->addSeparator();
  layerContextMenu->addAction(layerHorizontalFlipAction);
  layerContextMenu->addAction(layerVerticalFlipAction);

  layerContextMenu->addSeparator();
  layerContextMenu->addAction(layerLockedAction);
  layerContextMenu->addAction(layerHideAction);
  layerContextMenu->addAction(layerSoloAction);

  // Set context menu policy
  layerList->setContextMenuPolicy(Qt::CustomContextMenu);
  destinationCanvas->setContextMenuPolicy(Qt::CustomContextMenu);
  outputWindow->setContextMenuPolicy(Qt::CustomContextMenu);

  // Context Menu Connexions
  connect(layerItemDelegate, SIGNAL(itemContextMenuRequested(const QPoint&)),
          this, SLOT(showLayerContextMenu(const QPoint&)), Qt::QueuedConnection);
  connect(destinationCanvas, SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showLayerContextMenu(const QPoint&)));
  connect(outputWindow->getCanvas(), SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showLayerContextMenu(const QPoint&)));
}

void MainWindow::createSourceContextMenu()
{
  // Source Context Menu
  sourceContextMenu = new QMenu(this);
  sourceContextMenu->installEventFilter(this);

  // Add Actions
  sourceContextMenu->addAction(deleteSourceAction);
  sourceContextMenu->addAction(renameSourceAction);

  // Define Context policy
  sourceList->setContextMenuPolicy(Qt::CustomContextMenu);
  sourceCanvas->setContextMenuPolicy(Qt::CustomContextMenu);

  // Connexions
  connect(sourceList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showSourceContextMenu(const QPoint&)));
  connect(sourceCanvas, SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showSourceContextMenu(const QPoint&)));
}

void MainWindow::createToolBars()
{
  mainToolBar = addToolBar(tr("&Toolbar"));
  mainToolBar->setMovable(false);
  mainToolBar->addAction(importMediaAction);
  mainToolBar->addAction(AddCameraAction);
  mainToolBar->addAction(addColorAction);

  mainToolBar->addSeparator();

  mainToolBar->addAction(addMeshAction);
  mainToolBar->addAction(addTriangleAction);
  mainToolBar->addAction(addEllipseAction);
  mainToolBar->addSeparator();

  mainToolBar->addAction(outputFullScreenAction);
  mainToolBar->addAction(displayTestSignalAction);

  // XXX: style hack: dummy expanding widget allows the placement of toolbar at the top right
  // From: http://www.qtcentre.org/threads/9102-QToolbar-setContentsMargins
  QWidget* spacer = new QWidget(mainToolBar);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mainToolBar->addWidget(spacer);
  mainToolBar->addAction(playAction);
  mainToolBar->addAction(pauseAction);
  mainToolBar->addAction(rewindAction);

  // Disable toolbar context menu
  mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);

  // Toggle show/hide of toolbar
  showToolBarAction = mainToolBar->toggleViewAction();
  windowMenu->addAction(showToolBarAction);

  // Add toolbars.
  addToolBar(Qt::TopToolBarArea, mainToolBar);

  // XXX: style hack
  mainToolBar->setStyleSheet("border-bottom: solid 5px #272a36;");
}

void MainWindow::createStatusBar()
{
  // Create canvases zoom level statut
  destinationZoomLabel = new QLabel(statusBar());
  destinationZoomLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  destinationZoomLabel->setContentsMargins(2, 0, 0, 0);
  sourceZoomLabel = new QLabel(statusBar());
  sourceZoomLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  sourceZoomLabel->setContentsMargins(2, 0, 0, 0);
  // last action taking statut
  lastActionLabel = new QLabel(statusBar());
  lastActionLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  lastActionLabel->setContentsMargins(2, 0, 0, 0);
  // Standard message
  currentMessageLabel = new QLabel(statusBar());
  currentMessageLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  currentMessageLabel->setContentsMargins(0, 0, 0, 0);
  // Current location of the mouse
  mousePosLabel = new QLabel(statusBar());
  mousePosLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  mousePosLabel->setContentsMargins(2, 0, 0, 0);
  // FPS.
  trueFramesPerSecondsLabel = new QLabel(statusBar());
  trueFramesPerSecondsLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  trueFramesPerSecondsLabel->setContentsMargins(2, 0, 0, 0);

  // Add permanently into the statut bar
  statusBar()->addPermanentWidget(currentMessageLabel, 5);
  statusBar()->addPermanentWidget(lastActionLabel, 4);
  statusBar()->addPermanentWidget(mousePosLabel, 3);
  statusBar()->addPermanentWidget(sourceZoomLabel, 1);
  statusBar()->addPermanentWidget(destinationZoomLabel, 1);
  statusBar()->addPermanentWidget(trueFramesPerSecondsLabel, 1);

  // Update the status bar
  updateStatusBar();
}

void MainWindow::readSettings()
{
  // FIXME: for each setting that is new since the first release in the major version number branch,
  // make sure it exists before reading its value.
  QSettings settings;

  // settings present since 0.1.0:
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray());

  mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
  sourceSplitter->restoreState(settings.value("sourceSplitter").toByteArray());
  layerSplitter->restoreState(settings.value("layerSplitter").toByteArray());
  canvasSplitter->restoreState(settings.value("canvasSplitter").toByteArray());
  outputWindow->restoreGeometry(settings.value("outputWindow").toByteArray());

  // new in 0.1.2:
  outputFullScreenAction->setChecked(settings.value("displayOutputWindow", MM::DISPLAY_OUTPUT_WINDOW).toBool());
  displayTestSignalAction->setChecked(settings.value("displayTestSignal", MM::DISPLAY_TEST_SIGNAL).toBool());
  displayControlsAction->setChecked(settings.value("displayControls", MM::DISPLAY_CONTROLS).toBool());
  outputWindow->setCanvasDisplayCrosshair(settings.value("displayControls", MM::DISPLAY_CONTROLS).toBool());
  oscListeningPort = settings.value("oscListeningPort", MM::DEFAULT_OSC_PORT).toInt();

  // Update Recent files and video
  updateRecentFileActions();
  updateRecentVideoActions();

  // new in 0.3.2
  displayUndoHistoryAction->setChecked(settings.value("displayUndoStack", MM::DISPLAY_UNDO_HISTORY).toBool());
  displayZoomToolAction->setChecked(settings.value("zoomToolBar", MM::DISPLAY_ZOOM_TOOLBAR).toBool());
  showMenuBarAction->setChecked(settings.value("showMenuBar", MM::DISPLAY_MENU_BAR).toBool());

  // New in 0.4.1
   displaySourceControlsAction->setChecked(settings.value("displayAllControls", MM::DISPLAY_ALL_CONTROLS).toBool());
   stickyVerticesAction->setChecked(settings.value("stickyVertices", MM::STICKY_VERTICES).toBool());
   // Set toolbar icon size
   int toolBarIconSize = settings.value("toolbarIconSize", MM::TOOLBAR_ICON_SIZE).toInt();
   mainToolBar->setIconSize(QSize(toolBarIconSize, toolBarIconSize));
}

void MainWindow::writeSettings()
{
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  settings.setValue("mainSplitter", mainSplitter->saveState());
  settings.setValue("sourceSplitter", sourceSplitter->saveState());
  settings.setValue("layerSplitter", layerSplitter->saveState());
  settings.setValue("canvasSplitter", canvasSplitter->saveState());
  settings.setValue("outputWindow", outputWindow->saveGeometry());
  settings.setValue("displayOutputWindow", outputFullScreenAction->isChecked());
  settings.setValue("displayTestSignal", displayTestSignalAction->isChecked());
  settings.setValue("displayControls", displayControlsAction->isChecked());
  settings.setValue("displayAllControls", displaySourceControlsAction->isChecked());
  settings.setValue("oscListeningPort", oscListeningPort);
  settings.setValue("displayUndoStack", displayUndoHistoryAction->isChecked());
  settings.setValue("zoomToolBar", displayZoomToolAction->isChecked());
  settings.setValue("showMenuBar", showMenuBarAction->isChecked());
  settings.setValue("stickyVertices", stickyVerticesAction->isChecked());
}

bool MainWindow::okToContinue()
{
  if (isWindowModified())
  {
    int r = QMessageBox::warning(this, tr("MapMap"),
                                 tr("The document has been modified.\n"
                                    "Do you want to save your changes?"),
                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (r == QMessageBox::Yes)
    {
      return save();
    }
    else if (r == QMessageBox::Cancel)
    {
      return false;
    }
  }
  return true;
}

bool MainWindow::loadFile(const QString &fileName)
{
  QFile file(fileName);
  QDir currentDir;

  if (! file.open(QFile::ReadOnly | QFile::Text))
  {
    QMessageBox::warning(this, tr("Error reading mapping project file"),
                         tr("Cannot read file %1:\n%2.")
                         .arg(fileName)
                         .arg(file.errorString()));
    return false;
  }

  // Clear current project.
  clearProject();

  // Read new project
  ProjectReader reader(this);
  if (! reader.readFile(&file))
  {
    QMessageBox::warning(this, tr("Error reading mapping project file"),
                         tr("Parse error in file %1:\n\n%2")
                         .arg(fileName)
                         .arg(reader.errorString()));
  }
  else
  {
    settings.setValue("defaultProjectDir", currentDir.absoluteFilePath(fileName));
    statusBar()->showMessage(tr("File loaded"), 2000);
    setCurrentFile(fileName);
  }

  return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
  QFile file(fileName);
  if (! file.open(QFile::WriteOnly | QFile::Text))
  {
    QMessageBox::warning(this, tr("Error saving mapping project"),
                         tr("Cannot write file %1:\n%2.")
                         .arg(fileName)
                         .arg(file.errorString()));
    return false;
  }

  ProjectWriter writer(this);
  if (writer.writeFile(&file))
  {
    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
  }
  else
    return false;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
  curFile = fileName;
  setWindowModified(false);

  QString shownName = tr("Untitled");
  if (!curFile.isEmpty())
  {
    shownName = strippedName(curFile);
    recentFiles = settings.value("recentFiles").toStringList();
    recentFiles.removeAll(curFile);
    recentFiles.prepend(curFile);
    while (recentFiles.size() > MaxRecentFiles)
    {
      recentFiles.removeLast();
    }
    settings.setValue("recentFiles", recentFiles);
    updateRecentFileActions();
  }

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("MapMap Project")));
}

void MainWindow::setCurrentVideo(const QString &fileName)
{
  curVideo = fileName;

  recentVideos = settings.value("recentVideos").toStringList();
  recentVideos.removeAll(curVideo);
  recentVideos.prepend(curVideo);
  while (recentVideos.size() > MaxRecentVideo)
    recentVideos.removeLast();
  settings.setValue("recentVideos", recentVideos);
  updateRecentVideoActions();
}

void MainWindow::updateRecentFileActions()
{
  recentFiles = settings.value("recentFiles").toStringList();
  int numRecentFiles = qMin(recentFiles.size(), int(MaxRecentFiles));

  for (int j = 0; j < numRecentFiles; ++j)
  {
    QString text = tr("&%1 %2")
        .arg(j + 1)
        .arg(strippedName(recentFiles[j]));
    recentFileActions[j]->setText(text);
    recentFileActions[j]->setData(recentFiles[j]);
    recentFileActions[j]->setVisible(true);
    clearRecentFileActions->setVisible(true);
  }

  for (int i = numRecentFiles; i < MaxRecentFiles; ++i)
  {
    recentFileActions[i]->setVisible(false);
  }

  if (numRecentFiles > 0)
  {
    separatorAction->setVisible(true);
    clearRecentFileActions->setText(tr("Clear List"));
    clearRecentFileActions->setEnabled(true);
  } else {
    clearRecentFileActions->setText(tr("No Recents Projects"));
    clearRecentFileActions->setEnabled(false);
  }
}

void MainWindow::updateRecentVideoActions()
{
  recentVideos = settings.value("recentVideos").toStringList();
  int numRecentVideos = qMin(recentVideos.size(), int(MaxRecentVideo));

  for (int i = 0; i < numRecentVideos; ++i)
  {
    QString text = tr("&%1 %2")
        .arg(i + 1)
        .arg(strippedName(recentVideos[i]));
    recentVideoActions[i]->setText(text);
    recentVideoActions[i]->setData(recentVideos[i]);
    recentVideoActions[i]->setVisible(true);
  }

  for (int j = numRecentVideos; j < MaxRecentVideo; ++j)
    recentVideoActions[j]->setVisible(false);

  if (numRecentVideos >  0)
  {
    emptyRecentVideos->setVisible(false);
  }
}

void MainWindow::updateScreenActions()
{
  // Add new action for each screen
  for (QScreen *screen: QApplication::screens()) {
    QString actionLabel = tr("%1 - %2x%3")
        .arg(screen->name())
        .arg(QString::number(screen->size().width()))
        .arg(QString::number(screen->size().height()));
    if (screen == QApplication::primaryScreen()) {
      actionLabel.append(tr(" - Primary"));
    }
    QAction *action = new QAction(actionLabel, this);
    screenActions.append(action);
    action->setData(screenActions.count() - 1);
  }

  // Configure actions
  screenActionGroup = new QActionGroup(this);
  int preferredScreen = outputWindow->getPreferredScreen();
  for (QAction *action: screenActions) {
    action->setCheckable(true);
    if (action == screenActions.at(preferredScreen)) {
      action->setChecked(true);
    }
    connect(action, SIGNAL(triggered()), this, SLOT(setupOutputScreen()));
    screenActionGroup->addAction(action);
  }
}

void MainWindow::updateMediaListActions()
{
  // Clear media list menu
  _changeLayerMediaMenu->clear();

  if (sourceList->count() > 1) { // No need to load the same video
    for (auto i = 0; i < sourceList->count(); i++) {
      QAction *mediaAction = new QAction(this);
      mediaAction->setText(tr("&%1 %2").arg(i + 1).arg(mappingManager->getSource(i)->getName()));
      mediaAction->setData(mappingManager->getSource(i)->getId());
      mediaAction->setVisible(true);
      connect(mediaAction, SIGNAL(triggered()),
              this, SLOT(loadLayerMedia()));
      // Add new media on action list
      _changeLayerMediaMenu->addAction(mediaAction);
    }
  }
  // Add new media source in case no exists on the list
  _changeLayerMediaMenu->addAction(_importLayerMediaAction);
}

void MainWindow::updateLayerActions()
{
  if (layerListModel->rowCount() < 1) {
    duplicateLayerAction->setEnabled(false);
    deleteLayerAction->setEnabled(false);
    renameLayerAction->setEnabled(false);
    layerLockedAction->setEnabled(false);
    layerHideAction->setEnabled(false);
    layerSoloAction->setEnabled(false);
    layerRotate90CWAction->setEnabled(true);
    layerRotate90CCWAction->setEnabled(true);
    layerRotate180Action->setEnabled(true);
    layerHorizontalFlipAction->setEnabled(false);
    layerVerticalFlipAction->setEnabled(false);
    layerRaiseAction->setEnabled(false);
    layerLowerAction->setEnabled(false);
    layerRaiseToTopAction->setEnabled(false);
    layerLowerToBottomAction->setEnabled(false);
    //Disable zoom menus
    zoomInAction->setEnabled(false);
    zoomOutAction->setEnabled(false);
    resetZoomAction->setEnabled(false);
    fitToViewAction->setEnabled(false);
    // Also disable toobars
    destinationCanvasToolbar->enableZoomToolBar(false);
    sourceCanvasToolbar->enableZoomToolBar(false);
  }
}

void MainWindow::clearRecentFileList()
{
  recentFiles = settings.value("recentFiles").toStringList();

  while (recentFiles.size() > 0)
    recentFiles.clear();

  settings.setValue("recentFiles", recentFiles);
  updateRecentFileActions();
}

// TODO
// bool MainWindow::updateMediaFile(const QString &source_name, const QString &fileName)
// {
// }

bool MainWindow::importMediaFile(const QString &fileName, bool isImage, bool isCamera)
{
  QFile file(fileName);
  QDir currentDir;
  VideoType type = VIDEO_URI;

  if (!fileSupported(fileName, isImage))
    return false;

  if (isCamera) {
    type = VIDEO_WEBCAM;
  }

  if (!isCamera && !file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, tr("MapMap Project"),
                         tr("Cannot read file %1:\n%2.")
                         .arg(file.fileName())
                         .arg(file.errorString()));
    return false;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add media file to model.
  uint mediaId = createMediaSource(NULL_UID, fileName, 0, 0, isImage, type);

  // Initialize position (center).
  QSharedPointer<Video> media = qSharedPointerCast<Video>(mappingManager->getSourceById(mediaId));
  Q_CHECK_PTR(media);

  media->setPosition((sourceCanvas->width()  - media->getWidth() ) / 2.0f,
                     (sourceCanvas->height() - media->getHeight()) / 2.0f );

  QApplication::restoreOverrideCursor();

  if (!isCamera) { // Do not add camera to recents files
    if (!isImage)
    {
      settings.setValue("defaultVideoDir", currentDir.absoluteFilePath(fileName));
      setCurrentVideo(fileName);
    }
    else
    {
      settings.setValue("defaultImageDir", currentDir.absoluteFilePath(fileName));
    }
  }

  statusBar()->showMessage(tr("File imported"), 2000);

  // Update media list
  updateMediaListActions();

  return true;
}

bool MainWindow::addColorSource(const QColor& color)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add color to model.
  uint colorId = createColorSource(NULL_UID, color);

  // Initialize position (center).
  QSharedPointer<Color> colorSource = qSharedPointerCast<Color>(mappingManager->getSourceById(colorId));
  Q_CHECK_PTR(colorSource);

  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("Color source added"), 2000);

  return true;
}

void MainWindow::addSourceItem(uid sourceId, const QIcon& icon, const QString& name)
{
  Source::ptr source = mappingManager->getSourceById(sourceId);
  Q_CHECK_PTR(source);

  // Create source gui.
  SourceGui::ptr sourceGui;
  SourceType sourceType = source->getSourceType();
  if (sourceType == SourceType::Video)
    sourceGui = SourceGui::ptr(new VideoGui(source));
  else if (sourceType == SourceType::Image)
    sourceGui = SourceGui::ptr(new ImageGui(source));
  else if (sourceType == SourceType::Color)
    sourceGui = SourceGui::ptr(new ColorGui(source));
  else
    sourceGui = SourceGui::ptr(new SourceGui(source));

  // Add to list of source guis..
  sourceGuis[sourceId] = sourceGui;
  QWidget* sourceEditor = sourceGui->getPropertiesEditor();
  sourcePropertyPanel->addWidget(sourceEditor);
  sourcePropertyPanel->setCurrentWidget(sourceEditor);
  sourcePropertyPanel->setEnabled(true);

  // When source value is changed, update canvases.
  //  connect(sourceGui.get(), SIGNAL(valueChanged()),
  //          this,           SLOT(updateCanvases()));

  connect(sourceGui.data(), SIGNAL(valueChanged(Source::ptr)),
          this,            SLOT(handleSourceChanged(Source::ptr)));

  connect(source.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(sourcePropertyChanged(uid, QString, QVariant)));

  // TODO: attention: if mapping is invisible canvases will be updated for no reason
  connect(source.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(updateCanvases()));

  // Add source item to sourceList widget.
  QListWidgetItem* item = new QListWidgetItem(icon, name);
  setItemId(*item, sourceId); // TODO: could possibly be replaced by a Source pointer

  // Set size.
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::PAINT_LIST_ITEM_HEIGHT));

  // Set tooltip.
  item->setToolTip(QString("ID: %1").arg(source->getId()));

  // Switch to source tab.
  contentTab->setCurrentWidget(sourceSplitter);

  // Add item to source list.
  sourceList->addItem(item);
  sourceList->setCurrentItem(item);

  // Update mapping guis.
  updateMappers();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::updateSourceItem(uid sourceId, const QIcon& icon, const QString& name) {
  QListWidgetItem* item = getItemFromId(*sourceList, sourceId);
  if (item == NULL) {
    // FIXME there was an assert that seemed to make MapMap crash, here.
    return;
  }

  // Update item info.
  item->setIcon(icon);
  item->setText(name);

  // Update mapping guis.
  updateMappers();

  // Window was modified.
  windowModified();
}

void MainWindow::addLayerItem(uid layerId)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  QString defaultName;
  QIcon icon;

  ShapeType shapeType = layer->getShape()->getType();
  SourceType sourceType = layer->getSource()->getSourceType();

  // Add mapper.
  // XXX hardcoded for textures
  QSharedPointer<TextureLayer> textureLayer;
  if (sourceType == SourceType::Video || sourceType == SourceType::Image)
  {
    textureLayer = qSharedPointerCast<TextureLayer>(layer);
    Q_CHECK_PTR(textureLayer);
  }

  LayerGui::ptr mapper;

  // XXX Branching on nVertices() is crap

  // Triangle
  if (shapeType == ShapeType::Triangle)
  {
    defaultName = QString("Triangle %1").arg(layerId);
    icon = QIcon(":/shape-triangle");

    if (sourceType == SourceType::Color)
      mapper = LayerGui::ptr(new PolygonColorLayerGui(layer));
    else
      mapper = LayerGui::ptr(new TriangleTextureLayerGui(textureLayer));
  }
  // Mesh
  else if (shapeType == ShapeType::Mesh)
  {
    defaultName = QString("Mesh %1").arg(layerId);
    icon = QIcon(":/shape-mesh");
    if (sourceType == SourceType::Color)
      mapper = LayerGui::ptr(new MeshColorLayerGui(layer));
    else
      mapper = LayerGui::ptr(new MeshTextureLayerGui(textureLayer));
  }
  else if (shapeType == ShapeType::Ellipse)
  {
    defaultName = QString("Ellipse %1").arg(layerId);
    icon = QIcon(":/shape-ellipse");
    if (sourceType == SourceType::Color)
      mapper = LayerGui::ptr(new EllipseColorLayerGui(layer));
    else
      mapper = LayerGui::ptr(new EllipseTextureLayerGui(textureLayer));
  }
  else
  {
    defaultName = QString("Polygon %1").arg(layerId);
    icon = QIcon(":/shape-polygon");
  }

  // Label is only going to be applied if no name is present.
  if (layer->getName().isEmpty())
    layer->setName(defaultName);

  // Add to list of layerGuis.
  layerGuis[layerId] = mapper;
  QWidget* mapperEditor = mapper->getPropertiesEditor();
  layerPropertyPanel->addWidget(mapperEditor);
  layerPropertyPanel->setCurrentWidget(mapperEditor);
  layerPropertyPanel->setEnabled(true);

  // When mapper value is changed, update canvases.
  connect(mapper.data(), SIGNAL(valueChanged()),
          this,          SLOT(updateCanvases()));

  // Also update playing state in case source was changed.
  connect(mapper.data(), SIGNAL(sourceChanged()),
          this,          SLOT(updatePlayingState()));

  connect(sourceCanvas,  SIGNAL(shapeChanged(MShape*)),
          mapper.data(), SLOT(updateShape(MShape*)));

  connect(destinationCanvas, SIGNAL(shapeChanged(MShape*)),
          mapper.data(),     SLOT(updateShape(MShape*)));

  connect(layer.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(layerPropertyChanged(uid, QString, QVariant)));

  // TODO: attention: if mapping is invisible canvases will be updated for no reason
  connect(layer.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(updateCanvases()));

  // Switch to mapping tab.
  contentTab->setCurrentWidget(layerSplitter);

  // Add item to layerList widget.
  layerListModel->addItem(layer, icon, layer->getName());
  layerListModel->updateModel();
  setCurrentLayer(layerId);

  // Add items to scenes.
  if (mapper->getInputGraphicsItem())
    sourceCanvas->scene()->addItem(mapper->getInputGraphicsItem().data());
  if (mapper->getGraphicsItem())
    destinationCanvas->scene()->addItem(mapper->getGraphicsItem().data());

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::removeLayerItem(uid layerId)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  // Remove mapping from model.
  mappingManager->removeLayer(layerId);

  // Remove associated mapper.
  layerPropertyPanel->removeWidget(layerGuis[layerId]->getPropertiesEditor());
  layerGuis.remove(layerId);

  // Remove widget from layerList.
  int row = layerListModel->getItemRowFromId(layerId);
  Q_ASSERT( row >= 0 );
  layerListModel->removeItem(row);

  // Update list.
  layerListModel->updateModel();

  if (layerListModel->rowCount() == 0)
    removeCurrentLayer();
  else
  {
    int nextSelectedRow = row == layerListModel->rowCount() ? row - 1 : row;
    QModelIndex index = layerListModel->getIndexFromRow(nextSelectedRow);
    layerList->selectionModel()->select(index, QItemSelectionModel::Select);
    layerList->setCurrentIndex(index);
  }

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::moveLayerItem(uid layerId, int idx)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  // Remove mapping from model.
  mappingManager->moveLayer(layerId, idx);

  // Remove widget from layerList.
  int row = layerListModel->getItemRowFromId(layerId);
  Q_ASSERT( row >= 0 );
  layerListModel->moveItem(row, idx);

  // Update list.
  layerListModel->updateModel();

  QModelIndex index = layerListModel->getIndexFromRow(idx);
  layerList->selectionModel()->select(index, QItemSelectionModel::Select);
  layerList->setCurrentIndex(index);

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::removeSourceItem(uid sourceId)
{
  Source::ptr source = mappingManager->getSourceById(sourceId);
  Q_CHECK_PTR(source);

  // Remove all mappings associated with source.
  QMap<uid, Layer::ptr> sourceLayers = mappingManager->getSourceLayers(source);
  for (QMap<uid, Layer::ptr>::const_iterator it = sourceLayers.constBegin();
       it != sourceLayers.constEnd(); ++it) {
    removeLayerItem(it.key());
  }
  // Remove source from model.
  Q_ASSERT( mappingManager->removeSource(sourceId) );

  // Remove associated mapper.
  sourcePropertyPanel->removeWidget(sourceGuis[sourceId]->getPropertiesEditor());
  sourceGuis.remove(sourceId);

  updateMappers();

  // Remove widget from sourceList.
  int row = getItemRowFromId(*sourceList, sourceId);
  Q_ASSERT( row >= 0 );
  QListWidgetItem* item = sourceList->takeItem(row);
  if (item == currentSelectedItem)
    currentSelectedItem = NULL;
  delete item;

  // Update list.
  sourceList->update();

  // Reset current source.
  removeCurrentSource();

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();
  // Build mapping!
  // FIXME: layer->build(); // I removed this 2014-04-25

  // Update playing state.
  updatePlayingState();
}

void MainWindow::clearWindow()
{
  clearProject();
}

void MainWindow::syncLayerManager()
{
  // Reorder mappings.
  QVector<uid> newOrder;
  for (int row=0; row<layerListModel->rowCount(); row++)
//  for (int row=layerListModel->rowCount()-1; row>=0; row--)
  {
    uid layerId = layerListModel->getIndexFromRow(row).data(Qt::UserRole).toInt();
    newOrder.push_back(layerId);
  }
  mappingManager->reorderLayers(newOrder);
}

bool MainWindow::fileExists(const QString &file)
{
  QFileInfo checkFile(file);

  if (checkFile.exists() && checkFile.isFile())
    return true;

  return false;
}

bool MainWindow::fileSupported(const QString &file, bool isImage)
{
  QFileInfo fileInfo(file);
  QString fileExtension = fileInfo.suffix();

  if (isImage) {
    if (MM::IMAGE_FILES_FILTER.contains(fileExtension, Qt::CaseInsensitive) &&
        QImageReader(file).canRead()) // extra check: makes sure format is readable
      return true;
  } else {
    if (MM::VIDEO_FILES_FILTER.contains(fileExtension, Qt::CaseInsensitive))
      return true;
  }

  QMessageBox::warning(this, tr("Warning"),
                       tr("The following file is not supported: %1")
                       .arg(fileInfo.fileName()));
  return false;
}

bool MainWindow::fileSupported(const QString &file, const QString &extension)
{
  if (!QFileInfo(file).suffix().isEmpty() &&
      extension.contains(QFileInfo(file).suffix(), Qt::CaseInsensitive)) {
    return true;
  } else {
    return false;
  }
}

QString MainWindow::locateMediaFile(const QString &uri, bool isImage)
{
  // Get more info about url
  QFileInfo file(uri);
  // The name of the file
  QString filename = file.fileName();
  // Handle the case where it is video or image
  QString mediaFilter = isImage ? MM::IMAGE_FILES_FILTER : MM::VIDEO_FILES_FILTER;
  QString mediaType = isImage ? "Images" : "Videos";

  // Show a warning and offer to locate the file
  QMessageBox::warning(this,
                       tr("Cannot load movie"),
                       tr("Unable to use file %1.\n"
                          "The original file is not found. Please locate.")
                       .arg(filename));

  // Set the new uri
#ifdef Q_OS_LINUX
 QString newUri = QFileDialog::getOpenFileName(this,
                                               tr("Locate file %1").arg(filename),
                                               file.absolutePath(),
                                               tr("%1 files (%2)")
                                               .arg(mediaType)
                                               .arg(mediaFilter),
                                               nullptr, QFileDialog::DontUseNativeDialog);
#else
  QString newUri = QFileDialog::getOpenFileName(this,
                                                tr("Locate file %1").arg(filename),
                                                file.absolutePath(),
                                                tr("%1 files (%2)")
                                                .arg(mediaType)
                                                .arg(mediaFilter));
#endif

  return newUri;

}

MainWindow* MainWindow::window() {
  static MainWindow* instance = nullptr;
  if (!instance) {
    instance = new MainWindow;
  }
  return instance;
}

void MainWindow::updateCanvases()
{
  // Update scenes.
  sourceCanvas->scene()->update();
  destinationCanvas->scene()->update();

  // Update canvases.
  sourceCanvas->update();
  destinationCanvas->update();
  outputWindow->getCanvas()->update();

  // Update statut bar
  updateStatusBar();
}

void MainWindow::updateMappers() {
  // Update mapping guis.
  for (QMap<uid, LayerGui::ptr>::iterator it = layerGuis.begin();
       it != layerGuis.end(); ++it) {
    it.value()->updateSources();
  }
}

void MainWindow::processFrame()
{
  // Number of frames processed (restarted every second).
  static unsigned int nFrames = 0;

  // Update canvases.
  updateCanvases();

  // Update true FPS.
  nFrames++;
  if (nFrames > framesPerSecond())
  {
    // This is the real time needed to process one second.
    qreal trueFramesPerSecond = nFrames / systemTimer->restart() * 1000.0;
    trueFramesPerSecondsLabel->setText(
        "FPS: " + QString::number(trueFramesPerSecond, 'f', 2) + " / " +
        QString::number(framesPerSecond()  , 'f', 2));
    nFrames = 0;
  }
}

void MainWindow::updatePlayingState()
{
  // Pause all sources that are not visible.
  if (isPlaying())
  {
    QVector<Source::ptr> visibleSources = mappingManager->getVisibleSources();
    for (int i=0; i<mappingManager->nSources(); i++)
    {
      Source::ptr source = mappingManager->getSource(i);
      if (visibleSources.contains(source))
      {
        source->play();
      }
      else
      {
        source->pause();
      }
    }
  }

  // Pause everyone.
  else
  {
    for (int i=0; i<mappingManager->nSources(); i++)
    {
      mappingManager->getSource(i)->pause();
    }
  }

  // Update all source items with correct icon according to playing state.
  for (int i=0; i<mappingManager->nSources(); i++)
  {
    Source::ptr source = mappingManager->getSource(i);
    updateSourceItem(source->getId(), getSourceIcon(source), source->getName());
  }

}

void MainWindow::enableDisplayControls(bool display)
{
  _displayControls = display;
  updateCanvases();
}

void MainWindow::setFramesPerSecond(qreal fps)
{
  _framesPerSecond = qMax(fps, 0.0);
  videoTimer->setInterval( int( 1000 / _framesPerSecond ) );
}

void MainWindow::enableDisplaySourceControls(bool display)
{
  _displaySourceControls = display;
  updateCanvases();
}

void MainWindow::displayUndoHistory(bool display)
{
  _displayUndoStack = display;

  // Create undo view.
  undoView = new QUndoView(getUndoStack(), this);

  if (display) {
    contentTab->addTab(undoView, tr("Undo history"));
  } else {
    contentTab->removeTab(2);
  }
}

void MainWindow::enableStickyVertices(bool value)
{
  _stickyVertices = value;
  settings.setValue("stickyVertices", _stickyVertices);
}

void MainWindow::showLayerContextMenu(const QPoint &point)
{
  QWidget *objectSender = static_cast<QWidget*>(sender());
  uid layerId = currentLayerItemId();
  Layer::ptr layer = mappingManager->getLayerById(layerId);

  // Switch to right action check state
  layerLockedAction->setChecked(layer->isLocked());
  layerHideAction->setChecked(!layer->isVisible());
  layerSoloAction->setChecked(layer->isSolo());

  if (objectSender != nullptr) {
    if (sender() == layerItemDelegate) // XXX: The item delegate is not a widget
      layerContextMenu->exec(layerList->mapToGlobal(point));
    else
      layerContextMenu->exec(objectSender->mapToGlobal(point));
  }
}

void MainWindow::showSourceContextMenu(const QPoint &point)
{
  QWidget *objectSender = dynamic_cast<QWidget*>(sender());

  if (objectSender != nullptr && sourceList->count() > 0)
    sourceContextMenu->exec(objectSender->mapToGlobal(point));
}

void MainWindow::play(bool updatePlayPauseActions)
{
  // Update buttons.
  if (updatePlayPauseActions)
  {
    playAction->setVisible(false);
    pauseAction->setVisible(true);
  }

  _isPlaying = true;

  updatePlayingState();
}

void MainWindow::pause(bool updatePlayPauseActions)
{
  // Update buttons.
  if (updatePlayPauseActions)
  {
    playAction->setVisible(true);
    pauseAction->setVisible(false);
  }
  _isPlaying = false;

  updatePlayingState();
}

void MainWindow::rewind()
{
  // Rewind all paints.
  for (int i=0; i<mappingManager->nSources(); i++)
    mappingManager->getSource(i)->rewind();
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

const QIcon MainWindow::getSourceIcon(Source::ptr source)
{
  if (source->isPlaying())
    return source->getIcon();
  else
  {
    QPixmap pixmap = source->getIcon().pixmap(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(255, 0, 0, 128), 4));
    painter.drawLine(0, 0, pixmap.width(), pixmap.height());
    return QIcon(pixmap);
  }
}

void MainWindow::connectProjectWidgets()
{
  connect(sourceList, SIGNAL(itemSelectionChanged()),
          this,      SLOT(handleSourceItemSelectionChanged()));

  connect(sourceList, SIGNAL(itemPressed(QListWidgetItem*)),
          this,      SLOT(handleSourceItemSelected(QListWidgetItem*)));

  connect(sourceList, SIGNAL(itemActivated(QListWidgetItem*)),
          this,      SLOT(handleSourceItemSelected(QListWidgetItem*)));
  // Rename Source with double click
  connect(sourceList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this,      SLOT(renameSourceItem()));
  // When finish to edit mapping item
  connect(sourceList->itemDelegate(), SIGNAL(commitData(QWidget*)),
          this, SLOT(sourceListEditEnd(QWidget*)));

  connect(layerList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleLayerItemSelectionChanged(QModelIndex)));

  connect(layerListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleLayerItemChanged(QModelIndex)));

  connect(layerListModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this,                 SLOT(handleLayerIndexesMoved()));

  connect(layerItemDelegate, SIGNAL(itemDuplicated(uid)),
          this, SLOT(duplicateLayer(uid)));

  connect(layerItemDelegate, SIGNAL(itemRemoved(uid)),
          this, SLOT(deleteLayer(uid)));

  connect(_preferenceDialog, SIGNAL(settingSaved()), this, SLOT(updateSettings()));
}

void MainWindow::disconnectProjectWidgets()
{
  disconnect(sourceList, SIGNAL(itemSelectionChanged()),
             this,      SLOT(handleSourceItemSelectionChanged()));

  disconnect(sourceList, SIGNAL(itemPressed(QListWidgetItem*)),
             this,      SLOT(handleSourceItemSelected(QListWidgetItem*)));

  disconnect(sourceList, SIGNAL(itemActivated(QListWidgetItem*)),
             this,      SLOT(handleSourceItemSelected(QListWidgetItem*)));

  disconnect(layerList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleLayerItemSelectionChanged(QModelIndex)));

  disconnect(layerListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleLayerItemChanged(QModelIndex)));

  disconnect(layerListModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this,                 SLOT(handleLayerIndexesMoved()));

  disconnect(layerItemDelegate, SIGNAL(itemDuplicated(uid)),
          this, SLOT(duplicateLayer(uid)));

  disconnect(layerItemDelegate, SIGNAL(itemRemoved(uid)),
          this, SLOT(deleteLayer(uid)));

  disconnect(_preferenceDialog, SIGNAL(settingSaved()), this, SLOT(updateSettings()));
}

uid MainWindow::getItemId(const QListWidgetItem& item)
{
  return item.data(Qt::UserRole).toInt();
}

void MainWindow::setItemId(QListWidgetItem& item, uid id)
{
  item.setData(Qt::UserRole, id);
}

QListWidgetItem* MainWindow::getItemFromId(const QListWidget& list, uid id) {
  int row = getItemRowFromId(list, id);
  if (row >= 0)
    return list.item( row );
  else
    return NULL;
}

int MainWindow::getItemRowFromId(const QListWidget& list, uid id)
{
  for (int row=0; row<list.count(); row++)
  {
    QListWidgetItem* item = list.item(row);
    if (getItemId(*item) == id)
      return row;
  }

  return (-1);
}

uid MainWindow::currentLayerItemId() const
{
  return layerListModel->getItemId(currentSelectedIndex);
}

QIcon MainWindow::createColorIcon(const QColor &color) {
  QPixmap pixmap(100,100);
  pixmap.fill(color);
  return QIcon(pixmap);
}

QIcon MainWindow::createFileIcon(const QString& filename) {
  static QFileIconProvider provider;
  return provider.icon(QFileInfo(filename));
}

QIcon MainWindow::createImageIcon(const QString& filename) {
  return QIcon(filename);
}


void MainWindow::setCurrentSource(int uid)
{
  if (uid == NULL_UID)
    removeCurrentSource();
  else {
    if (currentSourceId != uid) {
      currentSourceId = uid;
      sourceList->setCurrentRow( getItemRowFromId(*sourceList, uid) );
      sourcePropertyPanel->setCurrentWidget(sourceGuis[uid]->getPropertiesEditor());
    }
    _hasCurrentSource = true;
  }
}

void MainWindow::setCurrentLayer(int uid)
{
  if (uid == NULL_UID)
    removeCurrentLayer();
  else {
    if (currentLayerId != uid) {
      currentLayerId = uid;
      currentSelectedIndex = layerListModel->getIndexFromRow(layerListModel->getItemRowFromId(uid));
      layerList->setCurrentIndex(currentSelectedIndex);
      layerPropertyPanel->setCurrentWidget(layerGuis[uid]->getPropertiesEditor());
    }
    _hasCurrentLayer = true;
  }
}

void MainWindow::removeCurrentSource() {
  _hasCurrentSource = false;
  currentSourceId = NULL_UID;
  sourceList->clearSelection();
}

void MainWindow::removeCurrentLayer() {
  _hasCurrentLayer = false;
  currentLayerId = NULL_UID;
  layerList->clearSelection();
}

void MainWindow::startOscReceiver()
{
  std::ostringstream os;
  os << oscListeningPort;
#if QT_VERSION >= 0x050500
  QMessageLogger(__FILE__, __LINE__, 0).info() << "OSC port: " << oscListeningPort;
#else
  QMessageLogger(__FILE__, __LINE__, 0).debug() << "OSC port: " << oscListeningPort;
#endif
  osc_interface.reset(new OscInterface(oscListeningPort));
  if (oscListeningPort != 0)
  {
    osc_interface->start();
  }
  osc_timer = new QTimer(this); // FIXME: memleak?
  connect(osc_timer, SIGNAL(timeout()), this, SLOT(pollOscInterface()));
  osc_timer->start();
}

bool MainWindow::setOscPort(int port)
{
  if (port <= 1023 || port > 65535)
  {
    qWarning() << "OSC port is out of range: " << port << Qt::endl;
    return false;
  }
  oscListeningPort = port;
  startOscReceiver();
  return true;
}

int MainWindow::getOscPort() const
{
  return oscListeningPort;
}

bool MainWindow::setOscPort(QString portNumber)
{
  bool ok;
  int port = portNumber.toInt(&ok);
  if (ok)
  {
    return setOscPort(port);
  }
  else
  {
    qWarning() << "OSC port is not a number: " << portNumber << Qt::endl;
    return false;
  }
  return true;
}

void MainWindow::pollOscInterface()
{
    // FIXME: we should now use its QObject signals instead of polling it
  osc_interface->consume_commands(*this);
}

void MainWindow::exitFullScreen()
{
  outputFullScreenAction->setChecked(false);
  displayTestSignalAction->setChecked(false);
}

void MainWindow::openShortcutWindow()
{
  _shortcutWindow->reload(); // Important for speed
  _shortcutWindow->setVisible(true);
}

void MainWindow::updateSettings()
{
  stickyVerticesAction->setChecked(settings.value("stickyVertices").toBool());
}

void MainWindow::updateLayerListColumnWidth()
{
  layerList->setColumnWidth(1, layerList->horizontalHeader()->width() - (MM::MAPPING_LIST_HIDE_COLUMN + MM::MAPPING_LIST_BUTTONS_COLUMN));
}

// void MainWindow::applyOscCommand(const QVariantList& command)
// {
//   bool VERBOSE = true;
//   if (VERBOSE)
//   {
//     std::cout << "Receive OSC: ";
//     for (int i = 0; i < command.size(); ++i)
//     {
//       if (command.at(i).type()  == QVariant::Int)
//       {
//         std::cout << command.at(i).toInt() << " ";
//       }
//       else if (command.at(i).type()  == QVariant::Double)
//       {
//         std::cout << command.at(i).toDouble() << " ";
//       }
//       else if (command.at(i).type()  == QVariant::String)
//       {
//         std::cout << command.at(i).toString().toStdString() << " ";
//       }
//       else
//       {
//         std::cout << "??? ";
//       }
//     }
//     std::cout << std::endl;
//     std::cout.flush();
//   }
//
//   if (command.size() < 2)
//       return;
//   if (command.at(0).type() != QVariant::String)
//       return;
//   if (command.at(1).type() != QVariant::String)
//       return;
//   std::string path = command.at(0).toString().toStdString();
//   std::string typetags = command.at(1).toString().toStdString();
//
//   // Handle all OSC messages here
//   if (path == "/image/uri" && typetags == "s")
//   {
//       std::string image_uri = command.at(2).toString().toStdString();
//       std::cout << "TODO load /image/uri " << image_uri << std::endl;
//   }
//   else if (path == "/add/quad")
//       addMesh();
//   else if (path == "/add/triangle")
//       addTriangle();
//   else if (path == "/add/ellipse")
//       addEllipse();
//   else if (path == "/project/save")
//       save();
//   else if (path == "/project/open")
//       open();
// }

void MainWindow::quitMapMap()
{
  close();
}

}
