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

#include "MainWindow.h"


SourceGLCanvas::SourceGLCanvas(QWidget* parent)
  : MapperGLCanvas(parent)
{
}

Shape* SourceGLCanvas::getShapeFromMappingId(int mappingId)
{
  if (mappingId >= 0)
  {
    Mapping::ptr mapping = MainWindow::getInstance().getMappingManager().getMapping(mappingId);
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(mapping);
    Q_CHECK_PTR(textureMapping);

    return textureMapping->getInputShape().get();
  }
  else
    return NULL;
}



//Quad& SourceGLCanvas::getQuad()
//{
//  std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(Common::currentMapping);
//  Q_CHECK_PTR(textureMapping);
//
//  std::tr1::shared_ptr<Quad> inputQuad = std::tr1::static_pointer_cast<Quad>(textureMapping->getInputShape());
//  Q_CHECK_PTR(inputQuad);
//
//  return (*inputQuad);
//}

void SourceGLCanvas::doDraw()
{
  if (!MainWindow::getInstance().hasCurrentPaint())
    return;

  uint paintId = MainWindow::getInstance().getCurrentPaintId();

  // Retrieve paint (as texture) and draw it.
  Paint::ptr paint = MainWindow::getInstance().getMappingManager().getPaintById(paintId);
  Q_CHECK_PTR(paint);
  std::tr1::shared_ptr<Texture> texture = std::tr1::static_pointer_cast<Texture>(paint);
  Q_CHECK_PTR(texture);

  // Load texture if needed.
  if (texture->getTextureId() == 0) {
    texture->loadTexture();
  }

  // Draw the texture.
  glPushMatrix();

  // Enable blending mode (for alphas).
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable (GL_LIGHTING);
  glEnable (GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texture->getBits());

  std::cout << texture->getX() << "x" << texture->getY() << " : " << texture->getWidth() << "x" << texture->getHeight() << " " << texture->getTextureId() << std::endl;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // TODO: Exact projection of texture
  // see http://stackoverflow.com/questions/15242507/perspective-correct-texturing-of-trapezoid-in-opengl-es-2-0

  // Draw source texture (not moving) in the center of the area.

  // float centerX = (float) width()  / 2.0f;
  // float centerY = (float) height() / 2.0f;
  // float textureHalfWidth  = (float) texture->getWidth()  / 2.0f;
  // float textureHalfHeight = (float) texture->getHeight() / 2.0f;


  glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
  // FIXME: Does this draw the quad counterclockwise?
  glBegin (GL_QUADS);
  {
    Util::correctGlTexCoord(0, 0);
    glVertex3f (texture->getX(), texture->getY(), 0);

    Util::correctGlTexCoord(1, 0);
    glVertex3f (texture->getX()+texture->getWidth(), texture->getY(), 0);

    Util::correctGlTexCoord(1, 1);
    glVertex3f (texture->getX()+texture->getWidth(), texture->getY() + texture->getHeight(), 0);

    Util::correctGlTexCoord(0, 1);
    glVertex3f (texture->getX(), texture->getY() + texture->getHeight(), 0);
  }
  glEnd ();

  glDisable(GL_TEXTURE_2D);

  // Retrieve all mappings associated to paint.
  std::map<uint, Mapping::ptr> mappings = MainWindow::getInstance().getMappingManager().getPaintMappingsById(paintId);

  for (std::map<uint, Mapping::ptr>::iterator it = mappings.begin(); it != mappings.end(); ++it)
  {
    // TODO: Ceci est un hack necessaire car tout est en fonction de la width/height de la texture.
    // Il faut changer ca.
    std::tr1::shared_ptr<TextureMapping> textureMapping = std::tr1::static_pointer_cast<TextureMapping>(it->second);
    Q_CHECK_PTR(textureMapping);

    std::tr1::shared_ptr<Shape> inputShape = std::tr1::static_pointer_cast<Quad>(textureMapping->getInputShape());
    Q_CHECK_PTR(inputShape);

    if (it->first == MainWindow::getInstance().getCurrentMappingId())
      glColor4f(0.0f, 0.0f, 0.7f, 1.0f);
    else
      glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

    Shape* shape = inputShape.get();
    if (dynamic_cast<Mesh*>(shape))
    {
      Mesh* mesh = (Mesh*)shape;

      std::vector<Quad> quads = mesh->getQuads();
      for (std::vector<Quad>::const_iterator it = quads.begin(); it != quads.end(); ++it)
      {
        glLineWidth(1);
        glBegin (GL_LINE_STRIP);
        for (int i = 0; i < it->nVertices()+1; i++)
        {
          glVertex2f(
              it->getVertex(i % it->nVertices()).x,
              it->getVertex(i % it->nVertices()).y
                     );
        }
        glEnd ();
      }

    }
    else
    {
      // Destination quad.
      // Source quad.
      glLineWidth(5);
      glBegin (GL_LINE_STRIP);
      {
        for (int i = 0; i < shape->nVertices()+1; i++)
        {
          glVertex2f(
              shape->getVertex(i % shape->nVertices()).x,
              shape->getVertex(i % shape->nVertices()).y
                     );
        }
      }
      glEnd ();
    }
  }

  glPopMatrix();
}

