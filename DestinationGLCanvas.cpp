/*
 * DestinationGLCanvas.cpp
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

#include "DestinationGLCanvas.h"
#include "MainWindow.h"

DestinationGLCanvas::DestinationGLCanvas(QWidget* parent, const QGLWidget * shareWidget)
: MapperGLCanvas(parent, shareWidget)
{
}

Shape* DestinationGLCanvas::getCurrentShape()
{
  int mappingId = MainWindow::getInstance().getCurrentMappingId();
  if (mappingId >= 0)
    return MainWindow::getInstance().getMappingManager().getMapping(mappingId)->getShape().get();
  else
    return NULL;
}

void DestinationGLCanvas::doDraw()
{
//  // No sources = nothing to do.
//  if (Common::nImages() == 0)
//    return;
//
//  // TODO: Ceci est un hack necessaire car tout est en fonction de la width/height de la texture.
//  // Il faut changer ca.
//  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
//  Q_CHECK_PTR(textureMapping);
//
//  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
//  Q_CHECK_PTR(texture);
//
//  for (int i=0; i < Common::nImages(); i++)
//  {
//    std::tr1::shared_ptr<Texture> tex = std::tr1::static_pointer_cast<Texture>(Common::mappings[i]->getPaint());
//    Q_CHECK_PTR(tex);
//
//    // FIXME: maybe the texture id is actually 0 and it's ok, no?
//    // we should use a boolean is_texture_loaded, or so
//    if (tex->getTextureId() == 0)
//    {
//      tex->loadTexture();
//    }
//  }

  glPushMatrix();

  MappingManager& mappingManager = MainWindow::getInstance().getMappingManager();
  for (int i=0; i<mappingManager.nMappings(); i++)
  {
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mappingManager.getMapping(i));
    Q_CHECK_PTR(textureMapping);

    std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
    Q_CHECK_PTR(texture);

    if (texture->getTextureId() == 0)
      texture->loadTexture();

    // Draw the mappings.
    TextureMapper mapper(textureMapping);
    mapper.draw();
  }

  // Draw the shape.
  if (MainWindow::getInstance().getCurrentMappingId() < 0)
    return;

  Shape* shape = getCurrentShape();
  if (shape)
  {
    glColor4f(0.0f, 0.0f, 0.7f, 1.0f);

    // Destination quad.
    // Source quad.
    glLineWidth(5);
    glBegin (GL_LINE_STRIP);
    {
      for (int i = 0; i < shape->nVertices()+1; i++)
      {
        glVertex2f(
            GLfloat(shape->getVertex(i % shape->nVertices())->x()),
            GLfloat(shape->getVertex(i % shape->nVertices())->y())
                   );
      }
    }
    glEnd ();
  }

  glPopMatrix();
}

