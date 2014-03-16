/*
 * OutputGLWindow.h
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

#ifndef OUTPUTGLWINDOW_H_
#define OUTPUTGLWINDOW_H_


#include <QDialog>
#include "DestinationGLCanvas.h"

//we need to create our own widget for the window containing the droneqglwidget
//to change the behavior of the close event to do nothing
//since we dont want this window to be closed by the user
class OutputGLWindow : public QDialog
{
public:
  OutputGLWindow(QWidget* parent = 0, const QGLWidget * shareWidget = 0);

protected:
  void closeEvent(QCloseEvent *){}

public slots:
  void updateCanvas();

private:
  DestinationGLCanvas* canvas;
};

#endif /* OutputGLWINDOW_H_ */
