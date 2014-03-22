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

DestinationGLCanvas::DestinationGLCanvas(wxFrame *parent) :
    MapperGLCanvas(parent) {
//  int argc = 1;
//  char* argv[1] = { wxString((wxTheApp ->argv)[0]).char_str() };
}

void DestinationGLCanvas::doRender() {
  // TODO: Ceci est un hack necessaire car tout est en fonction de la width/height de la texture.
  // Il faut changer ca.
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
  wxASSERT(textureMapping != NULL);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  wxASSERT(texture != NULL);

  if (texture->getTextureId() == 0)
  {
    texture->loadTexture();
    texture->setPosition( (GetClientSize().x - texture->getWidth()) / 2,
                          (GetClientSize().y - texture->getHeight()) / 2 );
  }

  // Now, draw
  // DRAW THE TEXTURE
  glPushMatrix();

  // Draw the mapping.
  Common::currentMapper->draw();

  // Draw the quad.
  Quad& quad = getQuad();

  glColor4f(1, 0, 0, 1);

  // Destination quad.
  // Source quad.
  glLineWidth(5);
  glBegin (GL_LINE_STRIP);
  {
    for (int i=0; i<5; i++) {
      glVertex3f(quad.getVertex(i % 4).x,
                 quad.getVertex(i % 4).y,
                 0);
    }
  }
  glEnd ();

  glPopMatrix();
}

