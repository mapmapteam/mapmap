/*
 * OutputGLWindow.cpp
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "OutputGLWindow.h"

#include "MainWindow.h"

OutputGLWindow::OutputGLWindow(QWidget* parent, const QGLWidget * shareWidget) : QDialog(parent)
{
  resize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  canvas = new DestinationGLCanvas(this, shareWidget);
  canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  canvas->setMinimumSize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  QLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0,0,0,0); // remove margin
  layout->addWidget(canvas);
  setLayout(layout);
}

void OutputGLWindow::closeEvent(QCloseEvent *event)
{
  emit closed();
  event->accept();
}

//void OutputGLWindow::updateCanvas() {
//  qDebug() << "Update output canvas" << endl;
//  canvas->updateCanvas();
//}
