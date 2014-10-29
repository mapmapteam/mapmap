/*
 * DestinationGLCanvas.h
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

#ifndef DESTINATIONGLCANVAS_H_
#define DESTINATIONGLCANVAS_H_

#include "MapperGLCanvas.h"
#include "MappingManager.h"

#include "Mapper.h"

class DestinationGLCanvas: public MapperGLCanvas
{
  Q_OBJECT

public:
  DestinationGLCanvas(MainWindow* mainWindow, QWidget* parent = 0, const QGLWidget * shareWidget = 0);
//  virtual ~DestinationGLCanvas();

  virtual Shape* getShapeFromMappingId(uid mappingId);

  void setDisplayCrosshair(bool displayCrosshair) {
    _displayCrosshair = displayCrosshair;
  }

private:
  virtual void doDraw(QPainter* painter);
  void _drawTestSignal(QPainter* painter);

  bool _displayCrosshair;
  QImage _svg_test_signal;
  QBrush _brush_test_signal;

protected:
  // overriden from QGlWidget:
  virtual void resizeGL(int width, int height);
  bool eventFilter(QObject *target, QEvent *event);
};

#endif /* DESTINATIONGLCANVAS_H_ */
