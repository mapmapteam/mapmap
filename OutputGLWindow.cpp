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

OutputGLWindow:: OutputGLWindow(const DestinationGLCanvas* canvas_)
{
  resize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  canvas = new DestinationGLCanvas(canvas_->getMainWindow(), this, (const QGLWidget*)canvas_->viewport(), canvas_->scene());
  canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  canvas->setMinimumSize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  QLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0,0,0,0); // remove margin
  layout->addWidget(canvas);
  setLayout(layout);

  // Save window geometry.
  _geometry = saveGeometry();

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

void OutputGLWindow::keyPressEvent(QKeyEvent *event)
{
  // Escape from full screen mode.
  if (isFullScreen())
  {
    if (event->key() == Qt::Key_Escape)
    {
      setFullScreen(false);
      emit fullScreenToggled(false);
    }
    else
    {
       setFullScreen(true);
       emit fullScreenToggled(true);
    }

  }

  else
  {
    QDialog::keyPressEvent(event);
  }
}


void OutputGLWindow::setFullScreen(bool fullscreen)
{
  // Activate crosshair in fullscreen mode.
  setCursorVisible(!fullscreen);
  canvas->setDisplayCrosshair(fullscreen);

  // NOTE: The showFullScreen() method does not work well under Ubuntu Linux. The code below fixes the issue.
  // Notice that there might be problems with the fullscreen in other OS / window managers. If so, please add
  // the code to fix those issues here.
  // See: http://qt-project.org/doc/qt-4.8/qwidget.html#showFullScreen
  // Source: http://stackoverflow.com/questions/12645880/fullscreen-for-qdialog-from-within-mainwindow-only-working-sometimes
#ifdef Q_OS_UNIX
  const QString session = QString(getenv("DESKTOP_SESSION")).toLower();
#endif
  if (fullscreen)
  {
    // Save window geometry.
    _geometry = saveGeometry();

    // Move window to second screen before fullscreening it.
    if (QApplication::desktop()->screenCount() > 1)
      setGeometry(QApplication::desktop()->screenGeometry(1));

#ifdef Q_OS_UNIX
    // Special case for Unity.
    if (session == "ubuntu" || session == "gnome") {
      setWindowState( windowState() | Qt::WindowFullScreen | Qt::WindowMaximized);
      setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
      show();
    } else {
      showFullScreen();
    }
#else
    showFullScreen();
#endif
  }
  else
  {
    // Restore geometry of window to what it was before full screen call.
    restoreGeometry(_geometry);

    // Keep window to second screen
    if (QApplication::desktop()->screenCount() > 1)
      setGeometry(QApplication::desktop()->screenGeometry(1));

#ifdef Q_OS_UNIX
    // Special case for Unity.
    if (session == "ubuntu" || session == "gnome") {
      setWindowState( windowState() & ~Qt::WindowFullScreen);
    } else {
      showNormal();
    }
#else
    showNormal();
#endif
  }
}
