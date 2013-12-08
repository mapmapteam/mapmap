/*
 * MainWindow.cpp
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

#include "MainWindow.h"

MainWindow::MainWindow()
{
  if (!instance)
    setInstance(this);

  mappingManager = new MappingManager;
  currentPaintId = 0;
  currentMappingId = 0;
  _hasCurrentPaint = false;
  _hasCurrentMapping = false;

  createLayout();

  createActions();
  createMenus();
  createContextMenu();
  createToolBars();
  createStatusBar();

  readSettings();

  //setWindowIcon(QIcon(":/images/icon.png"));
  setCurrentFile("");
}

MainWindow& MainWindow::getInstance()
{
  Q_ASSERT(instance);

  return *instance;
}

void MainWindow::setInstance(MainWindow* inst)
{
  instance = inst;
}

MainWindow::~MainWindow()
{
  delete mappingManager;
}

void MainWindow::handleSourceItemSelectionChanged()
{
  std::cout << "selection changed" << std::endl;
  QListWidgetItem* item = sourceList->currentItem();
  uint idx = item->data(Qt::UserRole).toUInt();
  std::cout << "idx=" << idx << std::endl;
  setCurrentPaint(idx);
  removeCurrentMapping();

  // Update canvases.
  updateAll();
  //sourceCanvas->switchImage(idx);
  //sourceCanvas->repaint();
  //destinationCanvas->repaint();
}

void MainWindow::handleLayerItemSelectionChanged()
{
  std::cout << "shape selection changed" << std::endl;
  QListWidgetItem* item = layerList->currentItem();
  uint idx = item->data(Qt::UserRole).toUInt();

  Mapping::ptr mapping = mappingManager->getLayerById(idx)->getMapping();
  setCurrentPaint(mapping->getPaint()->getId());
  setCurrentMapping(mapping->getId());

  updateAll();
  //sourceCanvas->switchImage(idx);
  //sourceCanvas->repaint();
  //destinationCanvas->repaint();
}

void MainWindow::handleLayerItemChanged(QListWidgetItem* item)
{
  uint layerId = item->data(Qt::UserRole).toUInt();
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  layer->setVisible(item->checkState() == Qt::Checked);
  updateAll();
}

void MainWindow::handleLayerIndexesMoved()
{
  qDebug() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1Moved!!!" << endl;
  std::vector<uint> newOrder;
  for (int row=layerList->count()-1; row>=0; row--)
  {
    uint layerId = layerList->item(row)->data(Qt::UserRole).toUInt();
    newOrder.push_back(layerId);
  }

  mappingManager->reorderLayers(newOrder);

  updateAll();
}

//void MainWindow::handleSourceSelectionChanged(const QItemSelection& selection)
//{
//  std::cout << "selection changed" << std::endl;
//  QModelIndex& index = selection.indexes().first();
//  int idx = index.row();
//  std::cout << "idx=" << idx << std::endl;
//  sourceCanvas->switchImage(idx);
//}

void MainWindow::closeEvent(QCloseEvent *event)
{
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
  if (okToContinue())
  {
    clearWindow();
    setCurrentFile("");
  }
}

void MainWindow::open()
{
  if (okToContinue())
  {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open mapping"), ".", tr("Libremapping files (*.lmp)"));
    if (!fileName.isEmpty())
      loadFile(fileName);
  }
}

bool MainWindow::save()
{
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
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save mapping"),
      ".", tr("Libremapping files (*.lmp)"));
  if (fileName.isEmpty())
    return false;

  return saveFile(fileName);
}

void MainWindow::import()
{
  if (okToContinue())
  {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import resource"), ".");
    if (!fileName.isEmpty())
      importFile(fileName);
  }
}

void MainWindow::addQuad()
{
  // Create default quad.

  // Retrieve current paint (as texture).
  Paint::ptr paint = MainWindow::getInstance().getMappingManager().getPaint(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
  Q_CHECK_PTR(texture);

  // Create input and output quads.
  Shape::ptr outputQuad = Shape::ptr(Util::createQuadForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
  Shape::ptr  inputQuad = Shape::ptr(Util::createQuadForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));

  // Create texture mapping.
  Mapping::ptr mapping(new TextureMapping(paint, outputQuad, inputQuad));
  uint layerId = mappingManager->addLayer(mapping);

  addLayerItem(layerId);
}

void MainWindow::addTriangle()
{
  // Create default quad.

  // Retrieve current paint (as texture).
  Paint::ptr paint = MainWindow::getInstance().getMappingManager().getPaint(getCurrentPaintId());
  Q_CHECK_PTR(paint);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
  Q_CHECK_PTR(texture);

  // Create input and output quads.
  Shape::ptr outputTriangle = Shape::ptr(Util::createTriangleForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));
  Shape::ptr inputTriangle = Shape::ptr(Util::createTriangleForTexture(texture.get(), sourceCanvas->width(), sourceCanvas->height()));

  // Create texture mapping.
  Mapping::ptr mapping(new TextureMapping(paint, inputTriangle, outputTriangle));
  uint layerId = mappingManager->addLayer(mapping);

  addLayerItem(layerId);
}

void MainWindow::about()
{
  QMessageBox::about(this, tr("About Libremapping"),
      tr("<h2>Libremapping "
          LIBREMAPPING_VERSION
          "</h2>"
          "<p>Copyright &copy; 2013 Office International de la Francophonie"
          "<p>Libremapping is a free software for video mapping."));
}

void MainWindow::updateStatusBar()
{
  // TODO
//  locationLabel->setText(spreadsheet->currentLocation());
//  formulaLabel->setText(spreadsheet->currentFormula());
}

void MainWindow::windowModified()
{
  setWindowModified(true);
  updateStatusBar();
}

void MainWindow::createLayout()
{
  sourceList = new QListWidget;
  sourceList->setSelectionMode(QAbstractItemView::SingleSelection);
  sourceList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sourceList->setMinimumWidth(SOURCE_LIST_MINIMUM_WIDTH);

  layerList = new QListWidget;
  layerList->setSelectionMode(QAbstractItemView::SingleSelection);
  layerList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  //layerList->setDragDropMode(QAbstractItemView::DragDrop);
  layerList->setDefaultDropAction(Qt::MoveAction);
  layerList->setDragDropMode(QAbstractItemView::InternalMove);
  layerList->setMinimumWidth(LAYER_LIST_MINIMUM_WIDTH);

  propertyPanel = new QStackedWidget;
  propertyPanel->setDisabled(true);
  propertyPanel->setMinimumWidth(PROPERTY_PANEL_MINIMUM_WIDTH);

  sourceCanvas = new SourceGLCanvas;
  destinationCanvas = new DestinationGLCanvas(0, sourceCanvas);

  connect(sourceCanvas,      SIGNAL(shapeChanged(Shape*)),
          destinationCanvas, SLOT(updateCanvas()));

//  connect(destinationCanvas, SIGNAL(imageChanged()),
//          sourceCanvas,      SLOT(updateCanvas()));

  sourceCanvas->setFocusPolicy(Qt::ClickFocus);
  destinationCanvas->setFocusPolicy(Qt::ClickFocus);

  sourceCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  destinationCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  sourceCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);
  destinationCanvas->setMinimumSize(CANVAS_MINIMUM_WIDTH, CANVAS_MINIMUM_HEIGHT);

  mainSplitter = new QSplitter(Qt::Vertical);

  resourceSplitter = new QSplitter(Qt::Horizontal);
  resourceSplitter->addWidget(sourceList);
  resourceSplitter->addWidget(layerList);
  resourceSplitter->addWidget(propertyPanel);

  canvasSplitter = new QSplitter(Qt::Horizontal);
  canvasSplitter->addWidget(sourceCanvas);
  canvasSplitter->addWidget(destinationCanvas);

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

  setWindowTitle(tr("Libremapping"));
  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
  setCentralWidget(mainSplitter);

//  Common::initializeLibremapper(sourceCanvas->width(), sourceCanvas->height());
//
//  for (int i = 0; i < Common::nImages(); i++)
//  {
//    std::tr1::shared_ptr<Image> img = std::tr1::static_pointer_cast<Image>(
//        Common::mappings[i]->getPaint());
//    Q_CHECK_PTR(img);
//
//    QListWidgetItem* item = new QListWidgetItem(strippedName(img->getImagePath()));
//    item->setData(Qt::UserRole, i);
//
//    sourceList->addItem(item);
//  }

  connect(sourceList, SIGNAL(itemSelectionChanged()),
          this, SLOT(handleSourceItemSelectionChanged()));

  connect(layerList, SIGNAL(itemSelectionChanged()),
          this, SLOT(handleLayerItemSelectionChanged()));

  connect(layerList, SIGNAL(itemChanged(QListWidgetItem*)),
          this, SLOT(handleLayerItemChanged(QListWidgetItem*)));

  connect(layerList->model(), SIGNAL(layoutChanged()),
          this, SLOT(handleLayerIndexesMoved()));


  //  sourceList->setModel(sourcesModel);
//
//  connect(sourceList->selectionModel(),
//          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
//          this, SLOT(handleSourceSelectionChanged(QItemSelection)));
}

void MainWindow::createActions()
{
  newAction = new QAction(tr("&New"), this);
  newAction->setIcon(QIcon(":/images/document-new-4.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setStatusTip(tr("Create a new mapping file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  openAction = new QAction(tr("&Open..."), this);
  openAction->setIcon(QIcon(":/images/document-open-3.png"));
  openAction->setShortcut(QKeySequence::Open);
  openAction->setStatusTip(tr("Open an existing mapping file"));
  connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

  saveAction = new QAction(tr("&Save"), this);
  saveAction->setIcon(QIcon(":/images/document-save-2.png"));
  saveAction->setShortcut(QKeySequence::Save);
  saveAction->setStatusTip(tr("Save the mapping to disk"));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

  saveAsAction = new QAction(tr("Save &As..."), this);
  saveAsAction->setIcon(QIcon(":/images/document-save-as-2.png"));
  saveAsAction->setStatusTip(tr("Save the mapping under a new name"));
  connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

  importAction = new QAction(tr("&Import media source..."), this);
  importAction->setIcon(QIcon(":/images/document-import-2.png"));
  importAction->setShortcut(QKeySequence::Open);
  importAction->setStatusTip(tr("Import a media source file"));
  connect(importAction, SIGNAL(triggered()), this, SLOT(import()));

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

  aboutAction = new QAction(tr("&About"), this);
  aboutAction->setStatusTip(tr("Show the application's About box"));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

  addQuadAction = new QAction(tr("&Add quad"), this);
  addQuadAction->setIcon(QIcon(":/images/draw-rectangle-2.png"));
  addQuadAction->setStatusTip(tr("Add quad"));
  connect(addQuadAction, SIGNAL(triggered()), this, SLOT(addQuad()));

  addTriangleAction = new QAction(tr("&Add triangle"), this);
  addTriangleAction->setIcon(QIcon(":/images/draw-triangle.png"));
  addTriangleAction->setStatusTip(tr("Add triangle"));
  connect(addTriangleAction, SIGNAL(triggered()), this, SLOT(addTriangle()));
}

void MainWindow::createMenus()
{
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openAction);
  fileMenu->addAction(importAction);
  fileMenu->addAction(saveAction);
  fileMenu->addAction(saveAsAction);
  separatorAction = fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

//  editMenu = menuBar()->addMenu(tr("&Edit"));
//  editMenu->addAction(cutAction);
//  editMenu->addAction(copyAction);
//  editMenu->addAction(pasteAction);
//  editMenu->addAction(deleteAction);

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

  menuBar()->addSeparator();

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
  fileToolBar->addAction(importAction);
  fileToolBar->addAction(newAction);
  fileToolBar->addAction(openAction);
  fileToolBar->addAction(saveAction);
  fileToolBar->addSeparator();
  fileToolBar->addAction(addQuadAction);
  fileToolBar->addAction(addTriangleAction);

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
  QSettings settings("OIF", "Libremapping");

  restoreGeometry(settings.value("geometry").toByteArray());
//  mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
//  sourceSplitter->restoreState(settings.value("sourceSplitter").toByteArray());
//  canvasSplitter->restoreState(settings.value("canvasSplitter").toByteArray());
}

void MainWindow::writeSettings()
{
  QSettings settings("OIF", "Libremapping");

  settings.setValue("geometry", saveGeometry());
  settings.setValue("mainSplitter", mainSplitter->saveState());
  settings.setValue("sourceSplitter", resourceSplitter->saveState());
  settings.setValue("canvasSplitter", canvasSplitter->saveState());
}

bool MainWindow::okToContinue()
{
  if (isWindowModified())
  {
    int r = QMessageBox::warning(this, tr("Libremapping"),
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
  // TODO: Try to read file.
//  if (!spreadsheet->readFile(fileName))
//  {
//    statusBar()->showMessage(tr("Loading canceled"), 2000);
//    return false;
//  }

  setCurrentFile(fileName);
  statusBar()->showMessage(tr("File loaded"), 2000);
  return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
  // TODO: Try to write file.
//  if (!spreadsheet->writeFile(fileName))
//  {
//    statusBar()->showMessage(tr("Saving canceled"), 2000);
//    return false;
//  }

  setCurrentFile(fileName);
  statusBar()->showMessage(tr("File saved"), 2000);
  return true;
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

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Spreadsheet")));
}

bool MainWindow::importFile(const QString &fileName)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
      QMessageBox::warning(this, tr("Spreadsheet"),
                           tr("Cannot read file %1:\n%2.")
                           .arg(file.fileName())
                           .arg(file.errorString()));
      return false;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Add image to model.
  uint imageId = mappingManager->addImage(fileName, sourceCanvas->width(), sourceCanvas->height());

  // Add image to sourceList widget.
  QListWidgetItem* item = new QListWidgetItem(strippedName(fileName));
  item->setData(Qt::UserRole, imageId); // TODO: could possibly be replaced by a Paint pointer
  item->setIcon(QIcon(fileName));
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::SOURCE_LIST_ITEM_HEIGHT));
  sourceList->addItem(item);
  sourceList->setCurrentItem(item);

//  update();

  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("File imported"), 2000);
  return true;
}

void MainWindow::addLayerItem(uint layerId)
{
  Layer::ptr layer = mappingManager->getLayerById(layerId);
  Q_CHECK_PTR(layer);

  Mapping::ptr mapping = layer->getMapping();
  uint mappingId = mapping->getId();

  QString label;
  QIcon icon;

  // Triangle
  if (mapping->getShape()->nVertices() == 3)
  {
    label = QString("Triangle %1").arg(mappingId);
    icon = QIcon(":/images/draw-triangle.png");
  }
  else if (mapping->getShape()->nVertices() == 4)
  {
    label = QString("Quad %1").arg(mappingId);
    icon = QIcon(":/images/draw-rectangle-2.png");
  }
  else
  {
    label = QString("Polygon %1").arg(mappingId);
    icon = QIcon(":/images/draw-polygon-2.png");
  }

  // Add mapper.
  // XXX hardcoded for textures
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
  Q_CHECK_PTR(textureMapping);

  Mapper::ptr mapper( new TextureMapper(textureMapping) );

  mappers[mappingId] = mapper;
  QWidget* mapperEditor = mapper->getPropertiesEditor();
  propertyPanel->addWidget(mapperEditor);
  propertyPanel->setCurrentWidget(mapperEditor);
  propertyPanel->setEnabled(true);

  // When mapper value is changed, update canvases.
  connect(mapper.get(), SIGNAL(valueChanged()),
          this,         SLOT(updateAll()));

  connect(sourceCanvas, SIGNAL(shapeChanged(Shape*)),
          mapper.get(), SLOT(updateShape(Shape*)));

  connect(destinationCanvas, SIGNAL(shapeChanged(Shape*)),
          mapper.get(), SLOT(updateShape(Shape*)));

    // Add item to layerList widget.
  QListWidgetItem* item = new QListWidgetItem(label);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, layerId); // TODO: could possibly be replaced by a Paint pointer
  item->setIcon(icon);
  item->setSizeHint(QSize(item->sizeHint().width(), MainWindow::SHAPE_LIST_ITEM_HEIGHT));
  layerList->insertItem(0, item);
  layerList->setCurrentItem(item);
}

void MainWindow::clearWindow()
{
  // TODO: implement clearWindow()
}

void MainWindow::updateAll()
{
  sourceCanvas->update();
  destinationCanvas->update();
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

MainWindow* MainWindow::instance = 0;
