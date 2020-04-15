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
  currentPaintId = NULL_UID;
  currentMappingId = NULL_UID;
  // TODO: not sure we need this anymore since we have NULL_UID
  _hasCurrentPaint = false;
  _hasCurrentMapping = false;
  currentSelectedItem = NULL;

  // Frames per second.
  _framesPerSecond = (-1);

  // Play state.
  _isPlaying = false;

  // Editing toggles.
  _displayControls = true;
  _displayPaintControls = true;
  _stickyVertices = true;
  _displayUndoStack = false;
  _showMenuBar = true; // Show menubar by default

  // UndoStack
  undoStack = new QUndoStack(this);

  // Create everything.
  createLayout();
  createActions();
  createMenus();
  createMappingContextMenu();
  createPaintContextMenu();
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
#ifdef HAVE_OSC
  delete osc_timer;
#endif // ifdef
  delete systemTimer;
}

void MainWindow::handlePaintItemSelectionChanged()
{
  // Set current paint.
  QListWidgetItem* item = paintList->currentItem();
  currentSelectedItem = item;

  // Is a paint item selected?
  bool paintItemSelected = (item ? true : false);

  if (paintItemSelected)
  {
    // Set current paint.
    uid paintId = getItemId(*item);
    // Unselect current mapping.
    if (currentPaintId != paintId)
      removeCurrentMapping();
    // Set current paint.
    setCurrentPaint(paintId);
  }
  else
    removeCurrentPaint();

  // Enable/disable creation of mappings depending on whether a paint is selected.
  addMeshAction->setEnabled(paintItemSelected);
  addTriangleAction->setEnabled(paintItemSelected);
  addEllipseAction->setEnabled(paintItemSelected);
  deletePaintAction->setEnabled(paintItemSelected);
  renamePaintAction->setEnabled(paintItemSelected);

  // Update canvases.
  updateCanvases();
}

void MainWindow::handleMappingItemSelectionChanged(const QModelIndex &index)
{
  // Set current paint and mappings.
  uid mappingId = mappingListModel->getItemId(index);
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  uid paintId = mapping->getPaint()->getId();
  // Set current mapping and paint
  setCurrentMapping(mappingId);
  setCurrentPaint(paintId);
  // Enable destination zoom toolbar buttons and avoid loop
  if (!destinationCanvasToolbar->buttonsAreEnable()) {
    // Enable destination toolbar
   destinationCanvasToolbar->enableZoomToolBar(true);
   // Enable source toolbar
   sourceCanvasToolbar->enableZoomToolBar(true);
   // Enable paint and mapping edit action
   duplicateMappingAction->setEnabled(true);
   deleteMappingAction->setEnabled(true);
   renameMappingAction->setEnabled(true);
   mappingLockedAction->setEnabled(true);
   mappingHideAction->setEnabled(true);
   mappingSoloAction->setEnabled(true);
   mappingRotate90CWAction->setEnabled(true);
   mappingRotate90CCWAction->setEnabled(true);
   mappingRotate180Action->setEnabled(true);
   mappingHorizontalFlipAction->setEnabled(true);
   mappingVerticalFlipAction->setEnabled(true);
   mappingRaiseAction->setEnabled(true);
   mappingLowerAction->setEnabled(true);
   mappingRaiseToTopAction->setEnabled(true);
   mappingLowerToBottomAction->setEnabled(true);
   // Enable zoom action
   zoomInAction->setEnabled(true);
   zoomOutAction->setEnabled(true);
   resetZoomAction->setEnabled(true);
   fitToViewAction->setEnabled(true);
  }

  // Update canvases.
  updateCanvases();
  updateMappingListColumnWidth();
}

void MainWindow::handleMappingItemChanged(const QModelIndex &index)
{
  // Get item.
  uid mappingId = mappingListModel->getItemId(index);

  // Sync name.
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  // Change properties.
  mapping->setName(index.data(Qt::EditRole).toString());
  mapping->setVisible(index.data(Qt::CheckStateRole).toBool());
  mapping->setSolo(index.data(Qt::CheckStateRole + 1).toBool());
  mapping->setLocked(index.data(Qt::CheckStateRole + 2).toBool());

  // Update model (important to make sure icons get updated in the interface).
  mappingListModel->updateModel();

  updatePlayingState();
 }

void MainWindow::handleMappingIndexesMoved()
{
  // Resync mapping manager.
  syncMappingManager();

  // Update canvases according to new order.
  updateCanvases();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::handlePaintItemSelected(QListWidgetItem* item)
{
  Q_UNUSED(item);
  // Change currently selected item.
  currentSelectedItem = item;
}

void MainWindow::handlePaintChanged(Paint::ptr paint)
{
  // Change currently selected item.
  uid curMappingId = getCurrentMappingId();
  removeCurrentMapping();
  removeCurrentPaint();

  uid paintId = mappingManager->getPaintId(paint);

//  QSharedPointer<Texture> texture;

  if (paint->getSourceType() == SourceType::Video)
  {
    QSharedPointer<Video> media = qSharedPointerCast<Video>(paint);
    Q_CHECK_PTR(media);
    updatePaintItem(paintId, getPaintIcon(paint), strippedName(media->getUri()));
    //    QString fileName = QFileDialog::getOpenFileName(this,
    //        tr("Import media source file"), ".");
    //    // Restart video playback. XXX Hack
    //    if (!fileName.isEmpty())
    //      importMediaFile(fileName, paint, false);
  }
  if (paint->getSourceType() == SourceType::Image)
  {
    QSharedPointer<Image> image = qSharedPointerCast<Image>(paint);
    Q_CHECK_PTR(image);
    updatePaintItem(paintId, getPaintIcon(paint), strippedName(image->getUri()));
    //    QString fileName = QFileDialog::getOpenFileName(this,
    //        tr("Import media source file"), ".");
    //    // Restart video playback. XXX Hack
    //    if (!fileName.isEmpty())
    //      importMediaFile(fileName, paint, true);
  }
  else if (paint->getSourceType() == SourceType::Color)
  {
    // Pop-up color-choosing dialog to choose color paint.
    QSharedPointer<Color> color = qSharedPointerCast<Color>(paint);
    Q_CHECK_PTR(color);
    updatePaintItem(paintId, getPaintIcon(paint), strippedName(color->getColor().name()));
  }

  if (curMappingId != NULL_UID)
  {
    setCurrentMapping(curMappingId);
  }

//  updatePlayingState();
}

void MainWindow::mappingPropertyChanged(uid id, QString propertyName, QVariant value)
{
  // Retrieve mapping.
  Mapping::ptr mapping = mappingManager->getMappingById(id);
  Q_CHECK_PTR(mapping);

  // Send to mapping gui.
  MappingGui::ptr mappingGui = getMappingGuiByMappingId(id);
  Q_CHECK_PTR(mappingGui);
  mappingGui->setValue(propertyName, value);

  // Send to actions.
  if (mapping == getCurrentMapping())
  {
    if (propertyName == "visible")
    {
      mappingHideAction->setChecked(!value.toBool());
      updatePlayingState();
    }
    else if (propertyName == "solo")
    {
      mappingSoloAction->setChecked(value.toBool());
      updatePlayingState();
    }
    else if (propertyName == "locked")
    {
      mappingLockedAction->setChecked(value.toBool());
    }
    else if (propertyName == "paintId")
    {
      mappingGui->updatePaints();
      updatePlayingState();
    }
  }

  // Send to list items.
  const QModelIndex& index = mappingListModel->getIndexFromId(mapping->getId());
  if (propertyName == "name")
  {
    mappingListModel->setData(index, mapping->getName(), Qt::EditRole);
  }
  else if (propertyName == "visible")
  {
    mappingListModel->setData(index, mapping->isVisible(), Qt::CheckStateRole);
  }
  else if (propertyName == "solo")
  {
    mappingListModel->setData(index, mapping->isSolo(), Qt::CheckStateRole + 1);
  }
  else if (propertyName == "locked")
  {
    mappingListModel->setData(index, mapping->isLocked(), Qt::CheckStateRole + 2);
  }
}

void MainWindow::paintPropertyChanged(uid id, QString propertyName, QVariant value)
{
  // Retrieve paint.
  Paint::ptr paint = mappingManager->getPaintById(id);
  Q_CHECK_PTR(paint);

  // Send to paint gui.
  PaintGui::ptr paintGui = getPaintGuiByPaintId(id);
  Q_CHECK_PTR(paintGui);

  paintGui->setValue(propertyName, value);

  // Send to list items.
  QListWidgetItem* paintItem = getItemFromId(*paintList, id);
  if (propertyName == "name")
    paintItem->setText(paint->getName());
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
#ifdef Q_OS_OSX // On Mac OS X
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
    foreach (QUrl url, mimeData->urls()) {
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
    foreach (QUrl url, mimeData->urls()) {
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
  displayPaintControlsAction->setChecked(enable);
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
  QList<QCameraInfo> cameras = QCameraInfo::availableCameras();

  if (cameras.count() > 1)
  {
    QStringList devicesList;
    QMap<QString, QString> devices;

    for (const QCameraInfo &cameraInfo: cameras)
    {
      devicesList << cameraInfo.description();
      devices.insert(cameraInfo.description(), cameraInfo.deviceName());
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
    if (QCameraInfo::defaultCamera().isNull())
    {
      QMessageBox::warning(this, tr("No camera available"), tr("You can not use this feature!\nNo camera available in your system"));

    }
    else
    {
      device = QCameraInfo::defaultCamera().deviceName();
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

  // Pop-up color-choosing dialog to choose color paint.
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
    addColorPaint(color);
  }

  // Restart video playback if it was previously playing. XXX Hack
  if (pauseAction->isVisible())
    play(false);
}

void MainWindow::addMesh()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaintById(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output quads.
  Mapping* mappingPtr;
  if (paint->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputQuad = MShape::ptr(Util::createMeshForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputQuad);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(paint);
    Q_CHECK_PTR(texture);

    MShape::ptr outputQuad = MShape::ptr(Util::createMeshForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr  inputQuad = MShape::ptr(Util::createMeshForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, outputQuad, inputQuad);
  }

  // Create texture mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);

  // Lets the undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddMappingCommand(this, mappingId));
}

void MainWindow::addTriangle()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaintById(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output quads.
  Mapping* mappingPtr;
  if (paint->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputTriangle = MShape::ptr(Util::createTriangleForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputTriangle);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(paint);
    Q_CHECK_PTR(texture);

    MShape::ptr outputTriangle = MShape::ptr(Util::createTriangleForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr inputTriangle = MShape::ptr(Util::createTriangleForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, inputTriangle, outputTriangle);
  }

  // Create mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);

  // Lets undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddMappingCommand(this, mappingId));
}

void MainWindow::addEllipse()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaintById(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output ellipses.
  Mapping* mappingPtr;
  if (paint->getSourceType() == SourceType::Color)
  {
    MShape::ptr outputEllipse = MShape::ptr(Util::createEllipseForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputEllipse);
  }
  else
  {
    QSharedPointer<Texture> texture = qSharedPointerCast<Texture>(paint);
    Q_CHECK_PTR(texture);

    MShape::ptr outputEllipse = MShape::ptr(Util::createEllipseForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    MShape::ptr inputEllipse = MShape::ptr(Util::createEllipseForTexture(texture.data(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, inputEllipse, outputEllipse);
  }

  // Create mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);

  // Lets undo-stack handle Undo/Redo the adding of mapping item.
  undoStack->push(new AddMappingCommand(this, mappingId));
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
  if (currentSelectedItem) // Show mouse coordinate only if mappingList is not empty
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

#ifdef Q_OS_OSX // On Mac OS X
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
 * Deletes either a Paint or a Mapping.
 */
void MainWindow::deleteItem()
{
  bool isMappingTabSelected = (mappingSplitter == contentTab->currentWidget());
  bool isPaintTabSelected = (paintSplitter == contentTab->currentWidget());

  if (currentSelectedItem)
  {
    if (isMappingTabSelected) //currentSelectedItem->listWidget() == mappingList)
    {
      // Delete mapping.
      undoStack->push(new DeleteMappingCommand(this, getCurrentMappingId()));
      //currentSelectedItem = NULL;
    }
    else if (isPaintTabSelected) //currentSelectedItem->listWidget() == paintList)
    {
      // Delete paint.
      undoStack->push(new RemovePaintCommand(this, getItemId(*paintList->currentItem())));
      //currentSelectedItem = NULL;
    }
    else
    {
      qCritical() << "Selected item neither a mapping nor a paint." << endl;
    }
  }
}

void MainWindow::duplicateMappingItem()
{
  if (currentSelectedIndex.isValid())
  {
    duplicateMapping(currentMappingItemId());
  }
  else
  {
    qCritical() << "No selected mapping" << endl;
  }
}

void MainWindow::deleteMappingItem()
{
  if (hasCurrentMapping())
  {
    undoStack->push(new DeleteMappingCommand(this, getCurrentMappingId()));
  }
  else
  {
    qCritical() << "No selected mapping" << endl;
  }
}

void MainWindow::renameMappingItem()
{
  // Set current item editable and rename it
  QModelIndex index = mappingList->currentIndex();
  // Used by context menu
  mappingList->edit(index);
  // Switch to mapping tab.
  contentTab->setCurrentWidget(mappingSplitter);
}

void MainWindow::setMappingItemLocked(bool locked)
{
  setMappingLocked(currentMappingItemId(), locked);
}

void MainWindow::setMappingItemHide(bool hide)
{
  setMappingVisible(currentMappingItemId(), !hide);
}

void MainWindow::setMappingItemSolo(bool solo)
{
  setMappingSolo(currentMappingItemId(), solo);
}

void MainWindow::loadLayerMedia()
{
  QAction *action = qobject_cast<QAction *>(sender());
  Paint::ptr media;
  uid currentLayerId = getCurrentMapping()->getId();

  if (action) {
    if (action->data().toString() == "import-new-media") {
      // Due to the fact that we can't assign a media/paint without adding a mesh
      importMedia();
      addMesh(); // Creating a temporary mesh
      media = mappingManager->getPaintById(currentPaintId); // The last imported video is current ID
      deleteMapping(getCurrentMapping()->getId()); // Delete the temporary mesh
      setCurrentMapping(currentLayerId); // Set the previous selected layer as the current
    } else {
      media = mappingManager->getPaintById(action->data().toInt());
    }

    if (media && media != getCurrentMapping()->getPaint() &&
        getCurrentMapping()->paintIsCompatible(media)) {
      // Change layer source
      getCurrentMapping()->setPaint(media);
    }
  }
}

void MainWindow::transformActionMappingItem()
{
  QAction *actionSender = qobject_cast<QAction *>(sender());

  if (actionSender == mappingRotate90CWAction) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate90CW));
  }
  else if (actionSender == mappingRotate90CCWAction) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate90CCW));
  }
  else if (actionSender == mappingRotate180Action) {
    undoStack->push(new RotateShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Rotate180));
  }

  else if (actionSender == mappingHorizontalFlipAction) {
    undoStack->push(new FlipShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Horizontal));
  }
  else if (actionSender == mappingVerticalFlipAction) {
    undoStack->push(new FlipShapeCommand(destinationCanvas, TransformShapeCommand::FREE, destinationCanvas->getCurrentShape(), MShape::Vertical));
  }

}

void MainWindow::reorderMappingItem()
{
  QAction *actionSender = qobject_cast<QAction *>(sender());

  if (actionSender == mappingRaiseAction) {
    undoStack->push(new MoveMappingCommand(this, getCurrentMappingId(), MM::Raise));
  }
  else if (actionSender == mappingLowerAction) {
    undoStack->push(new MoveMappingCommand(this, getCurrentMappingId(), MM::Lower));
  }
  else if (actionSender == mappingRaiseToTopAction) {
    undoStack->push(new MoveMappingCommand(this, getCurrentMappingId(), MM::Top));
  }
  else if (actionSender == mappingLowerToBottomAction) {
    undoStack->push(new MoveMappingCommand(this, getCurrentMappingId(), MM::Bottom));
  }
}

void MainWindow::renameMapping(uid mappingId, const QString &name)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  if (!mapping.isNull()) {
    QModelIndex index = mappingListModel->getIndexFromId(mappingId);
    mappingListModel->setData(index, name, Qt::EditRole);
    mapping->setName(name);
  }
}

//void MainWindow::mappingListEditEnd(QWidget *editor)
//{
//  QString name = reinterpret_cast<QLineEdit*>(editor)->text();
//  renameMapping(getItemId(*mappingList->currentItem()), name);
//}

void MainWindow::deletePaintItem()
{
  if(hasCurrentPaint())
  {
    undoStack->push(new RemovePaintCommand(this, getCurrentPaintId()));
  }
  else
  {
    qCritical() << "No selected source" << endl;
  }
}

void MainWindow::renamePaintItem()
{
  // Set current item editable and rename it
  QListWidgetItem* item = paintList->currentItem();
  item->setFlags(item->flags() | Qt::ItemIsEditable);
  // Used by context menu
  paintList->editItem(item);
  // Switch to paint tab
  contentTab->setCurrentWidget(paintSplitter);
}

void MainWindow::renamePaint(uid paintId, const QString &name)
{
  Paint::ptr paint = mappingManager->getPaintById(paintId);
  Q_CHECK_PTR(paint);
  if (!paint.isNull()) {
    paint->setName(name);
  }
}

void MainWindow::paintListEditEnd(QWidget *editor)
{
  QString name = reinterpret_cast<QLineEdit*>(editor)->text();
  renamePaint(getItemId(*paintList->currentItem()), name);
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
  // Disconnect signals to avoid problems when clearning mappingList and paintList.
  disconnectProjectWidgets();

  // Clear current paint / mapping.
  removeCurrentPaint();
  removeCurrentMapping();

  // Empty list widgets.
  mappingListModel->clear();
  paintList->clear();

  // Clear property panel.
  for (int i=mappingPropertyPanel->count()-1; i>=0; i--)
    mappingPropertyPanel->removeWidget(mappingPropertyPanel->widget(i));

  // Disable property panel.
  mappingPropertyPanel->setDisabled(true);

  // Clear list of mappers.
  mappers.clear();

  // Clear list of paint guis.
  paintGuis.clear();

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

uid MainWindow::createMediaPaint(uid paintId, QString uri, float x, float y,
                                 bool isImage, VideoType type, double rate)
{
  // Cannot create image with already existing id.
  if (Paint::getUidAllocator().exists(paintId))
    return NULL_UID;

  else
  {
    Texture* tex = nullptr;
    if (isImage)
      tex = new Image(uri, paintId);
    else {
      tex = new Video(uri, type, rate, paintId);
    }

    // Create new image with corresponding ID.
    tex->setPosition(x, y);

    // Add it to the manager.
    Paint::ptr paint(tex);

    if (type == VIDEO_WEBCAM) {
      paint->setName(tex->getCameraNameFromUri(uri));
    } else {
      paint->setName(strippedName(uri));
    }

    // Add paint to model and return its uid.
    uid id = mappingManager->addPaint(paint);

    // Add paint widget item.
    undoStack->push(new AddPaintCommand(this, id, paint->getIcon(), paint->getName()));
    return id;
  }
}

uid MainWindow::createColorPaint(uid paintId, QColor color)
{
  // Cannot create image with already existing id.
  if (Paint::getUidAllocator().exists(paintId))
    return NULL_UID;

  else
  {
    Color* img = new Color(color, paintId);

    // Add it to the manager.
    Paint::ptr paint(img);
    paint->setName(strippedName(color.name()));

    // Add paint to model and return its uid.
    uid id = mappingManager->addPaint(paint);

    // Add paint widget item.
    undoStack->push(new AddPaintCommand(this, id, paint->getIcon(), paint->getName()));

    return id;
  }
}

uid MainWindow::createMeshTextureMapping(uid mappingId,
                                         uid paintId,
                                         int nColumns, int nRows,
                                         const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    int nVertices = nColumns * nRows;
    qDebug() << nVertices << " vs " << nColumns << "x" << nRows << " vs " << src.size() << " " << dst.size() << endl;
    Q_ASSERT(src.size() == nVertices && dst.size() == nVertices);

    MShape::ptr inputMesh( new Mesh(src, nColumns, nRows));
    MShape::ptr outputMesh(new Mesh(dst, nColumns, nRows));

    // Add it to the manager.
    Mapping::ptr mapping(new TextureMapping(paint, outputMesh, inputMesh, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createTriangleTextureMapping(uid mappingId,
                                             uid paintId,
                                             const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    Q_ASSERT(src.size() == 3 && dst.size() == 3);

    MShape::ptr inputTriangle( new Triangle(src[0], src[1], src[2]));
    MShape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

    // Add it to the manager.
    Mapping::ptr mapping(new TextureMapping(paint, outputTriangle, inputTriangle, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createEllipseTextureMapping(uid mappingId,
                                            uid paintId,
                                            const QVector<QPointF> &src, const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    Q_ASSERT(src.size() == 5 && dst.size() == 5);

    MShape::ptr inputEllipse( new Ellipse(src[0], src[1], src[2], src[3], src[4]));
    MShape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3], dst[4]));

    // Add it to the manager.
    Mapping::ptr mapping(new TextureMapping(paint, outputEllipse, inputEllipse, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createQuadColorMapping(uid mappingId,
                                       uid paintId,
                                       const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    Q_ASSERT(dst.size() == 4);

    MShape::ptr outputQuad(new Quad(dst[0], dst[1], dst[2], dst[3]));

    // Add it to the manager.
    Mapping::ptr mapping(new ColorMapping(paint, outputQuad, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createTriangleColorMapping(uid mappingId,
                                           uid paintId,
                                           const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    Q_ASSERT(dst.size() == 3);

    MShape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

    // Add it to the manager.
    Mapping::ptr mapping(new ColorMapping(paint, outputTriangle, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}

uid MainWindow::createEllipseColorMapping(uid mappingId,
                                          uid paintId,
                                          const QVector<QPointF> &dst)
{
  // Cannot create element with already existing id or element for which no paint exists.
  if (Mapping::getUidAllocator().exists(mappingId) ||
      !Paint::getUidAllocator().exists(paintId) ||
      paintId == NULL_UID)
    return NULL_UID;

  else
  {
    Paint::ptr paint = mappingManager->getPaintById(paintId);
    Q_ASSERT(dst.size() == 4);

    MShape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3]));

    // Add it to the manager.
    Mapping::ptr mapping(new ColorMapping(paint, outputEllipse, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
  }
}


void MainWindow::setMappingVisible(uid mappingId, bool visible)
{
  // Set mapping visibility
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);

  if (mapping.isNull())
  {
    qDebug() << "No such mapping id" << endl;
  }
  else
  {
    mapping->setVisible(visible);
    // Change list item check state
    QModelIndex index = mappingListModel->getIndexFromId(mappingId);
    mappingListModel->setData(index, visible, Qt::CheckStateRole);
    // Update canvases.
    updateCanvases();
  }
}

void MainWindow::setMappingSolo(uid mappingId, bool solo)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  if (!mapping.isNull()) {
    // Turn this mapping into solo mode
    mapping->setSolo(solo);
    // Change list item check state
    QModelIndex index = mappingListModel->getIndexFromId(mappingId);
    mappingListModel->setData(index, solo, Qt::CheckStateRole + 1);
    // Update canvases
    updateCanvases();
  }
}

void MainWindow::setMappingLocked(uid mappingId, bool locked)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);

  if (!mapping.isNull()) {
    // Lock position of mapping
    mapping->setLocked(locked);
    // Lock shape too.
    mapping->getShape()->setLocked(locked);
    // Change list item check state
    QModelIndex index = mappingListModel->getIndexFromId(mappingId);
    mappingListModel->setData(index, locked, Qt::CheckStateRole + 2);
    // Update canvases
    updateCanvases();
  }
}

void MainWindow::deleteMapping(uid mappingId)
{
  // Cannot delete unexisting mapping.
  if (Mapping::getUidAllocator().exists(mappingId))
  {
    removeMappingItem(mappingId);
  }
}

void MainWindow::moveMapping(uid mappingId, int idx)
{
  // Cannot delete unexisting mapping.
  if (Mapping::getUidAllocator().exists(mappingId))
  {
    moveMappingItem(mappingId, idx);
  }
}

void MainWindow::duplicateMapping(uid mappingId)
{
  // Clone current Mapping.
  Mapping::ptr clonedMappingPtr(mappingManager->getMappingById(mappingId)->clone());

  // Get duplicated mapping id
  uid cloneId = mappingManager->addMapping(clonedMappingPtr);

  // Lets the undo-stack handle Undo/Redo the duplication of mapping item.
  undoStack->push(new DuplicateMappingCommand(this, cloneId));
}

/// Deletes/removes a paint and all associated mappigns.
void MainWindow::deletePaint(uid paintId, bool replace)
{
  // Cannot delete unexisting paint.
  if (Paint::getUidAllocator().exists(paintId))
  {
    if (replace == false) {
      int r = QMessageBox::warning(this, tr("MapMap"),
                                   tr("Remove this source and all its associated layers?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
      if (r == QMessageBox::Ok)
      {
        removePaintItem(paintId);
      }
    }
    else
      removePaintItem(paintId);
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
  // Create paint list.
  paintList = new QListWidget;
  paintList->setSelectionMode(QAbstractItemView::SingleSelection);
  paintList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  paintList->setDefaultDropAction(Qt::MoveAction);
  paintList->setDragDropMode(QAbstractItemView::InternalMove);
  paintList->setMinimumWidth(PAINT_LIST_MINIMUM_HEIGHT);

  // Create paint panel.
  paintPropertyPanel = new QStackedWidget;
  paintPropertyPanel->setDisabled(true);
  paintPropertyPanel->setMinimumHeight(PAINT_PROPERTY_PANEL_MINIMUM_HEIGHT);

  // Create mapping list.
  mappingList = new QTableView;
  mappingList->setSelectionMode(QAbstractItemView::SingleSelection);
  mappingList->setSelectionBehavior(QAbstractItemView::SelectRows);
  mappingList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  mappingList->setDragEnabled(true);
  mappingList->setAcceptDrops(true);
  mappingList->setDropIndicatorShown(true);
  mappingList->setEditTriggers(QAbstractItemView::DoubleClicked);
  mappingList->setMinimumHeight(MAPPING_LIST_MINIMUM_HEIGHT);
  mappingList->setContentsMargins(0, 0, 0, 0);
  // Set view delegate
  mappingListModel = new MappingListModel;
  mappingItemDelegate = new MappingItemDelegate;
  mappingList->setModel(mappingListModel);
  mappingList->setItemDelegate(mappingItemDelegate);
  // Pimp Mapping table widget
  mappingList->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  mappingList->setShowGrid(false);
  mappingList->horizontalHeader()->hide();
  mappingList->verticalHeader()->hide();
  mappingList->setMouseTracking(true);// Important
  mappingList->setColumnWidth(0, MM::MAPPING_LIST_HIDE_COLUMN);
  mappingList->setColumnWidth(1, MM::MAPPING_LIST_NAME_COLUMN);
  mappingList->setColumnWidth(2, MM::MAPPING_LIST_BUTTONS_COLUMN);

  // Create property panel.
  mappingPropertyPanel = new QStackedWidget;
  mappingPropertyPanel->setDisabled(true);
  mappingPropertyPanel->setMinimumHeight(MAPPING_PROPERTY_PANEL_MINIMUM_HEIGHT);

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

  destinationCanvas = new MapperGLCanvas(this, true, nullptr, static_cast<QGLWidget*>(sourceCanvas->viewport()));
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
  paintSplitter = new QSplitter(Qt::Vertical);
  paintSplitter->setChildrenCollapsible(false);
  paintSplitter->addWidget(paintList);
  paintSplitter->addWidget(paintPropertyPanel);

  mappingSplitter = new QSplitter(Qt::Vertical);
  mappingSplitter->setChildrenCollapsible(false);
  mappingSplitter->addWidget(mappingList);
  mappingSplitter->addWidget(mappingPropertyPanel);

  // Content tab.
  contentTab = new QTabWidget;
  contentTab->addTab(paintSplitter, QIcon(":/add-video"), tr("Library"));
  contentTab->addTab(mappingSplitter, QIcon(":/add-mesh"), tr("Layers"));

  canvasSplitter = new QSplitter(Qt::Vertical);
  canvasSplitter->addWidget(sourcePanel);
  canvasSplitter->addWidget(destinationPanel);

  mainSplitter = new QSplitter(Qt::Horizontal);
  mainSplitter->addWidget(canvasSplitter);
  mainSplitter->addWidget(contentTab);
  connect(mainSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(updateMappingListColumnWidth()));

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

  // Connect mapping and paint lists signals and slots.
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
  importMediaAction->setShortcut(Qt::CTRL + Qt::Key_I);
  importMediaAction->setIcon(QIcon(":/add-video"));
  importMediaAction->setToolTip(tr("Import a video or image file..."));
  importMediaAction->setIconVisibleInMenu(false);
  importMediaAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(importMediaAction);
  connect(importMediaAction, SIGNAL(triggered()), this, SLOT(importMedia()));

  // Open camera.
  AddCameraAction = new QAction(tr("Open &Camera Device..."), this);
  AddCameraAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
  AddCameraAction->setIcon(QIcon(":/add-camera"));
  AddCameraAction->setIconVisibleInMenu(false);
  AddCameraAction->setToolTip(tr("Choose your camera device..."));
  AddCameraAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(AddCameraAction);
  connect(AddCameraAction, SIGNAL(triggered()), this, SLOT(openCameraDevice()));

  // Add color.
  addColorAction = new QAction(tr("Add &Color Source..."), this);
  addColorAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_A);
  addColorAction->setIcon(QIcon(":/add-color"));
  addColorAction->setToolTip(tr("Add a color paint..."));
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
  duplicateMappingAction = new QAction(tr("Duplicate Layer"), this);
  duplicateMappingAction->setShortcut(Qt::CTRL + Qt::Key_D);
  duplicateMappingAction->setToolTip(tr("Duplicate layer item"));
  duplicateMappingAction->setIconVisibleInMenu(false);
  duplicateMappingAction->setEnabled(false);
  duplicateMappingAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(duplicateMappingAction);
  connect(duplicateMappingAction, SIGNAL(triggered()), this, SLOT(duplicateMappingItem()));

  // Delete mapping.
  deleteMappingAction = new QAction(tr("Delete Layer"), this);
  deleteMappingAction->setShortcut(QKeySequence::Delete);
  deleteMappingAction->setToolTip(tr("Delete layer item"));
  deleteMappingAction->setIconVisibleInMenu(false);
  deleteMappingAction->setEnabled(false);
  deleteMappingAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(deleteMappingAction);
  connect(deleteMappingAction, SIGNAL(triggered()), this, SLOT(deleteMappingItem()));

  // Rename mapping.
  renameMappingAction = new QAction(tr("Rename Layer"), this);
  renameMappingAction->setShortcut(Qt::Key_F2);
  renameMappingAction->setToolTip(tr("Rename layer item"));
  renameMappingAction->setIconVisibleInMenu(false);
  renameMappingAction->setEnabled(false);
  renameMappingAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(renameMappingAction);
  connect(renameMappingAction, SIGNAL(triggered()), this, SLOT(renameMappingItem()));

  // Lock mapping.
  mappingLockedAction = new QAction(tr("Lock Layer"), this);
  mappingLockedAction->setToolTip(tr("Lock layer item"));
  mappingLockedAction->setIconVisibleInMenu(false);
  mappingLockedAction->setCheckable(true);
  mappingLockedAction->setChecked(false);
  mappingLockedAction->setEnabled(false);
  mappingLockedAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(mappingLockedAction);
  connect(mappingLockedAction, SIGNAL(triggered(bool)), this, SLOT(setMappingItemLocked(bool)));

  // Hide mapping.
  mappingHideAction = new QAction(tr("Hide Layer"), this);
  mappingHideAction->setToolTip(tr("Hide layer item"));
  mappingHideAction->setIconVisibleInMenu(false);
  mappingHideAction->setCheckable(true);
  mappingHideAction->setChecked(false);
  mappingHideAction->setEnabled(false);
  mappingHideAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(mappingHideAction);
  connect(mappingHideAction, SIGNAL(triggered(bool)), this, SLOT(setMappingItemHide(bool)));

  // Solo mapping.
  mappingSoloAction = new QAction(tr("Solo Layer"), this);
  mappingSoloAction->setToolTip(tr("Solo layer item"));
  mappingSoloAction->setIconVisibleInMenu(false);
  mappingSoloAction->setCheckable(true);
  mappingSoloAction->setChecked(false);
  mappingSoloAction->setEnabled(false);
  mappingSoloAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(mappingSoloAction);
  connect(mappingSoloAction, SIGNAL(triggered(bool)), this, SLOT(setMappingItemSolo(bool)));

  // Rotate 90 degrees CW action.
  mappingRotate90CWAction = new QAction(tr("Rotate 90° CW"), this);
  mappingRotate90CWAction->setToolTip(tr("Rotate 90° CW"));
  mappingRotate90CWAction->setIconVisibleInMenu(true);
  mappingRotate90CWAction->setEnabled(false);
  addAction(mappingRotate90CWAction);
  connect(mappingRotate90CWAction, SIGNAL(triggered()), SLOT(transformActionMappingItem()));

  // Rotate 90 degrees CW action.
  mappingRotate90CCWAction = new QAction(tr("Rotate 90° CW"), this);
  mappingRotate90CCWAction->setToolTip(tr("Rotate 90° CW"));
  mappingRotate90CCWAction->setIconVisibleInMenu(true);
  mappingRotate90CCWAction->setEnabled(false);
  addAction(mappingRotate90CCWAction);
  connect(mappingRotate90CCWAction, SIGNAL(triggered()), SLOT(transformActionMappingItem()));

  // Rotate 180 degrees action.
  mappingRotate180Action = new QAction(tr("Rotate 180°"), this);
  mappingRotate180Action->setToolTip(tr("Rotate 180°"));
  mappingRotate180Action->setIconVisibleInMenu(true);
  mappingRotate180Action->setEnabled(false);
  addAction(mappingRotate180Action);
  connect(mappingRotate180Action, SIGNAL(triggered()), SLOT(transformActionMappingItem()));

  // Horizontal Flip Action
  mappingHorizontalFlipAction = new QAction(tr("Flip Horizontally"), this);
  mappingHorizontalFlipAction->setShortcut(Qt::Key_H);
  mappingHorizontalFlipAction->setToolTip(tr("Flip Horizontally"));
  mappingHorizontalFlipAction->setIconVisibleInMenu(true);
  mappingHorizontalFlipAction->setEnabled(false);
  addAction(mappingHorizontalFlipAction);
  connect(mappingHorizontalFlipAction, SIGNAL(triggered()), SLOT(transformActionMappingItem()));

  // Vertical Flip Action
  mappingVerticalFlipAction = new QAction(tr("Flip Vertically"), this);
  mappingVerticalFlipAction->setShortcut(Qt::Key_V);
  mappingVerticalFlipAction->setToolTip(tr("Flip Vertically"));
  mappingVerticalFlipAction->setIconVisibleInMenu(true);
  mappingVerticalFlipAction->setEnabled(false);
  addAction(mappingVerticalFlipAction);
  connect(mappingVerticalFlipAction, SIGNAL(triggered()), SLOT(transformActionMappingItem()));

  mappingRaiseAction = new QAction(tr("Raise"), this);
  mappingRaiseAction->setShortcut(Qt::Key_PageUp);
  mappingRaiseAction->setToolTip(tr("Raise"));
  mappingRaiseAction->setIconVisibleInMenu(true);
  mappingRaiseAction->setEnabled(false);
  addAction(mappingRaiseAction);
  connect(mappingRaiseAction, SIGNAL(triggered()), SLOT(reorderMappingItem()));

  mappingLowerAction = new QAction(tr("Lower"), this);
  mappingLowerAction->setShortcut(Qt::Key_PageDown);
  mappingLowerAction->setToolTip(tr("Lower"));
  mappingLowerAction->setIconVisibleInMenu(true);
  mappingLowerAction->setEnabled(false);
  addAction(mappingLowerAction);
  connect(mappingLowerAction, SIGNAL(triggered()), SLOT(reorderMappingItem()));

  mappingRaiseToTopAction = new QAction(tr("Raise to Top"), this);
  mappingRaiseToTopAction->setShortcut(Qt::Key_Home); // bottom = end
  mappingRaiseToTopAction->setToolTip(tr("Raise to top"));
  mappingRaiseToTopAction->setIconVisibleInMenu(true);
  mappingRaiseToTopAction->setEnabled(false);
  addAction(mappingRaiseToTopAction);
  connect(mappingRaiseToTopAction, SIGNAL(triggered()), SLOT(reorderMappingItem()));

  mappingLowerToBottomAction = new QAction(tr("Lower to Bottom"), this);
  mappingLowerToBottomAction->setShortcut(Qt::Key_End);
  mappingLowerToBottomAction->setToolTip(tr("Lower to bottom"));
  mappingLowerToBottomAction->setIconVisibleInMenu(true);
  mappingLowerToBottomAction->setEnabled(false);
  addAction(mappingLowerToBottomAction);
  connect(mappingLowerToBottomAction, SIGNAL(triggered()), SLOT(reorderMappingItem()));

  // Delete paint.
  deletePaintAction = new QAction(tr("Delete Source"), this);
  //deletePaintAction->setShortcut(tr("CTRL+DEL"));
  deletePaintAction->setToolTip(tr("Delete source item"));
  deletePaintAction->setIconVisibleInMenu(false);
  deletePaintAction->setEnabled(false);
  deletePaintAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(deletePaintAction);
  connect(deletePaintAction, SIGNAL(triggered()), this, SLOT(deletePaintItem()));

  // Rename paint.
  renamePaintAction = new QAction(tr("Rename Source"), this);
  //renamePaintAction->setShortcut(Qt::Key_F2);
  renamePaintAction->setToolTip(tr("Rename source item"));
  renamePaintAction->setIconVisibleInMenu(false);
  renamePaintAction->setEnabled(false);
  renamePaintAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(renamePaintAction);
  connect(renamePaintAction, SIGNAL(triggered()), this, SLOT(renamePaintItem()));

  // Import a new media for current layer
  _importLayerMediaAction = new QAction(tr("Import New Media"), this);
  _importLayerMediaAction->setToolTip(tr("Import new media file if not exists on the list"));
  _importLayerMediaAction->setIconVisibleInMenu(false);
  _importLayerMediaAction->setData("import-new-media"); // Important
  connect(_importLayerMediaAction, SIGNAL(triggered()), this, SLOT(loadLayerMedia()));

  // Preferences...
  preferencesAction = new QAction(tr("&Preferences..."), this);
  //preferencesAction->setIcon(QIcon(":/preferences"));
  preferencesAction->setShortcut(Qt::CTRL + Qt::Key_Comma);
  preferencesAction->setToolTip(tr("Configure preferences..."));
  //preferencesAction->setIconVisibleInMenu(false);
  preferencesAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(preferencesAction);
  connect(preferencesAction, SIGNAL(triggered()), _preferenceDialog, SLOT(exec()));

  // Add mesh.
  addMeshAction = new QAction(tr("Add &Mesh Layer"), this);
  addMeshAction->setShortcut(Qt::CTRL + Qt::Key_M);
  addMeshAction->setIcon(QIcon(":/add-mesh"));
  addMeshAction->setToolTip(tr("Add mesh layer"));
  addMeshAction->setIconVisibleInMenu(false);
  addMeshAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addMeshAction);
  connect(addMeshAction, SIGNAL(triggered()), this, SLOT(addMesh()));
  addMeshAction->setEnabled(false);

  // Add triangle.
  addTriangleAction = new QAction(tr("Add &Triangle Layer"), this);
  addTriangleAction->setShortcut(Qt::CTRL + Qt::Key_T);
  addTriangleAction->setIcon(QIcon(":/add-triangle"));
  addTriangleAction->setToolTip(tr("Add triangle layer"));
  addTriangleAction->setIconVisibleInMenu(false);
  addTriangleAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addTriangleAction);
  connect(addTriangleAction, SIGNAL(triggered()), this, SLOT(addTriangle()));
  addTriangleAction->setEnabled(false);

  // Add ellipse.
  addEllipseAction = new QAction(tr("Add &Ellipse Layer"), this);
  addEllipseAction->setShortcut(Qt::CTRL + Qt::Key_E);
  addEllipseAction->setIcon(QIcon(":/add-ellipse"));
  addEllipseAction->setToolTip(tr("Add ellipse layer"));
  addEllipseAction->setIconVisibleInMenu(false);
  addEllipseAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(addEllipseAction);
  connect(addEllipseAction, SIGNAL(triggered()), this, SLOT(addEllipse()));
  addEllipseAction->setEnabled(false);

  // Play.
  const QKeySequence PLAY_PAUSE_KEY_SEQUENCE = Qt::CTRL + Qt::SHIFT + Qt::Key_P;
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
  rewindAction->setShortcut(Qt::CTRL + Qt::Key_R);
  rewindAction->setIcon(QIcon(":/rewind"));
  rewindAction->setToolTip(tr("Restart"));
  rewindAction->setIconVisibleInMenu(false);
  rewindAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(rewindAction);
  connect(rewindAction, SIGNAL(triggered()), this, SLOT(rewind()));

  // Toggle display of output window.
  outputFullScreenAction = new QAction(tr("Toggle &Fullscreen"), this);
  outputFullScreenAction->setShortcut(Qt::CTRL + Qt::Key_F);
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
  connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), this, SLOT(updateScreenCount()));
  // Create hiden action for closing output window
  QAction *closeOutput = new QAction(this);
  closeOutput->setShortcut(Qt::Key_Escape);
  closeOutput->setShortcutContext(Qt::ApplicationShortcut);
  addAction(closeOutput);
  connect(closeOutput, SIGNAL(triggered(bool)), this, SLOT(exitFullScreen()));

  // Toggle display of canvas controls.
  displayControlsAction = new QAction(tr("&Display Controls"), this);
  displayControlsAction->setShortcut(Qt::ALT + Qt::Key_C);
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
  displayPaintControlsAction = new QAction(tr("&Display Controls of Layers of a Source"), this);
  //displayPaintControlsAction->setShortcut(Qt::ALT + Qt::Key_C);
  displayPaintControlsAction->setIcon(QIcon(":/control-points"));
  displayPaintControlsAction->setToolTip(tr("Display all canvas controls related to current source"));
  displayPaintControlsAction->setIconVisibleInMenu(false);
  displayPaintControlsAction->setCheckable(true);
  displayPaintControlsAction->setChecked(_displayPaintControls);
  displayPaintControlsAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayPaintControlsAction);
  // Manage show/hide of canvas controls.
  connect(displayPaintControlsAction, SIGNAL(toggled(bool)), this, SLOT(enableDisplayPaintControls(bool)));
//  connect(displayPaintControlsAction, SIGNAL(toggled(bool)), outputWindow, SLOT(setDisplayCrosshair(bool)));
  connect(displayControlsAction, SIGNAL(toggled(bool)), displayPaintControlsAction, SLOT(setEnabled(bool)));

  // Toggle sticky vertices
  stickyVerticesAction = new QAction(tr("&Sticky Vertices"), this);
  stickyVerticesAction->setShortcut(Qt::ALT + Qt::Key_S);
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
  displayTestSignalAction->setShortcut(Qt::ALT + Qt::Key_T);
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
  displayUndoHistoryAction->setShortcut(Qt::ALT + Qt::Key_U);
  displayUndoHistoryAction->setCheckable(true);
  displayUndoHistoryAction->setChecked(_displayUndoStack);
  displayUndoHistoryAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(displayUndoHistoryAction);
  // Manage show/hide of Undo History
  connect(displayUndoHistoryAction, SIGNAL(toggled(bool)), this, SLOT(displayUndoHistory(bool)));

  // Toggle display of Console output
  openConsoleAction = new QAction(tr("Open Conso&le"), this);
  openConsoleAction->setShortcut(Qt::ALT + Qt::Key_L);
  openConsoleAction->setCheckable(true);
  openConsoleAction->setChecked(false);
  openConsoleAction->setShortcutContext(Qt::ApplicationShortcut);
  addAction(openConsoleAction);
  connect(openConsoleAction, SIGNAL(toggled(bool)), consoleWindow, SLOT(setVisible(bool)));
  // uncheck action when window is closed
  connect(consoleWindow, SIGNAL(windowClosed()), openConsoleAction, SLOT(toggle()));

  // Toggle display of zoom tool buttons
  displayZoomToolAction = new QAction(tr("Display &Zoom Toolbar"), this);
  displayZoomToolAction->setShortcut(Qt::ALT + Qt::Key_Z);
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
  mainViewAction->setShortcut(Qt::CTRL + Qt::Key_1);
  mainViewAction->setToolTip(tr("Switch to the Main layout."));
  connect(mainViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(0), SLOT(setVisible(bool)));
  connect(mainViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(1), SLOT(setVisible(bool)));
  // Source Only
  sourceViewAction = new QAction(tr("Input editor Layout"), this);
  sourceViewAction->setCheckable(true);
  sourceViewAction->setShortcut(Qt::CTRL + Qt::Key_2);
  sourceViewAction->setToolTip(tr("Switch to the Input editor Layout."));
  connect(sourceViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(0), SLOT(setVisible(bool)));
  connect(sourceViewAction, SIGNAL(triggered(bool)), canvasSplitter->widget(1), SLOT(setHidden(bool)));
  // Destination Only
  destViewAction = new QAction(tr("Output Editor Layout"), this);
  destViewAction->setCheckable(true);
  destViewAction->setShortcut(Qt::CTRL + Qt::Key_3);
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
  resetZoomAction->setShortcut(Qt::CTRL + Qt::Key_0);
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
  shortcutAction->setShortcut(Qt::CTRL + Qt::Key_K);
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
  editMenu->addAction(deletePaintAction);
  editMenu->addAction(renamePaintAction);
  editMenu->addSeparator();
  // Destination canvas menu
  editMenu->addAction(duplicateMappingAction);
  editMenu->addAction(deleteMappingAction);
  editMenu->addAction(renameMappingAction);
  editMenu->addSeparator();
  editMenu->addAction(mappingRaiseAction);
  editMenu->addAction(mappingLowerAction);
  editMenu->addAction(mappingRaiseToTopAction);
  editMenu->addAction(mappingLowerToBottomAction);
  editMenu->addSeparator();
  editMenu->addAction(mappingRotate90CWAction);
  editMenu->addAction(mappingRotate90CCWAction);
  editMenu->addAction(mappingRotate180Action);
  editMenu->addSeparator();
  editMenu->addAction(mappingHorizontalFlipAction);
  editMenu->addAction(mappingVerticalFlipAction);
  editMenu->addSeparator();

  editMenu->addAction(mappingLockedAction);
  editMenu->addAction(mappingHideAction);
  editMenu->addAction(mappingSoloAction);
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
  viewMenu->addAction(displayPaintControlsAction);
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

void MainWindow::createMappingContextMenu()
{
  // Context menu.
  mappingContextMenu = new QMenu(this);
  mappingContextMenu->installEventFilter(this);

  // Add different Action
  mappingContextMenu->addAction(duplicateMappingAction);
  mappingContextMenu->addAction(deleteMappingAction);
  mappingContextMenu->addAction(renameMappingAction);
  // Add a little separator
  mappingContextMenu->addSeparator();

  // Create menu for source list
  _changeLayerMediaMenu = mappingContextMenu->addMenu(tr("Change Layer Source"));

  // Add another separator
  mappingContextMenu->addSeparator();
  mappingContextMenu->addAction(mappingRaiseAction);
  mappingContextMenu->addAction(mappingLowerAction);
  mappingContextMenu->addAction(mappingRaiseToTopAction);
  mappingContextMenu->addAction(mappingLowerToBottomAction);
  mappingContextMenu->addSeparator();
  mappingContextMenu->addAction(mappingRotate90CWAction);
  mappingContextMenu->addAction(mappingRotate90CCWAction);
  mappingContextMenu->addAction(mappingRotate180Action);
  mappingContextMenu->addSeparator();
  mappingContextMenu->addAction(mappingHorizontalFlipAction);
  mappingContextMenu->addAction(mappingVerticalFlipAction);

  mappingContextMenu->addSeparator();
  mappingContextMenu->addAction(mappingLockedAction);
  mappingContextMenu->addAction(mappingHideAction);
  mappingContextMenu->addAction(mappingSoloAction);

  // Set context menu policy
  mappingList->setContextMenuPolicy(Qt::CustomContextMenu);
  destinationCanvas->setContextMenuPolicy(Qt::CustomContextMenu);
  outputWindow->setContextMenuPolicy(Qt::CustomContextMenu);

  // Context Menu Connexions
  connect(mappingItemDelegate, SIGNAL(itemContextMenuRequested(const QPoint&)),
          this, SLOT(showMappingContextMenu(const QPoint&)), Qt::QueuedConnection);
  connect(destinationCanvas, SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showMappingContextMenu(const QPoint&)));
  connect(outputWindow->getCanvas(), SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showMappingContextMenu(const QPoint&)));
}

void MainWindow::createPaintContextMenu()
{
  // Paint Context Menu
  paintContextMenu = new QMenu(this);
  paintContextMenu->installEventFilter(this);

  // Add Actions
  paintContextMenu->addAction(deletePaintAction);
  paintContextMenu->addAction(renamePaintAction);

  // Define Context policy
  paintList->setContextMenuPolicy(Qt::CustomContextMenu);
  sourceCanvas->setContextMenuPolicy(Qt::CustomContextMenu);

  // Connexions
  connect(paintList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showPaintContextMenu(const QPoint&)));
  connect(sourceCanvas, SIGNAL(shapeContextMenuRequested(const QPoint&)), this, SLOT(showPaintContextMenu(const QPoint&)));
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
  paintSplitter->restoreState(settings.value("paintSplitter").toByteArray());
  mappingSplitter->restoreState(settings.value("mappingSplitter").toByteArray());
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
   displayPaintControlsAction->setChecked(settings.value("displayAllControls", MM::DISPLAY_ALL_CONTROLS).toBool());
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
  settings.setValue("paintSplitter", paintSplitter->saveState());
  settings.setValue("mappingSplitter", mappingSplitter->saveState());
  settings.setValue("canvasSplitter", canvasSplitter->saveState());
  settings.setValue("outputWindow", outputWindow->saveGeometry());
  settings.setValue("displayOutputWindow", outputFullScreenAction->isChecked());
  settings.setValue("displayTestSignal", displayTestSignalAction->isChecked());
  settings.setValue("displayControls", displayControlsAction->isChecked());
  settings.setValue("displayAllControls", displayPaintControlsAction->isChecked());
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

  if (paintList->count() > 1) { // No need to load the same video
    for (auto i = 0; i < paintList->count(); i++) {
      QAction *mediaAction = new QAction(this);
      mediaAction->setText(tr("&%1 %2").arg(i + 1).arg(mappingManager->getPaint(i)->getName()));
      mediaAction->setData(mappingManager->getPaint(i)->getId());
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
  if (mappingListModel->rowCount() < 1) {
    duplicateMappingAction->setEnabled(false);
    deleteMappingAction->setEnabled(false);
    renameMappingAction->setEnabled(false);
    mappingLockedAction->setEnabled(false);
    mappingHideAction->setEnabled(false);
    mappingSoloAction->setEnabled(false);
    mappingRotate90CWAction->setEnabled(true);
    mappingRotate90CCWAction->setEnabled(true);
    mappingRotate180Action->setEnabled(true);
    mappingHorizontalFlipAction->setEnabled(false);
    mappingVerticalFlipAction->setEnabled(false);
    mappingRaiseAction->setEnabled(false);
    mappingLowerAction->setEnabled(false);
    mappingRaiseToTopAction->setEnabled(false);
    mappingLowerToBottomAction->setEnabled(false);
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
    if (file.isSequential()) {
      type = VIDEO_SHMSRC;
    }
    else {
      QMessageBox::warning(this, tr("MapMap Project"),
                           tr("Cannot read file %1:\n%2.")
                           .arg(file.fileName())
                           .arg(file.errorString()));
      return false;
    }
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add media file to model.
  uint mediaId = createMediaPaint(NULL_UID, fileName, 0, 0, isImage, type);

  // Initialize position (center).
  QSharedPointer<Video> media = qSharedPointerCast<Video>(mappingManager->getPaintById(mediaId));
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

bool MainWindow::addColorPaint(const QColor& color)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add color to model.
  uint colorId = createColorPaint(NULL_UID, color);

  // Initialize position (center).
  QSharedPointer<Color> colorPaint = qSharedPointerCast<Color>(mappingManager->getPaintById(colorId));
  Q_CHECK_PTR(colorPaint);

  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("Color source added"), 2000);

  return true;
}

void MainWindow::addPaintItem(uid paintId, const QIcon& icon, const QString& name)
{
  Paint::ptr paint = mappingManager->getPaintById(paintId);
  Q_CHECK_PTR(paint);

  // Create paint gui.
  PaintGui::ptr paintGui;
  SourceType paintType = paint->getSourceType();
  if (paintType == SourceType::Video)
    paintGui = PaintGui::ptr(new VideoGui(paint));
  else if (paintType == SourceType::Image)
    paintGui = PaintGui::ptr(new ImageGui(paint));
  else if (paintType == SourceType::Color)
    paintGui = PaintGui::ptr(new ColorGui(paint));
  else
    paintGui = PaintGui::ptr(new PaintGui(paint));

  // Add to list of paint guis..
  paintGuis[paintId] = paintGui;
  QWidget* paintEditor = paintGui->getPropertiesEditor();
  paintPropertyPanel->addWidget(paintEditor);
  paintPropertyPanel->setCurrentWidget(paintEditor);
  paintPropertyPanel->setEnabled(true);

  // When paint value is changed, update canvases.
  //  connect(paintGui.get(), SIGNAL(valueChanged()),
  //          this,           SLOT(updateCanvases()));

  connect(paintGui.data(), SIGNAL(valueChanged(Paint::ptr)),
          this,            SLOT(handlePaintChanged(Paint::ptr)));

  connect(paint.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(paintPropertyChanged(uid, QString, QVariant)));

  // TODO: attention: if mapping is invisible canvases will be updated for no reason
  connect(paint.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(updateCanvases()));

  // Add paint item to paintList widget.
  QListWidgetItem* item = new QListWidgetItem(icon, name);
  setItemId(*item, paintId); // TODO: could possibly be replaced by a Paint pointer

  // Set size.
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::PAINT_LIST_ITEM_HEIGHT));

  // Set tooltip.
  item->setToolTip(QString("ID: %1").arg(paint->getId()));

  // Switch to paint tab.
  contentTab->setCurrentWidget(paintSplitter);

  // Add item to paint list.
  paintList->addItem(item);
  paintList->setCurrentItem(item);

  // Update mapping guis.
  updateMappers();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::updatePaintItem(uid paintId, const QIcon& icon, const QString& name) {
  QListWidgetItem* item = getItemFromId(*paintList, paintId);
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

void MainWindow::addMappingItem(uid mappingId)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  QString defaultName;
  QIcon icon;

  ShapeType shapeType = mapping->getShape()->getType();
  SourceType paintType = mapping->getPaint()->getSourceType();

  // Add mapper.
  // XXX hardcoded for textures
  QSharedPointer<TextureMapping> textureMapping;
  if (paintType == SourceType::Video || paintType == SourceType::Image)
  {
    textureMapping = qSharedPointerCast<TextureMapping>(mapping);
    Q_CHECK_PTR(textureMapping);
  }

  MappingGui::ptr mapper;

  // XXX Branching on nVertices() is crap

  // Triangle
  if (shapeType == ShapeType::Triangle)
  {
    defaultName = QString("Triangle %1").arg(mappingId);
    icon = QIcon(":/shape-triangle");

    if (paintType == SourceType::Color)
      mapper = MappingGui::ptr(new PolygonColorMappingGui(mapping));
    else
      mapper = MappingGui::ptr(new TriangleTextureMappingGui(textureMapping));
  }
  // Mesh
  else if (shapeType == ShapeType::Mesh)
  {
    defaultName = QString("Mesh %1").arg(mappingId);
    icon = QIcon(":/shape-mesh");
    if (paintType == SourceType::Color)
      mapper = MappingGui::ptr(new MeshColorMappingGui(mapping));
    else
      mapper = MappingGui::ptr(new MeshTextureMappingGui(textureMapping));
  }
  else if (shapeType == ShapeType::Ellipse)
  {
    defaultName = QString("Ellipse %1").arg(mappingId);
    icon = QIcon(":/shape-ellipse");
    if (paintType == SourceType::Color)
      mapper = MappingGui::ptr(new EllipseColorMappingGui(mapping));
    else
      mapper = MappingGui::ptr(new EllipseTextureMappingGui(textureMapping));
  }
  else
  {
    defaultName = QString("Polygon %1").arg(mappingId);
    icon = QIcon(":/shape-polygon");
  }

  // Label is only going to be applied if no name is present.
  if (mapping->getName().isEmpty())
    mapping->setName(defaultName);

  // Add to list of mappers.
  mappers[mappingId] = mapper;
  QWidget* mapperEditor = mapper->getPropertiesEditor();
  mappingPropertyPanel->addWidget(mapperEditor);
  mappingPropertyPanel->setCurrentWidget(mapperEditor);
  mappingPropertyPanel->setEnabled(true);

  // When mapper value is changed, update canvases.
  connect(mapper.data(), SIGNAL(valueChanged()),
          this,          SLOT(updateCanvases()));

  // Also update playing state in case paint was changed.
  connect(mapper.data(), SIGNAL(paintChanged()),
          this,          SLOT(updatePlayingState()));

  connect(sourceCanvas,  SIGNAL(shapeChanged(MShape*)),
          mapper.data(), SLOT(updateShape(MShape*)));

  connect(destinationCanvas, SIGNAL(shapeChanged(MShape*)),
          mapper.data(),     SLOT(updateShape(MShape*)));

  connect(mapping.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(mappingPropertyChanged(uid, QString, QVariant)));

  // TODO: attention: if mapping is invisible canvases will be updated for no reason
  connect(mapping.data(), SIGNAL(propertyChanged(uid, QString, QVariant)),
          this,           SLOT(updateCanvases()));

  // Switch to mapping tab.
  contentTab->setCurrentWidget(mappingSplitter);

  // Add item to layerList widget.
  mappingListModel->addItem(mapping, icon, mapping->getName());
  mappingListModel->updateModel();
  setCurrentMapping(mappingId);

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

void MainWindow::removeMappingItem(uid mappingId)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  // Remove mapping from model.
  mappingManager->removeMapping(mappingId);

  // Remove associated mapper.
  mappingPropertyPanel->removeWidget(mappers[mappingId]->getPropertiesEditor());
  mappers.remove(mappingId);

  // Remove widget from mappingList.
  int row = mappingListModel->getItemRowFromId(mappingId);
  Q_ASSERT( row >= 0 );
  mappingListModel->removeItem(row);

  // Update list.
  mappingListModel->updateModel();

  if (mappingListModel->rowCount() == 0)
    removeCurrentMapping();
  else
  {
    int nextSelectedRow = row == mappingListModel->rowCount() ? row - 1 : row;
    QModelIndex index = mappingListModel->getIndexFromRow(nextSelectedRow);
    mappingList->selectionModel()->select(index, QItemSelectionModel::Select);
    mappingList->setCurrentIndex(index);
  }

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::moveMappingItem(uid mappingId, int idx)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  // Remove mapping from model.
  uid exchangeMappingId = mappingManager->getMapping(idx)->getId();
  mappingManager->moveMapping(mappingId, idx);

  // Remove widget from mappingList.
  int row = mappingListModel->getItemRowFromId(mappingId);
  int rowTo = mappingListModel->getItemRowFromId(exchangeMappingId);
  Q_ASSERT( row >= 0 );
  mappingListModel->moveItem(row, idx);

  // Update list.
  mappingListModel->updateModel();

  QModelIndex index = mappingListModel->getIndexFromRow(idx);
  mappingList->selectionModel()->select(index, QItemSelectionModel::Select);
  mappingList->setCurrentIndex(index);

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();

  // Update playing state.
  updatePlayingState();
}

void MainWindow::removePaintItem(uid paintId)
{
  Paint::ptr paint = mappingManager->getPaintById(paintId);
  Q_CHECK_PTR(paint);

  // Remove all mappings associated with paint.
  QMap<uid, Mapping::ptr> paintMappings = mappingManager->getPaintMappings(paint);
  for (QMap<uid, Mapping::ptr>::const_iterator it = paintMappings.constBegin();
       it != paintMappings.constEnd(); ++it) {
    removeMappingItem(it.key());
  }
  // Remove paint from model.
  Q_ASSERT( mappingManager->removePaint(paintId) );

  // Remove associated mapper.
  paintPropertyPanel->removeWidget(paintGuis[paintId]->getPropertiesEditor());
  paintGuis.remove(paintId);

  updateMappers();

  // Remove widget from paintList.
  int row = getItemRowFromId(*paintList, paintId);
  Q_ASSERT( row >= 0 );
  QListWidgetItem* item = paintList->takeItem(row);
  if (item == currentSelectedItem)
    currentSelectedItem = NULL;
  delete item;

  // Update list.
  paintList->update();

  // Reset current paint.
  removeCurrentPaint();

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();
  // Build mapping!
  // FIXME: mapping->build(); // I removed this 2014-04-25

  // Update playing state.
  updatePlayingState();
}

void MainWindow::clearWindow()
{
  clearProject();
}

void MainWindow::syncMappingManager()
{
  // Reorder mappings.
  QVector<uid> newOrder;
  for (int row=0; row<mappingListModel->rowCount(); row++)
//  for (int row=mappingListModel->rowCount()-1; row>=0; row--)
  {
    uid layerId = mappingListModel->getIndexFromRow(row).data(Qt::UserRole).toInt();
    newOrder.push_back(layerId);
  }
  mappingManager->reorderMappings(newOrder);
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
  for (QMap<uid, MappingGui::ptr>::iterator it = mappers.begin();
       it != mappers.end(); ++it) {
    it.value()->updatePaints();
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
  // Pause all paints that are not visible.
  if (isPlaying())
  {
    QVector<Paint::ptr> visiblePaints = mappingManager->getVisiblePaints();
    for (int i=0; i<mappingManager->nPaints(); i++)
    {
      Paint::ptr paint = mappingManager->getPaint(i);
      if (visiblePaints.contains(paint))
      {
        paint->play();
      }
      else
      {
        paint->pause();
      }
    }
  }

  // Pause everyone.
  else
  {
    for (int i=0; i<mappingManager->nPaints(); i++)
    {
      mappingManager->getPaint(i)->pause();
    }
  }

  // Update all paint items with correct icon according to playing state.
  for (int i=0; i<mappingManager->nPaints(); i++)
  {
    Paint::ptr paint = mappingManager->getPaint(i);
    updatePaintItem(paint->getId(), getPaintIcon(paint), paint->getName());
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

void MainWindow::enableDisplayPaintControls(bool display)
{
  _displayPaintControls = display;
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

void MainWindow::showMappingContextMenu(const QPoint &point)
{
  QWidget *objectSender = static_cast<QWidget*>(sender());
  uid mappingId = currentMappingItemId();
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);

  // Switch to right action check state
  mappingLockedAction->setChecked(mapping->isLocked());
  mappingHideAction->setChecked(!mapping->isVisible());
  mappingSoloAction->setChecked(mapping->isSolo());

  if (objectSender != nullptr) {
    if (sender() == mappingItemDelegate) // XXX: The item delegate is not a widget
      mappingContextMenu->exec(mappingList->mapToGlobal(point));
    else
      mappingContextMenu->exec(objectSender->mapToGlobal(point));
  }
}

void MainWindow::showPaintContextMenu(const QPoint &point)
{
  QWidget *objectSender = dynamic_cast<QWidget*>(sender());

  if (objectSender != nullptr && paintList->count() > 0)
    paintContextMenu->exec(objectSender->mapToGlobal(point));
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
  for (int i=0; i<mappingManager->nPaints(); i++)
    mappingManager->getPaint(i)->rewind();
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

const QIcon MainWindow::getPaintIcon(Paint::ptr paint)
{
  if (paint->isPlaying())
    return paint->getIcon();
  else
  {
    QPixmap pixmap = paint->getIcon().pixmap(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(255, 0, 0, 128), 4));
    painter.drawLine(0, 0, pixmap.width(), pixmap.height());
    return QIcon(pixmap);
  }
}

void MainWindow::connectProjectWidgets()
{
  connect(paintList, SIGNAL(itemSelectionChanged()),
          this,      SLOT(handlePaintItemSelectionChanged()));

  connect(paintList, SIGNAL(itemPressed(QListWidgetItem*)),
          this,      SLOT(handlePaintItemSelected(QListWidgetItem*)));

  connect(paintList, SIGNAL(itemActivated(QListWidgetItem*)),
          this,      SLOT(handlePaintItemSelected(QListWidgetItem*)));
  // Rename Paint with double click
  connect(paintList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this,      SLOT(renamePaintItem()));
  // When finish to edit mapping item
  connect(paintList->itemDelegate(), SIGNAL(commitData(QWidget*)),
          this, SLOT(paintListEditEnd(QWidget*)));

  connect(mappingList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleMappingItemSelectionChanged(QModelIndex)));

  connect(mappingListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleMappingItemChanged(QModelIndex)));

  connect(mappingListModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this,                 SLOT(handleMappingIndexesMoved()));

  connect(mappingItemDelegate, SIGNAL(itemDuplicated(uid)),
          this, SLOT(duplicateMapping(uid)));

  connect(mappingItemDelegate, SIGNAL(itemRemoved(uid)),
          this, SLOT(deleteMapping(uid)));

  connect(_preferenceDialog, SIGNAL(settingSaved()), this, SLOT(updateSettings()));
}

void MainWindow::disconnectProjectWidgets()
{
  disconnect(paintList, SIGNAL(itemSelectionChanged()),
             this,      SLOT(handlePaintItemSelectionChanged()));

  disconnect(paintList, SIGNAL(itemPressed(QListWidgetItem*)),
             this,      SLOT(handlePaintItemSelected(QListWidgetItem*)));

  disconnect(paintList, SIGNAL(itemActivated(QListWidgetItem*)),
             this,      SLOT(handlePaintItemSelected(QListWidgetItem*)));

  disconnect(mappingList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleMappingItemSelectionChanged(QModelIndex)));

  disconnect(mappingListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this,        SLOT(handleMappingItemChanged(QModelIndex)));

  disconnect(mappingListModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this,                 SLOT(handleMappingIndexesMoved()));

  disconnect(mappingItemDelegate, SIGNAL(itemDuplicated(uid)),
          this, SLOT(duplicateMapping(uid)));

  disconnect(mappingItemDelegate, SIGNAL(itemRemoved(uid)),
          this, SLOT(deleteMapping(uid)));

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

uid MainWindow::currentMappingItemId() const
{
  return mappingListModel->getItemId(currentSelectedIndex);
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


void MainWindow::setCurrentPaint(int uid)
{
  if (uid == NULL_UID)
    removeCurrentPaint();
  else {
    if (currentPaintId != uid) {
      currentPaintId = uid;
      paintList->setCurrentRow( getItemRowFromId(*paintList, uid) );
      paintPropertyPanel->setCurrentWidget(paintGuis[uid]->getPropertiesEditor());
    }
    _hasCurrentPaint = true;
  }
}

void MainWindow::setCurrentMapping(int uid)
{
  if (uid == NULL_UID)
    removeCurrentMapping();
  else {
    if (currentMappingId != uid) {
      currentMappingId = uid;
      currentSelectedIndex = mappingListModel->getIndexFromRow(mappingListModel->getItemRowFromId(uid));
      mappingList->setCurrentIndex(currentSelectedIndex);
      mappingPropertyPanel->setCurrentWidget(mappers[uid]->getPropertiesEditor());
    }
    _hasCurrentMapping = true;
  }
}

void MainWindow::removeCurrentPaint() {
  _hasCurrentPaint = false;
  currentPaintId = NULL_UID;
  paintList->clearSelection();
}

void MainWindow::removeCurrentMapping() {
  _hasCurrentMapping = false;
  currentMappingId = NULL_UID;
  mappingList->clearSelection();
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
    qWarning() << "OSC port is out of range: " << port << endl;
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
    qWarning() << "OSC port is not a number: " << portNumber << endl;
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

void MainWindow::updateMappingListColumnWidth()
{
  mappingList->setColumnWidth(1, mappingList->horizontalHeader()->width() - (MM::MAPPING_LIST_HIDE_COLUMN + MM::MAPPING_LIST_BUTTONS_COLUMN));
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
