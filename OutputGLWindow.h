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
#include "DestinationGLCanvas.h"

// TODO: add SLOT for mySetVisible
// TODO: Maybe improve support for Ubuntu: http://stackoverflow.com/questions/12645880/fullscreen-for-qdialog-from-within-mainwindow-only-working-sometimes

class OutputGLWindow : public QDialog
{
  Q_OBJECT

public:
  OutputGLWindow(MainWindow* mainWindow, QWidget* parent = 0, const QGLWidget * shareWidget = 0);

public slots:
  void setFullScreen(bool fullScreen);
  void setCursorVisible(bool visible);

protected:
  void closeEvent(QCloseEvent* event);
  void keyPressEvent(QKeyEvent *event);

signals:
  void closed();
  void fullScreenToggled(bool fullScreen);

public:
  DestinationGLCanvas* getCanvas() const { return canvas; }
  void setPointerHasMoved();

private:
  DestinationGLCanvas* canvas;
  QByteArray _geometry;

  bool _pointerIsVisible;
};

#endif /* OutputGLWINDOW_H_ */
