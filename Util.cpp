/*
 * Util.cpp
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

#include "Util.h"
#include "unused.h"
#include <algorithm>

namespace Util {

void correctGlTexCoord(GLfloat x, GLfloat y)
{
  glTexCoord2f (x, 1.0f - y);
}

void setGlTexPoint(const Texture& texture, const QPointF& inputPoint, const QPointF& outputPoint)
{
  // Set point in texture.
  correctGlTexCoord(
    (inputPoint.x() - texture.getX()) / (GLfloat) texture.getWidth(),
    (inputPoint.y() - texture.getY()) / (GLfloat) texture.getHeight());
  // Add point in output.
  glVertex2f(
    outputPoint.x(),
    outputPoint.y()
  );
}

/**
 * Convenience function to map a variable from one coordinate space
 * to another.
 * The result is clipped in the range [ostart, ostop]
 * Make sure ostop is bigger than ostart.
 *
 * To map a MIDI control value into the [0,1] range:
 * map(value, 0.0, 1.0, 0. 127.);
 *
 * Depends on: #include <algorithm>
 */
float map_float(float value, float istart, float istop, float ostart, float ostop)
{
    float ret = ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
    // In Processing, they don't do the following: (clipping)
    return std::max(std::min(ret, ostop), ostart);
}

/**
 * See map_float
 */
int map_int(int value, int istart, int istop, int ostart, int ostop)
{
    float ret = ostart + (ostop - ostart) * ((value - istart) / float(istop - istart));
    //g_print("%f = %d + (%d-%d) * ((%d-%d) / (%d-%d))", ret, ostart, ostop, ostart, value, istart, istop, istart);
    // In Processing, they don't do the following: (clipping)
    return std::max(std::min(int(ret), ostop), ostart);
}

Mesh* createMeshForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  UNUSED(frameHeight);
  UNUSED(frameWidth);

  return new Mesh(
    QPointF(texture->getX(), texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX(), texture->getY() + texture->getHeight())
  );
}

Triangle* createTriangleForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  UNUSED(frameHeight);
  UNUSED(frameWidth);

  return new Triangle(
    QPointF(texture->getX(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX() + texture->getWidth() / 2, texture->getY())
  );
}

Ellipse* createEllipseForTexture(Texture* texture, int frameWidth,
    int frameHeight)
{
  UNUSED(frameHeight);
  UNUSED(frameWidth);

  qreal halfWidth  = texture->getWidth() / 2;
  qreal halfHeight = texture->getHeight() / 2;

  return new Ellipse(
    QPointF(texture->getX(), texture->getY() + halfHeight),
    QPointF(texture->getX() + halfWidth, texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + halfHeight),
    QPointF(texture->getX() + halfWidth, texture->getY() + texture->getHeight()),
    true
  );
}



Quad* createQuadForColor(int frameWidth, int frameHeight)
{
  return new Quad(
    QPointF(frameWidth / 4, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight * 3/ 4),
    QPointF(frameWidth / 4, frameHeight * 3 / 4)
  );
}

Triangle* createTriangleForColor(int frameWidth, int frameHeight)
{
  return new Triangle(
    QPointF(frameWidth / 4, frameHeight * 3 / 4),
    QPointF(frameWidth * 3 / 4, frameHeight * 3 / 4),
    QPointF(frameWidth / 2, frameHeight / 4)
  );
}

Ellipse* createEllipseForColor(int frameWidth, int frameHeight)
{
  return new Ellipse(
    QPointF(frameWidth / 4, frameHeight / 2),
    QPointF(frameWidth / 2, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight / 2),
    QPointF(frameWidth / 2, frameHeight * 3 / 4),
    false
  );
}

} // end of namespace

