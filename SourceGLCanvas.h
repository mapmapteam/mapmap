/*
 * SourceGLCanvas.h
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

#ifndef SOURCEGLCANVAS_H_
#define SOURCEGLCANVAS_H_

#include <QGLWidget>

#include "MapperGLCanvas.h"
#include "DestinationGLCanvas.h"

#include "Util.h"

class SourceGLCanvas: public MapperGLCanvas
{
  Q_OBJECT

public:
  SourceGLCanvas(MainWindow* mainWindow, QWidget* parent = 0);
  virtual ~SourceGLCanvas() {}

  virtual bool isOutput() const { return false; }
  virtual MShape::ptr getShapeFromMapping(const Mapping::ptr& mapping) const;
  virtual QSharedPointer<ShapeGraphicsItem> getShapeGraphicsItemFromMapping(const Mapping::ptr& mapping) const;

private:
//  virtual void doDraw(QPainter* painter);
};


#endif /* DESTINATIONGLCANVAS_H_ */
