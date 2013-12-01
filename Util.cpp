/*
 * Util.cpp
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

#include "Util.h"
#include <algorithm>

namespace Util {

void correctGlTexCoord(GLfloat x, GLfloat y)
{
  glTexCoord2f (x, 1.0f - y);
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

Quad* createQuadForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  return new Quad(
    Point(texture->getX(), texture->getY()),
    Point(texture->getX() + texture->getWidth(), texture->getY()),
    Point(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    Point(texture->getX(), texture->getY() + texture->getHeight())
  );
}

} // end of namespace

