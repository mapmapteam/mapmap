/*
 * OutputGLWindow.h
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef OUTPUTGLWINDOW_H_
#define OUTPUTGLWINDOW_H_

#include <QDialog>
#include <QtGlobal>
#include <QTimer>
#include "OutputGLCanvas.h"

namespace mmp {

// TODO: add SLOT for mySetVisible
// TODO: Maybe improve support for Ubuntu: http://stackoverflow.com/questions/12645880/fullscreen-for-qdialog-from-within-mainwindow-only-working-sometimes

/**
 * This class acts as the pop-up window containing a copy of the destination canvas (see OutputGLCanvas)
 * and that can be put to fullscreen. Aside from the fullscreen functionality, it adds the possibility of
 * removing the cursor and displaying a cross-hair.
 */
class OutputGLWindow : public QDialog
{
  Q_OBJECT

public:
  OutputGLWindow(QWidget* parent, const MapperGLCanvas* canvas_);
  //OutputGLWindow(MainWindow* mainWindow, QWidget* parent = 0, const QGLWidget * shareWidget = 0);

public slots:
  void setFullScreen(bool fullScreen);
  void setCanvasDisplayCrosshair(bool crosshair);
  void setDisplayTestSignal(bool displayTestSignal);

signals:
  void closed();

public:
  MapperGLCanvas* getCanvas() const { return canvas; }
  void setPointerHasMoved();

  int getPreferredScreen() const { return _preferredScreen; }
  void setPreferredScreen(int screen);

private:
  OutputGLCanvas* canvas;

  void _updateToPreferredScreen();

  // Actually sets window to fullscreen (without affecting _isFullScreen).
  void _setFullScreen(bool fullscreen);
  void _resetCursor(bool fullscreen);

  bool _isFullScreen;
  int _preferredScreen;
};

}

#endif /* OutputGLWINDOW_H_ */
