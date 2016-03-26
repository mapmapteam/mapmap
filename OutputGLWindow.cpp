/*
 * OutputGLWindow.cpp
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#include "OutputGLWindow.h"

#include "MainWindow.h"

MM_BEGIN_NAMESPACE

OutputGLWindow:: OutputGLWindow(QWidget* parent, const MapperGLCanvas* canvas_) : QDialog(parent)
{
  resize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  canvas = new OutputGLCanvas(canvas_->getMainWindow(), this, (const QGLWidget*)canvas_->viewport(), canvas_->scene());
  canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  canvas->setMinimumSize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  QLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0,0,0,0); // remove margin
  layout->addWidget(canvas);
  setLayout(layout);

  setDisplayCrosshair(false); // default
}

//OutputGLWindow::OutputGLWindow(MainWindow* mainWindow, QWidget* parent, const QGLWidget * shareWidget) : QDialog(parent)
//{
//  resize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);
//
//  canvas = new DestinationGLCanvas(mainWindow, this, shareWidget);
//  canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//  canvas->setMinimumSize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);
//
//  QLayout* layout = new QVBoxLayout;
//  layout->setContentsMargins(0,0,0,0); // remove margin
//  layout->addWidget(canvas);
//  setLayout(layout);
//
//  // Save window geometry.
//  _geometry = saveGeometry();
//
//  _pointerIsVisible = true;
//
//}

void OutputGLWindow::setFullScreen(bool fullscreen)
{
  if (fullscreen)
  {
    _updateToPreferredScreen();
#ifdef Q_OS_LINUX
    // The problem related to the full screen on linux seems to be resolved with Qt 5.5 on Debian.
    // However this still seems to be needed on Ubuntu 15.10.
    // Fix source:
    // http://stackoverflow.com/questions/12645880/fullscreen-for-qdialog-from-within-mainwindow-only-working-sometimes
    setWindowFlags(Qt::Window);
#endif
    showFullScreen();
  }
  else
  {
    hide();
  }
}

void OutputGLWindow::updateScreenCount(int nScreens)
{
  Q_UNUSED(nScreens);
  // Untested.
  _updateToPreferredScreen();
}

void OutputGLWindow::_updateToPreferredScreen()
{
  // Check if user is on multiple screen (always pre
  int screen = _getPreferredScreen();
  //Move window to second screen before fullscreening it.
  setGeometry(QApplication::desktop()->screenGeometry(screen));
}


void OutputGLWindow::setDisplayCrosshair(bool crosshair)
{
  canvas->setDisplayCrosshair(crosshair);

  if (crosshair)
  {
//    this->setCursor(Qt::ArrowCursor);
    setCursor(Qt::BlankCursor);
  }
  else
  {
    setCursor(Qt::ArrowCursor);
  }
}

void OutputGLWindow::setDisplayTestSignal(bool displayTestSignal)
{
  canvas->setDisplayTestSignal(displayTestSignal);
  canvas->update();
}


MM_END_NAMESPACE
