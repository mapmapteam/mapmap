/*
 * SourceGLCanvas.cpp
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

#include "SourceGLCanvas.h"

SourceGLCanvas::SourceGLCanvas(wxFrame *parent) :
    MapperGLCanvas(parent) {
//  int argc = 1;
//  char* argv[1] = { wxString((wxTheApp ->argv)[0]).char_str() };
}

void SourceGLCanvas::doRender() {
  // TODO: Ceci est un hack necessaire car tout est en fonction de la width/height de la texture.
  // Il faut changer ca.
  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
  wxASSERT(textureMapping != NULL);

  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(textureMapping->getPaint());
  wxASSERT(texture != NULL);

  if (texture->getTextureId() == 0)
    texture->loadTexture();

  // Now, draw
  // DRAW THE TEXTURE
  glPushMatrix();

  // Enable blending mode (for alphas).
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable (GL_LIGHTING);
  glEnable (GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // TODO: Exact projection of texture
  // see http://stackoverflow.com/questions/15242507/perspective-correct-texturing-of-trapezoid-in-opengl-es-2-0

  // Draw source texture (not moving).
  glColor4f (1, 1, 1, 1.0f);
  glBegin (GL_QUADS);
  {
    glTexCoord2f (0, 0);
    glVertex3f (0, 0, 0);

    glTexCoord2f (1, 0);
    glVertex3f (1, 0, 0);

    glTexCoord2f (1, 1);
    glVertex3f (1, 1, 0);

    glTexCoord2f (0, 1);
    glVertex3f (0, 1, 0);
  }
  glEnd ();

  glDisable(GL_TEXTURE_2D);

  // Draw the quad.
  Quad& quad = getQuad();

  glColor4f(1, 0, 0, 1);

  // Source quad.
  // Source quad.
  glLineWidth(5);
  glBegin (GL_LINE_STRIP);
  {
    for (int i=0; i<5; i++) {
      glVertex3f(quad.getVertex(i % 4).x / (GLfloat)texture->getWidth(),
                 quad.getVertex(i % 4).y / (GLfloat)texture->getHeight(),
                 0);
    }
  }
  glEnd ();

  glPopMatrix();
}

