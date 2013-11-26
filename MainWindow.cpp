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
  sourceList = new QListWidget;
  sourceCanvas = new SourceGLCanvas;
  destinationCanvas = new DestinationGLCanvas(0, sourceCanvas);

  connect(sourceCanvas, SIGNAL(quadChanged()),
          destinationCanvas, SLOT(updateCanvas()));

  connect(destinationCanvas, SIGNAL(quadSwitched()),
          sourceCanvas, SLOT(updateCanvas()));

  sourceCanvas->setFocusPolicy(Qt::ClickFocus);
  destinationCanvas->setFocusPolicy(Qt::ClickFocus);

  QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);

  QSplitter* canvasSplitter = new QSplitter(Qt::Horizontal);
  canvasSplitter->addWidget(sourceCanvas);
  canvasSplitter->addWidget(destinationCanvas);
  canvasSplitter->setMinimumWidth(DEFAULT_WIDTH * 2/3);

  mainSplitter->addWidget(sourceList);
  mainSplitter->addWidget(canvasSplitter);

  this->setWindowTitle(QObject::tr("Libremapping"));
  this->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
  this->setCentralWidget(mainSplitter);

  Common::initializeLibremapper(sourceCanvas->width(), sourceCanvas->height());

  for (int i=0; i<Common::nImages(); i++)
  {
    std::tr1::shared_ptr<Image> img = std::tr1::static_pointer_cast<Image>(Common::mappings[i]->getPaint());
    Q_CHECK_PTR(img);

    QListWidgetItem* item = new QListWidgetItem(img->getImagePath());
    item->setData(Qt::UserRole, i);

    sourceList->addItem(item);
  }

  connect(sourceList, SIGNAL(itemSelectionChanged()),
          this,       SLOT(handleSourceItemSelectionChanged()));

//  sourceList->setModel(sourcesModel);
//
//  connect(sourceList->selectionModel(),
//          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
//          this, SLOT(handleSourceSelectionChanged(QItemSelection)));
}

MainWindow::~MainWindow() {
}

void MainWindow::handleSourceItemSelectionChanged()
{
  std::cout << "selection changed" << std::endl;
  QListWidgetItem* item = sourceList->currentItem();
  int idx = item->data(Qt::UserRole).toInt();
  std::cout << "idx=" << idx << std::endl;
  sourceCanvas->switchImage(idx);
  sourceCanvas->repaint();
  destinationCanvas->repaint();
}

//void MainWindow::handleSourceSelectionChanged(const QItemSelection& selection)
//{
//  std::cout << "selection changed" << std::endl;
//  QModelIndex& index = selection.indexes().first();
//  int idx = index.row();
//  std::cout << "idx=" << idx << std::endl;
//  sourceCanvas->switchImage(idx);
//}

