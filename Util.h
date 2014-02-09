/*
 * Util.h
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

#ifndef UTIL_H_
#define UTIL_H_

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "Shape.h"
#include "Paint.h"

/**
 * @namespace Util Useful functions.
 */
namespace Util {

void correctGlTexCoord(GLfloat x, GLfloat y);
float map_float(float value, float istart, float istop, float ostart, float ostop);
int map_int(int value, int istart, int istop, int ostart, int ostop);


Mesh* createMeshForTexture(Texture* texture, int frameWidth, int frameHeight);
Triangle* createTriangleForTexture(Texture* texture, int frameWidth, int frameHeight);
Ellipse* createEllipseForTexture(Texture* texture, int frameWidth, int frameHeight);

Quad* createQuadForColor(int frameWidth, int frameHeight);
Triangle* createTriangleForColor(int frameWidth, int frameHeight);
Ellipse* createEllipseForColor(int frameWidth, int frameHeight);

} // end of namespace

#endif /* UTIL_H_ */
