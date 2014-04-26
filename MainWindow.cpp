/*
 * MainWindow.cpp
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

#include "MainWindow.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include "Facade.h"
#include <sstream>

MainWindow::MainWindow()
{
  // Create model.
  if (Media::hasVideoSupport())
    std::cout << "Video support: yes" << std::endl;
  else
    std::cout << "Video support: no" << std::endl;

  mappingManager = new MappingManager;

  // Initialize internal variables.
  currentPaintId = NULL_UID;
  currentMappingId = NULL_UID;
  // TODO: not sure we need this anymore since we have NULL_UID
  _hasCurrentPaint = false;
  _hasCurrentMapping = false;
  currentSelectedItem = NULL;

  // Create everything.
  videoTimer = new QTimer(this);
  videoTimer->setInterval(1000/30);
  connect(videoTimer, SIGNAL(timeout()), this, SLOT(updateCanvases()));
  videoTimer->start();

  createLayout();
  createActions();
  createMenus();
  createContextMenu();
  createToolBars();
  createStatusBar();

  // Load settings.
  readSettings();

  // Start osc.
  startOscReceiver();

  // Defaults.
  //setWindowIcon(QIcon(":/images/icon.png"));
  setCurrentFile("");
}

MainWindow::~MainWindow()
{
  delete mappingManager;
//  delete _facade;
#ifdef HAVE_OSC
  delete osc_timer;
#endif // ifdef
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
    uid idx = getItemId(*item);
    setCurrentPaint(idx);
    removeCurrentMapping();
  }
  else
    removeCurrentPaint();

  // Enable/disable creation of mappings depending on whether a paint is selected.
  addMeshAction->setEnabled(paintItemSelected);
  addTriangleAction->setEnabled(paintItemSelected);
  addEllipseAction->setEnabled(paintItemSelected);

  // Update canvases.
  updateCanvases();
}

void MainWindow::handleMappingItemSelectionChanged()
{
  // Get current mapping.
  QListWidgetItem* item = mappingList->currentItem();
  currentSelectedItem = item;
  if (item)
  {
    // Get index.
    uid idx = getItemId(*item);

    // Set current paint and mappings.
    Mapping::ptr mapping = mappingManager->getMappingById(idx);
    setCurrentPaint(mapping->getPaint()->getId());
    setCurrentMapping(mapping->getId());
  }
  else
    removeCurrentMapping();

  // Update canvases.
  updateCanvases();
}

void MainWindow::handleMappingItemChanged(QListWidgetItem* item)
{
  // Toggle visibility of mapping depending on checkbox of item.
  uid mappingId = getItemId(*item);
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  mapping->setVisible(item->checkState() == Qt::Checked);

  // Update canvases.
  updateCanvases();
}

void MainWindow::handleMappingIndexesMoved()
{
  // Reorder mappings.
  QVector<uid> newOrder;
  for (int row=mappingList->count()-1; row>=0; row--)
  {
    uid layerId = mappingList->item(row)->data(Qt::UserRole).toInt();
    newOrder.push_back(layerId);
  }
  mappingManager->reorderMappings(newOrder);

  // Update canvases according to new order.
  updateCanvases();
}

void MainWindow::handleItemSelected(QListWidgetItem* item)
{
  // Change currently selected item.
  currentSelectedItem = item;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Popup dialog allowing the user to save before closing.
  if (okToContinue())
  {
    writeSettings();
    event->accept();
  }
  else
  {
    event->ignore();
  }
}

void MainWindow::newFile()
{
  // Popup dialog allowing the user to save before creating a new file.
  if (okToContinue())
  {
    clearWindow();
    setCurrentFile("");
  }
}

void MainWindow::open()
{
  // Popup dialog allowing the user to save before opening a new file.
  if (okToContinue())
  {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open project"), ".", tr("MapMap files (*.lmp)"));
    if (!fileName.isEmpty())
      loadFile(fileName);
  }
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
  // Popul file dialog to choose filename.
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save project"),
      ".", tr("MapMap files (*.lmp)"));
  if (fileName.isEmpty())
    return false;

  // Save to filename.
  return saveFile(fileName);
}

void MainWindow::import()
{
  // Pop-up file-choosing dialog to choose media file.
  // TODO: restrict the type of files that can be imported
  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Import media source file"), ".");
  if (!fileName.isEmpty())
    importMediaFile(fileName);
}

void MainWindow::addColor()
{
  // Pop-up color-choosing dialog to choose color paint.
  QColor initialColor;
  QColor color = QColorDialog::getColor(initialColor, this);
  if (color.isValid())
    addColorPaint(color);
}

void MainWindow::addMesh()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaint(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output quads.
  Mapping* mappingPtr;
  if (paint->getType() == "color")
  {
    Shape::ptr outputQuad = Shape::ptr(Util::createQuadForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputQuad);
  }
  else
  {
    std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
    Q_CHECK_PTR(texture);

    Shape::ptr outputQuad = Shape::ptr(Util::createMeshForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    Shape::ptr  inputQuad = Shape::ptr(Util::createMeshForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, outputQuad, inputQuad);
  }

  // Create texture mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);
  addMappingItem(mappingId);
}

void MainWindow::addTriangle()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaint(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output quads.
  Mapping* mappingPtr;
  if (paint->getType() == "color")
  {
    Shape::ptr outputTriangle = Shape::ptr(Util::createTriangleForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputTriangle);
  }
  else
  {
    std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
    Q_CHECK_PTR(texture);

    Shape::ptr outputTriangle = Shape::ptr(Util::createTriangleForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    Shape::ptr inputTriangle = Shape::ptr(Util::createTriangleForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, inputTriangle, outputTriangle);
  }

  // Create mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);
  addMappingItem(mappingId);
}

void MainWindow::addEllipse()
{
  // A paint must be selected to add a mapping.
  if (getCurrentPaintId() == NULL_UID)
    return;

  // Retrieve current paint (as texture).
  Paint::ptr paint = getMappingManager().getPaint(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  // Create input and output ellipses.
  Mapping* mappingPtr;
  if (paint->getType() == "color")
  {
    Shape::ptr outputEllipse = Shape::ptr(Util::createEllipseForColor(sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new ColorMapping(paint, outputEllipse);
  }
  else
  {
    std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
    Q_CHECK_PTR(texture);

    Shape::ptr outputEllipse = Shape::ptr(Util::createEllipseForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    Shape::ptr inputEllipse = Shape::ptr(Util::createEllipseForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
    mappingPtr = new TextureMapping(paint, inputEllipse, outputEllipse);
  }

  // Create mapping.
  Mapping::ptr mapping(mappingPtr);
  uint mappingId = mappingManager->addMapping(mapping);
  addMappingItem(mappingId);
}

void MainWindow::about()
{
  // Pop-up about dialog.
  QMessageBox::about(this, tr("About MapMap"),
      tr("<h2>MapMap "
          MAPMAP_VERSION
          "</h2>"
          "<p>Copyright &copy; 2013 Sofian Audry, Alexandre Quessy, Julien Keable, "
          "Mike Latona and Vasilis Liaskovitis."
          "<p>MapMap is a free software for video mapping. "
          "<p>Projection mapping, also known as video mapping and spatial augmented reality, "
          "is a projection technology used to turn objects, often irregularly shaped, into "
          "a display surface for video projection. These objects may be complex industrial "
          "landscapes, such as buildings. By using specialized software, a two or three "
          "dimensional object is spatially mapped on the virtual program which mimics the "
          "real environment it is to be projected on. The software can interact with a "
          "projector to fit any desired image onto the surface of that object. This "
          "technique is used by artists and advertisers alike who can add extra dimensions, "
          "optical illusions, and notions of movement onto previously static objects. The "
          "video is commonly combined with, or triggered by, audio to create an "
          "audio-visual narrative."
          "This project was made possible by the support of the International Organization of "
          "La Francophonie."
          "http://www.francophonie.org/"
          ));
}

void MainWindow::updateStatusBar()
{
  // Nothing to do for now.
//  locationLabel->setText(spreadsheet->currentLocation());
//  formulaLabel->setText(spreadsheet->currentFormula());
}

void MainWindow::deleteItem()
{
  if (currentSelectedItem)
  {
    if (currentSelectedItem->listWidget() == mappingList)
    {
      // Delete mapping.
      deleteMapping( getItemId(*mappingList->currentItem()) );
      //currentSelectedItem = NULL;
    }
    else if (currentSelectedItem->listWidget() == paintList)
    {
      // Delete paint.
      deletePaint( getItemId(*paintList->currentItem()) );
      //currentSelectedItem = NULL;
    }
    else
      qCritical() << "Selected item neither a mapping nor a paint." << endl;
  }
}

bool MainWindow::clearProject()
{
  // Disconnect signals to avoid problems when clearning mappingList and paintList.
  disconnectProjectWidgets();

  // Clear current paint / mapping.
  removeCurrentPaint();
  removeCurrentMapping();

  // Empty list widgets.
  mappingList->clear();
  paintList->clear();

  // Clear property panel.
  for (int i=propertyPanel->count()-1; i>=0; i--)
    propertyPanel->removeWidget(propertyPanel->widget(i));

  // Disable property panel.
  propertyPanel->setDisabled(true);

  // Clear list of mappers.
  mappers.clear();

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

uid MainWindow::createMediaPaint(uid paintId, QString uri, float x, float y)
{
  // Cannot create image with already existing id.
  if (Paint::getUidAllocator().exists(paintId))
    return NULL_UID;

  else
  {
    Media* img = new Media(uri, paintId);
//    Image* img = new Image(uri, paintId);

    // Create new image with corresponding ID.
    img->setPosition(x, y);

    // Add it to the manager.
    Paint::ptr paint(img);

    // Add paint to model and return its uid.
    uid id = mappingManager->addPaint(paint);

    // Add paint widget item.
    addPaintItem(id, QIcon(uri), strippedName(uri));

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

    // Add paint to model and return its uid.
    uid id = mappingManager->addPaint(paint);

    // Create a small icon with the color.
    QPixmap pixmap(100,100);
    pixmap.fill(color);

    // Add paint widget item.
    addPaintItem(id, QIcon(pixmap), strippedName(color.name()));

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

    Shape::ptr inputMesh( new Mesh(src, nColumns, nRows));
    Shape::ptr outputMesh(new Mesh(dst, nColumns, nRows));

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

    Shape::ptr inputTriangle( new Triangle(src[0], src[1], src[2]));
    Shape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

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

    Shape::ptr inputEllipse( new Ellipse(src[0], src[1], src[2], dst[3], dst[4]));
    Shape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3], dst[4]));

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

    Shape::ptr outputQuad(new Quad(dst[0], dst[1], dst[2], dst[3]));

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

    Shape::ptr outputTriangle(new Triangle(dst[0], dst[1], dst[2]));

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

    Shape::ptr outputEllipse(new Ellipse(dst[0], dst[1], dst[2], dst[3]));

    // Add it to the manager.
    Mapping::ptr mapping(new ColorMapping(paint, outputEllipse, mappingId));
    uid id = mappingManager->addMapping(mapping);

    // Add it to the GUI.
    addMappingItem(mappingId);

    // Return the id.
    return id;
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

/// Deletes/removes a paint and all associated mappigns.
void MainWindow::deletePaint(uid paintId)
{
  // Cannot delete unexisting paint.
  if (Paint::getUidAllocator().exists(paintId))
  {
    int r = QMessageBox::warning(this, tr("MapMap"),
        tr("Remove this paint and all its associated mappings?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (r == QMessageBox::Ok)
    {
      removePaintItem(paintId);
    }
  }
}

void MainWindow::windowModified()
{
  setWindowModified(true);
  updateStatusBar();
}

void MainWindow::createLayout()
{
  // Create paint list.
  paintList = new QListWidget;
  paintList->setSelectionMode(QAbstractItemView::SingleSelection);
  paintList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  paintList->setDefaultDropAction(Qt::MoveAction);
  paintList->setDragDropMode(QAbstractItemView::InternalMove);
  paintList->setMinimumWidth(PAINT_LIST_MINIMUM_WIDTH);

  // Create mapping list.
  mappingList = new QListWidget;
  mappingList->setSelectionMode(QAbstractItemView::SingleSelection);
  mappingList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  //layerList->setDragDropMode(QAbstractItemView::DragDrop);
  mappingList->setDefaultDropAction(Qt::MoveAction);
  mappingList->setDragDropMode(QAbstractItemView::InternalMove);
  mappingList->setMinimumWidth(MAPPING_LIST_MINIMUM_WIDTH);

  // Create property panel.
  propertyPanel = new QStackedWidget;
  propertyPanel->setDisabled(true);
  propertyPanel->setMinimumWidth(PROPERTY_PANEL_MINIMUM_WIDTH);

  // Create canvases.

  sourceCanvas = new SourceGLCanvas(this);
  sourceCanvas->setFocusPolicy(Qt::ClickFocus);
  sourceCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sourceCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);

  destinationCanvas = new DestinationGLCanvas(this, 0, sourceCanvas);
  destinationCanvas->setFocusPolicy(Qt::ClickFocus);
  destinationCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  destinationCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);

  outputWindow = new OutputGLWindow(this, this, sourceCanvas);
  outputWindow->setVisible(true);

  // Source changed -> change destination
  connect(sourceCanvas,      SIGNAL(shapeChanged(Shape*)),
          destinationCanvas, SLOT(updateCanvas()));

  // Source changed -> change output window
  connect(sourceCanvas,              SIGNAL(shapeChanged(Shape*)),
          outputWindow->getCanvas(), SLOT(updateCanvas()));

  // Destination changed -> change output window
  connect(destinationCanvas,         SIGNAL(shapeChanged(Shape*)),
          outputWindow->getCanvas(), SLOT(updateCanvas()));

  // Output changed -> change destinatioin
  connect(outputWindow->getCanvas(), SIGNAL(shapeChanged(Shape*)),
          destinationCanvas,         SLOT(updateCanvas()));

  // Create layout.

  resourceSplitter = new QSplitter(Qt::Horizontal);
  resourceSplitter->addWidget(paintList);
  resourceSplitter->addWidget(mappingList);
  resourceSplitter->addWidget(propertyPanel);

  canvasSplitter = new QSplitter(Qt::Horizontal);
  canvasSplitter->addWidget(sourceCanvas);
  canvasSplitter->addWidget(destinationCanvas);

  mainSplitter = new QSplitter(Qt::Vertical);
  mainSplitter->addWidget(canvasSplitter);
  mainSplitter->addWidget(resourceSplitter);

  // Initialize size to 2:1 proportions.
  QSize sz = mainSplitter->size();
  QList<int> sizes;
  sizes.append(sz.height() * 2 / 3);
  sizes.append(sz.height() - sizes.at(0));
  mainSplitter->setSizes(sizes);

  // Upon resizing window, give some extra stretch expansion to canvasSplitter.
  //mainSplitter->setStretchFactor(0, 1);

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
  newAction->setIcon(QIcon(":/images/document-new-4.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setStatusTip(tr("Create a new project"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  // Open.
  openAction = new QAction(tr("&Open..."), this);
  openAction->setIcon(QIcon(":/images/document-open-3.png"));
  openAction->setShortcut(QKeySequence::Open);
  openAction->setStatusTip(tr("Open an existing project"));
  connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

  // Save.
  saveAction = new QAction(tr("&Save"), this);
  saveAction->setIcon(QIcon(":/images/document-save-2.png"));
  saveAction->setShortcut(QKeySequence::Save);
  saveAction->setStatusTip(tr("Save the project"));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

  // Save as.
  saveAsAction = new QAction(tr("Save &As..."), this);
  saveAsAction->setIcon(QIcon(":/images/document-save-as-2.png"));
  saveAsAction->setStatusTip(tr("Save the project as..."));
  connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

  // Import media.
  importAction = new QAction(tr("&Import media source file..."), this);
  importAction->setIcon(QIcon(":/images/document-import-2.png"));
  importAction->setStatusTip(tr("Import a media source file..."));
  connect(importAction, SIGNAL(triggered()), this, SLOT(import()));

  // Add color.
  addColorAction = new QAction(tr("Add &Color paint..."), this);
  addColorAction->setIcon(QIcon(":/images/colorize.png"));
  addColorAction->setStatusTip(tr("Add a color paint..."));
  connect(addColorAction, SIGNAL(triggered()), this, SLOT(addColor()));

  // Exit/quit.
  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setStatusTip(tr("Exit the application"));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

//  cutAction = new QAction(tr("Cu&t"), this);
//  cutAction->setIcon(QIcon(":/images/cut.png"));
//  cutAction->setShortcut(QKeySequence::Cut);
//  cutAction->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
//  connect(cutAction, SIGNAL(triggered()), spreadsheet, SLOT(cut()));
//
//  copyAction = new QAction(tr("&Copy"), this);
//  copyAction->setIcon(QIcon(":/images/copy.png"));
//  copyAction->setShortcut(QKeySequence::Copy);
//  copyAction->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
//  connect(copyAction, SIGNAL(triggered()), spreadsheet, SLOT(copy()));
//
//  pasteAction = new QAction(tr("&Paste"), this);
//  pasteAction->setIcon(QIcon(":/images/paste.png"));
//  pasteAction->setShortcut(QKeySequence::Paste);
//  pasteAction->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
//  connect(pasteAction, SIGNAL(triggered()), spreadsheet, SLOT(paste()));
//
//  deleteAction = new QAction(tr("&Delete"), this);
//  deleteAction->setShortcut(QKeySequence::Delete);
//  deleteAction->setStatusTip(tr("Delete the current selection's contents"));
//  connect(deleteAction, SIGNAL(triggered()), spreadsheet, SLOT(del()));

  // About.
  aboutAction = new QAction(tr("&About"), this);
  aboutAction->setStatusTip(tr("Show the application's About box"));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

  // Delete.
  deleteAction = new QAction(tr("Delete"), this);
  deleteAction->setShortcut(tr("CTRL+DEL"));
  deleteAction->setStatusTip(tr("Delete item"));
  connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItem()));

  // Add quad/mesh.
  addMeshAction = new QAction(tr("Add Quad/&Mesh"), this);
  addMeshAction->setShortcut(tr("CTRL+M"));
  addMeshAction->setIcon(QIcon(":/images/draw-rectangle-2.png"));
  addMeshAction->setStatusTip(tr("Add quad/mesh"));
  connect(addMeshAction, SIGNAL(triggered()), this, SLOT(addMesh()));
  addMeshAction->setEnabled(false);

  // Add triangle.
  addTriangleAction = new QAction(tr("Add &Triangle"), this);
  addTriangleAction->setShortcut(tr("CTRL+T"));
  addTriangleAction->setIcon(QIcon(":/images/draw-triangle.png"));
  addTriangleAction->setStatusTip(tr("Add triangle"));
  connect(addTriangleAction, SIGNAL(triggered()), this, SLOT(addTriangle()));
  addTriangleAction->setEnabled(false);

  // Add ellipse.
  addEllipseAction = new QAction(tr("Add &Ellipse"), this);
  addEllipseAction->setShortcut(tr("CTRL+E"));
  addEllipseAction->setIcon(QIcon(":/images/draw-ellipse-2.png"));
  addEllipseAction->setStatusTip(tr("Add ellipse"));
  connect(addEllipseAction, SIGNAL(triggered()), this, SLOT(addEllipse()));
  addEllipseAction->setEnabled(false);

  // Toggle display of output window.
  displayOutputWindow = new QAction(tr("&Display output window"), this);
  displayOutputWindow->setShortcut(tr("Ctrl+D"));
  displayOutputWindow->setStatusTip(tr("Display output window"));
  displayOutputWindow->setCheckable(true);
  displayOutputWindow->setChecked(true);
  // Manage show/hide of GL output window.
  connect(displayOutputWindow, SIGNAL(toggled(bool)), outputWindow, SLOT(setVisible(bool)));
  // When closing the GL output window, uncheck the action in menu.
  connect(outputWindow, SIGNAL(closed()), displayOutputWindow, SLOT(toggle()));

  // Toggle display of canvas controls.
  displayCanvasControls = new QAction(tr("&Display canvas controls"), this);
//  displayCanvasControls->setShortcut(tr("Ctrl+E"));
  displayCanvasControls->setStatusTip(tr("Display canvas controls"));
  displayCanvasControls->setCheckable(true);
  displayCanvasControls->setChecked(true);
  // Manage show/hide of canvas controls.
  connect(displayCanvasControls, SIGNAL(toggled(bool)), sourceCanvas, SLOT(enableDisplayControls(bool)));
  connect(displayCanvasControls, SIGNAL(toggled(bool)), destinationCanvas, SLOT(enableDisplayControls(bool)));
  connect(displayCanvasControls, SIGNAL(toggled(bool)), outputWindow->getCanvas(), SLOT(enableDisplayControls(bool)));
}

void MainWindow::createMenus()
{
  // File.
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openAction);
  fileMenu->addAction(saveAction);
  fileMenu->addAction(saveAsAction);
  fileMenu->addSeparator();
  fileMenu->addAction(importAction);
  fileMenu->addAction(addColorAction);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  // Edit.
  editMenu = menuBar()->addMenu(tr("&Edit"));
  //  editMenu->addAction(cutAction);
  //  editMenu->addAction(copyAction);
  //  editMenu->addAction(pasteAction);
  editMenu->addAction(deleteAction);

  // View.
  viewMenu = menuBar()->addMenu(tr("&View"));
  viewMenu->addAction(displayOutputWindow);
  viewMenu->addAction(displayCanvasControls);

//  selectSubMenu = editMenu->addMenu(tr("&Select"));
//  selectSubMenu->addAction(selectRowAction);
//  selectSubMenu->addAction(selectColumnAction);
//  selectSubMenu->addAction(selectAllAction);

//  editMenu->addSeparator();
//  editMenu->addAction(findAction);
//  editMenu->addAction(goToCellAction);

//  toolsMenu = menuBar()->addMenu(tr("&Tools"));
//  toolsMenu->addAction(recalculateAction);
//  toolsMenu->addAction(sortAction);
//
//  optionsMenu = menuBar()->addMenu(tr("&Options"));
//  optionsMenu->addAction(showGridAction);
//  optionsMenu->addAction(autoRecalcAction);

  // Help.
  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutAction);
//  helpMenu->addAction(aboutQtAction);
}

void MainWindow::createContextMenu()
{
//  spreadsheet->addAction(cutAction);
//  spreadsheet->addAction(copyAction);
//  spreadsheet->addAction(pasteAction);
//  spreadsheet->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void MainWindow::createToolBars()
{
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  fileToolBar->addAction(importAction);
  fileToolBar->addAction(addColorAction);
  fileToolBar->addAction(newAction);
  fileToolBar->addAction(openAction);
  fileToolBar->addAction(saveAction);
  fileToolBar->addSeparator();
  fileToolBar->addAction(addMeshAction);
  fileToolBar->addAction(addTriangleAction);
  fileToolBar->addAction(addEllipseAction);

//  editToolBar = addToolBar(tr("&Edit"));
//  editToolBar->addAction(cutAction);
//  editToolBar->addAction(copyAction);
//  editToolBar->addAction(pasteAction);
//  editToolBar->addSeparator();
//  editToolBar->addAction(findAction);
//  editToolBar->addAction(goToCellAction);
}

void MainWindow::createStatusBar()
{
//  locationLabel = new QLabel(" W999 ");
//  locationLabel->setAlignment(Qt::AlignHCenter);
//  locationLabel->setMinimumSize(locationLabel->sizeHint());
//
//  formulaLabel = new QLabel;
//  formulaLabel->setIndent(3);
//
//  statusBar()->addWidget(locationLabel);
//  statusBar()->addWidget(formulaLabel, 1);
//
//  connect(spreadsheet, SIGNAL(currentCellChanged(int, int, int, int)), this,
//      SLOT(updateStatusBar()));
//  connect(spreadsheet, SIGNAL(modified()), this, SLOT(spreadsheetModified()));

  updateStatusBar();
}

void MainWindow::readSettings()
{
  QSettings settings("MapMap", "MapMap");

  restoreGeometry(settings.value("geometry").toByteArray());
  mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
  resourceSplitter->restoreState(settings.value("resourceSplitter").toByteArray());
  canvasSplitter->restoreState(settings.value("canvasSplitter").toByteArray());
  outputWindow->restoreGeometry(settings.value("outputWindow").toByteArray());
  config_osc_receive_port = settings.value("osc_receive_port", 12345).toInt();
}

void MainWindow::writeSettings()
{
  QSettings settings("MapMap", "MapMap");

  settings.setValue("geometry", saveGeometry());
  settings.setValue("mainSplitter", mainSplitter->saveState());
  settings.setValue("resourceSplitter", resourceSplitter->saveState());
  settings.setValue("canvasSplitter", canvasSplitter->saveState());
  settings.setValue("outputWindow", outputWindow->saveGeometry());
  settings.setValue("osc_receive_port", config_osc_receive_port);
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

  ProjectWriter writer(mappingManager);
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
  }

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("MapMap Project")));
}

// TODO
// bool MainWindow::updateMediaFile(const QString &source_name, const QString &fileName)
// {
// }

bool MainWindow::importMediaFile(const QString &fileName)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
      QMessageBox::warning(this, tr("MapMap Project"),
                           tr("Cannot read file %1:\n%2.")
                           .arg(file.fileName())
                           .arg(file.errorString()));
      return false;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add media file to model.
  uint mediaId = createMediaPaint(NULL_UID, fileName, 0, 0);

  // Initialize position (center).
  std::tr1::shared_ptr<Media> media = std::tr1::static_pointer_cast<Media>(mappingManager->getPaintById(mediaId));
  Q_CHECK_PTR(media);

  media->setPosition((sourceCanvas->width()  - media->getWidth() ) / 2.0f,
                     (sourceCanvas->height() - media->getHeight()) / 2.0f );

  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("File imported"), 2000);

  return true;
}

bool MainWindow::addColorPaint(const QColor& color)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add color to model.
  uint colorId = createColorPaint(NULL_UID, color);

  // Initialize position (center).
  std::tr1::shared_ptr<Color> colorPaint = std::tr1::static_pointer_cast<Color>(mappingManager->getPaintById(colorId));
  Q_CHECK_PTR(colorPaint);

  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("Color paint added"), 2000);

  return true;
}

void MainWindow::addPaintItem(uid paintId, const QIcon& icon, const QString& name)
{
  Paint::ptr paint = mappingManager->getPaintById(paintId);
  Q_CHECK_PTR(paint);

  // Add paint item to paintList widget.
  QListWidgetItem* item = new QListWidgetItem(name);
  setItemId(*item, paintId); // TODO: could possibly be replaced by a Paint pointer
  item->setIcon(icon);

  // Set size.
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::PAINT_LIST_ITEM_HEIGHT));

  // Add item to paint list.
  paintList->addItem(item);
  paintList->setCurrentItem(item);

  // Window was modified.
  windowModified();
}

void MainWindow::addMappingItem(uid mappingId)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  QString label;
  QIcon icon;

  QString shapeType = mapping->getShape()->getType();
  QString paintType = mapping->getPaint()->getType();

  // Add mapper.
  // XXX hardcoded for textures
  std::tr1::shared_ptr<TextureMapping> textureMapping;
  if (paintType == "media")
  {
    textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
    Q_CHECK_PTR(textureMapping);
  }

  Mapper::ptr mapper;

  // XXX Branching on nVertices() is crap


  // Triangle
  if (shapeType == "triangle")
  {
    label = QString("Triangle %1").arg(mappingId);
    icon = QIcon(":/images/draw-triangle.png");

    if (paintType == "color")
      mapper = Mapper::ptr(new PolygonColorMapper(mapping));
    else
      mapper = Mapper::ptr(new TriangleTextureMapper(textureMapping));
  }
  // Mesh
  else if (shapeType == "mesh" || shapeType == "quad")
  {
    label = QString(shapeType == "mesh" ? "Mesh %1" : "Quad %1").arg(mappingId);
    icon = QIcon(":/images/draw-rectangle-2.png");
    if (paintType == "color")
      mapper = Mapper::ptr(new PolygonColorMapper(mapping));
    else
      mapper = Mapper::ptr(new MeshTextureMapper(textureMapping));
  }
  else if (shapeType == "ellipse")
  {
    label = QString("Ellipse %1").arg(mappingId);
    icon = QIcon(":/images/draw-ellipse-2.png");
    if (paintType == "color")
      mapper = Mapper::ptr(new EllipseColorMapper(mapping));
    else
      mapper = Mapper::ptr(new EllipseTextureMapper(textureMapping));
  }
  else
  {
    label = QString("Polygon %1").arg(mappingId);
    icon = QIcon(":/images/draw-polygon-2.png");
  }

  // Add to list of mappers.
  mappers[mappingId] = mapper;
  QWidget* mapperEditor = mapper->getPropertiesEditor();
  propertyPanel->addWidget(mapperEditor);
  propertyPanel->setCurrentWidget(mapperEditor);
  propertyPanel->setEnabled(true);

  // When mapper value is changed, update canvases.
  connect(mapper.get(), SIGNAL(valueChanged()),
          this,         SLOT(updateCanvases()));

  connect(sourceCanvas, SIGNAL(shapeChanged(Shape*)),
          mapper.get(), SLOT(updateShape(Shape*)));

  connect(destinationCanvas, SIGNAL(shapeChanged(Shape*)),
          mapper.get(), SLOT(updateShape(Shape*)));

    // Add item to layerList widget.
  QListWidgetItem* item = new QListWidgetItem(label);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  setItemId(*item, mappingId); // TODO: could possibly be replaced by a Paint pointer
  item->setIcon(icon);
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::SHAPE_LIST_ITEM_HEIGHT));
  mappingList->insertItem(0, item);
  mappingList->setCurrentItem(item);

  // Window was modified.
  windowModified();
}

void MainWindow::removeMappingItem(uid mappingId)
{
  Mapping::ptr mapping = mappingManager->getMappingById(mappingId);
  Q_CHECK_PTR(mapping);

  // Remove mapping from model.
  mappingManager->removeMapping(mappingId);

  // Remove associated mapper.
  propertyPanel->removeWidget(mappers[mappingId]->getPropertiesEditor());
  mappers.remove(mappingId);

  // Remove widget from mappingList.
  int row = getItemRowFromId(*mappingList, mappingId);
  Q_ASSERT( row >= 0 );
  QListWidgetItem* item = mappingList->takeItem(row);
  if (item == currentSelectedItem)
    currentSelectedItem = NULL;
  delete item;

  // Update list.
  mappingList->update();

  // Update everything.
  updateCanvases();

  // Window was modified.
  windowModified();
}

void MainWindow::removePaintItem(uid paintId)
{
  Paint::ptr paint = mappingManager->getPaintById(paintId);
  Q_CHECK_PTR(paint);

  // Remove all mappings associated with paint.
  QMap<uid, Mapping::ptr> paintMappings = mappingManager->getPaintMappings(paint);
  for (QMap<uid, Mapping::ptr>::const_iterator it = paintMappings.constBegin();
       it != paintMappings.constEnd(); ++it)
    removeMappingItem(it.key());

  // Remove paint from model.
  Q_ASSERT( mappingManager->removePaint(paintId) );

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
}

void MainWindow::clearWindow()
{
  clearProject();
}

void MainWindow::updateCanvases()
{
  sourceCanvas->update();
  destinationCanvas->update();
  outputWindow->getCanvas()->update();
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void MainWindow::connectProjectWidgets()
{
  connect(paintList, SIGNAL(itemSelectionChanged()),
          this,      SLOT(handlePaintItemSelectionChanged()));

  connect(paintList, SIGNAL(itemPressed(QListWidgetItem*)),
          this,      SLOT(handleItemSelected(QListWidgetItem*)));

  connect(paintList, SIGNAL(itemActivated(QListWidgetItem*)),
          this,      SLOT(handleItemSelected(QListWidgetItem*)));

  connect(mappingList, SIGNAL(itemSelectionChanged()),
          this,        SLOT(handleMappingItemSelectionChanged()));

  connect(mappingList, SIGNAL(itemChanged(QListWidgetItem*)),
          this,        SLOT(handleMappingItemChanged(QListWidgetItem*)));

  connect(mappingList, SIGNAL(itemPressed(QListWidgetItem*)),
          this,        SLOT(handleItemSelected(QListWidgetItem*)));

  connect(mappingList, SIGNAL(itemActivated(QListWidgetItem*)),
          this,        SLOT(handleItemSelected(QListWidgetItem*)));

  connect(mappingList->model(), SIGNAL(layoutChanged()),
          this,                 SLOT(handleMappingIndexesMoved()));
}

void MainWindow::disconnectProjectWidgets()
{
  disconnect(paintList, SIGNAL(itemSelectionChanged()),
             this,      SLOT(handlePaintItemSelectionChanged()));

  disconnect(paintList, SIGNAL(itemPressed(QListWidgetItem*)),
             this,      SLOT(handleItemSelected(QListWidgetItem*)));

  disconnect(paintList, SIGNAL(itemActivated(QListWidgetItem*)),
             this,      SLOT(handleItemSelected(QListWidgetItem*)));

  disconnect(mappingList, SIGNAL(itemSelectionChanged()),
             this,        SLOT(handleMappingItemSelectionChanged()));

  disconnect(mappingList, SIGNAL(itemChanged(QListWidgetItem*)),
             this,        SLOT(handleMappingItemChanged(QListWidgetItem*)));

  disconnect(mappingList, SIGNAL(itemPressed(QListWidgetItem*)),
             this,        SLOT(handleItemSelected(QListWidgetItem*)));

  disconnect(mappingList, SIGNAL(itemActivated(QListWidgetItem*)),
             this,        SLOT(handleItemSelected(QListWidgetItem*)));

  disconnect(mappingList->model(), SIGNAL(layoutChanged()),
             this,                 SLOT(handleMappingIndexesMoved()));
}

uid MainWindow::getItemId(const QListWidgetItem& item)
{
  return item.data(Qt::UserRole).toInt();
}

void MainWindow::setItemId(QListWidgetItem& item, uid id)
{
  item.setData(Qt::UserRole, id);
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


void MainWindow::startOscReceiver()
{
#ifdef HAVE_OSC
  int port = config_osc_receive_port;
  std::ostringstream os;
  os << port;
  osc_interface.reset(new OscInterface(os.str()));
  if (port != 0)
  {
    osc_interface->start();
  }
  osc_timer = new QTimer(this); // FIXME: memleak?
  connect(osc_timer, SIGNAL(timeout()), this, SLOT(pollOscInterface()));
  osc_timer->start();
#endif
}

void MainWindow::pollOscInterface()
{
#ifdef HAVE_OSC
  osc_interface->consume_commands(*this);
#endif
}

void MainWindow::applyOscCommand(const QVariantList& command)
{
  bool VERBOSE = true;
  if (VERBOSE)
  {
    std::cout << "Receive OSC: ";
    for (int i = 0; i < command.size(); ++i)
    {
      if (command.at(i).type()  == QVariant::Int)
      {
        std::cout << command.at(i).toInt() << " ";
      }
      else if (command.at(i).type()  == QVariant::Double)
      {
        std::cout << command.at(i).toDouble() << " ";
      }
      else if (command.at(i).type()  == QVariant::String)
      {
        std::cout << command.at(i).toString().toStdString() << " ";
      }
      else
      {
        std::cout << "??? ";
      }
    }
    std::cout << std::endl;
    std::cout.flush();
  }

  if (command.size() < 2)
      return;
  if (command.at(0).type() != QVariant::String)
      return;
  if (command.at(1).type() != QVariant::String)
      return;
  std::string path = command.at(0).toString().toStdString();
  std::string typetags = command.at(1).toString().toStdString();

  // Handle all OSC messages here
  if (path == "/image/uri" && typetags == "s")
  {
      std::string image_uri = command.at(2).toString().toStdString();
      std::cout << "TODO load /image/uri " << image_uri << std::endl;
  }
  else if (path == "/add/quad")
      addMesh();
  else if (path == "/add/triangle")
      addTriangle();
  else if (path == "/add/ellipse")
      addEllipse();
  else if (path == "/project/save")
      save();
  else if (path == "/project/open")
      open();
}

