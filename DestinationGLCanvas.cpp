/*
 * DestinationGLCanvas.cpp
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

#include "DestinationGLCanvas.h"

DestinationGLCanvas::DestinationGLCanvas(QWidget* parent, const QGLWidget * shareWidget)
: MapperGLCanvas(parent, shareWidget)
{
}

Quad& DestinationGLCanvas::getQuad()
{
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Quad> quad = std::tr1::static_pointer_cast<Quad>(Common::currentMapping->getShape());
  Q_CHECK_PTR(quad);

  return (*quad);
}

void DestinationGLCanvas::doDraw()
{
  // TODO: Ceci est un hack necessaire car tout est en fonction de la width/height de la texture.
  // Il faut changer ca.
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  for (int i=0; i < Common::nImages(); i++)
  {
    std::tr1::shared_ptr<Texture> tex = std::tr1::static_pointer_cast<Texture>(Common::mappings[i]->getPaint());
    Q_CHECK_PTR(tex);

    // FIXME: maybe the texture id is actually 0 and it's ok, no?
    // we should use a boolean is_texture_loaded, or so
    if (tex->getTextureId() == 0)
    {
      tex->loadTexture();
      tex->setPosition(
        (width() - tex->getWidth()) / 2,
        (height() - tex->getHeight()) / 2 );
    }
  }

  // Now, draw
  // DRAW THE TEXTURE
  glPushMatrix();

  for (int i=0; i < Common::nImages(); i++)
  {
    // Draw the mappings.
    Common::mappers[i]->draw();
  }

  // Draw the quad.
  Quad& quad = getQuad();

  glColor4f(1.0, 0.0, 0.0, 1.0);

  // Destination quad.
  // Source quad.
  glLineWidth(5);
  glBegin (GL_LINE_STRIP);
  {
    for (int i=0; i<5; i++)
    {
      glVertex2f(
        quad.getVertex(i % 4).x,
        quad.getVertex(i % 4).y);
    }
  }
  glEnd ();

  glPopMatrix();
}

