/*
 * Mapper.cpp
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

#include "Mapper.h"

void QuadTextureMapper::draw()
{
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(_mapping);
  Q_CHECK_PTR(textureMapping);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  Q_CHECK_PTR(texture);

  std::tr1::shared_ptr<Quad> quad = std::tr1::static_pointer_cast<Quad>(textureMapping->getShape());
  Q_CHECK_PTR(quad);

  std::tr1::shared_ptr<Quad> inputQuad = std::tr1::static_pointer_cast<Quad>(textureMapping->getInputShape());
  Q_CHECK_PTR(inputQuad);

  printf("Texid: %d\n", texture->getTextureId());
  // Project source texture and sent it to destination.

  glEnable (GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
    texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
    GL_UNSIGNED_BYTE, texture->getBits());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
  {
    for (int i=0; i<4; i++)
    {
      Util::correctGlTexCoord(
        (inputQuad->getVertex(i).x - texture->getX()) / (GLfloat) texture->getWidth(),
        (inputQuad->getVertex(i).y - texture->getY()) / (GLfloat) texture->getHeight());
      glVertex2f(
        quad->getVertex(i).x,
        quad->getVertex(i).y
        );
    }
  }
  glEnd();

  glDisable(GL_TEXTURE_2D);
}
