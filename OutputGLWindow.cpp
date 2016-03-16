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

  _pointerIsVisible = true;
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

void OutputGLWindow::setCursorVisible(bool visible)
{
  _pointerIsVisible = visible;

  if (_pointerIsVisible)
  {
    this->setCursor(Qt::ArrowCursor);
    _pointerIsVisible = true;
  }
  else
  {
    this->setCursor(Qt::ArrowCursor);
    this->setCursor(Qt::BlankCursor);
    _pointerIsVisible = false;
  }
}

void OutputGLWindow::closeEvent(QCloseEvent *event)
{
  emit closed();
  event->accept();
}

void OutputGLWindow::setFullScreen(bool fullscreen)
{
  if (fullscreen)
  {
    // Check if user is on multiple screen
    if (QApplication::desktop()->screenCount() > 1)
    {
      // Hide cursor
      setCursorVisible(!fullscreen);
      // Activate crosshair in fullscreen mode.
      // should be only drawn if the controls should be shown
      canvas->setDisplayCrosshair(fullscreen && canvas->getMainWindow()->displayControls());
      //Move window to second screen before fullscreening it.
      setGeometry(QApplication::desktop()->screenGeometry(1));
      //The problem related to the full screen on linux seems to be resolved
      // with Qt 5.5 at least on Debian but define macro anyway
#ifdef Q_OS_LINUX
      setWindowFlags(Qt::Window);
      setVisible(true);
      setWindowState( windowState() ^ Qt::WindowFullScreen );
      show();
#else
    showFullScreen();
#endif
    }
    else
    {
      show();
    }
  }
  else
  {
    if (QApplication::desktop()->screenCount() > 1) {
#ifdef Q_OS_LINUX
      setWindowFlags( windowFlags() & ~Qt::Window );
#else
    showNormal();
#endif
    }
    else
    {
      hide();
    }
  }
}

MM_END_NAMESPACE
