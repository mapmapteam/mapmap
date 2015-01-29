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

// XXX OpenGL ES:
#include <QtGui/qopengl.h>
#include <QtGui/qopengles2ext.h>
#include <QtGui/QOpenGLFunctions>
//#include <QtGui/qopenglext.h>

#include "MM.h"
#include "Shape.h"
#include "Paint.h"
#include <QString>

/**
 * @namespace Util Useful functions.
 */
namespace Util {

/// Calls glTexCoord2f() with the corrected texture coordinates.
void correctGlTexCoord(GLfloat x, GLfloat y);

/**
 * Takes a point (intputPoint) on texture and outputs it as a GL vertex on output (outputPoint).
 * Useful in texture mappings, where one often needs to call this function to project points from texture
 * between glBegin(<GL_SHAPE>) and glEnd() calls.
 */
void setGlTexPoint(const Texture& texture, const QPointF& inputPoint, const QPointF& outputPoint);

float map_float(float value, float istart, float istop, float ostart, float ostop);
int map_int(int value, int istart, int istop, int ostart, int ostop);

Mesh* createMeshForTexture(Texture* texture, int frameWidth, int frameHeight);
Triangle* createTriangleForTexture(Texture* texture, int frameWidth, int frameHeight);
Ellipse* createEllipseForTexture(Texture* texture, int frameWidth, int frameHeight);

Quad* createQuadForColor(int frameWidth, int frameHeight);
Triangle* createTriangleForColor(int frameWidth, int frameHeight);
Ellipse* createEllipseForColor(int frameWidth, int frameHeight);

void drawControlsVertex(QPainter* painter, const QPointF& vertex, bool selected, qreal radius = MM::VERTEX_SELECT_RADIUS, qreal strokeWidth = MM::VERTEX_SELECT_STROKE_WIDTH);

void drawControlsVertices(QPainter* painter, const QList<int>* selectedVertices, const Shape& shape);
void drawControlsEllipse(QPainter* painter, const QList<int>* selectedVertices, const Ellipse& ellipse);
void drawControlsQuad(QPainter* painter, const QList<int>* selectedVertices, const Quad& quad);
void drawControlsMesh(QPainter* painter, const QList<int>* selectedVertices, const Mesh& mesh);
void drawControlsPolygon(QPainter* painter, const QList<int>* selectedVertices, const Polygon& polygon);

/**
 * Checks if a file exists or not.
 */
bool fileExists(const QString& filename);
bool eraseFile(const QString& filename);
bool eraseSettings();

bool isNumeric(const QString& text);

} // end of namespace

#endif /* UTIL_H_ */
