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

class SourceGLCanvas: public MapperGLCanvas
{
public:
  SourceGLCanvas(QWidget* parent = 0);

  virtual Quad& getQuad() {
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
    Q_CHECK_PTR(textureMapping);

    std::tr1::shared_ptr<Quad> inputQuad = std::tr1::static_pointer_cast<Quad>(textureMapping->getInputShape());
    Q_CHECK_PTR(inputQuad);

    return (*inputQuad);
  }

private:
  virtual void doDraw();
};


#endif /* DESTINATIONGLCANVAS_H_ */
