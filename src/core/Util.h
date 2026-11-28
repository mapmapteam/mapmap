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

#include <QtGui/qopengl.h>

#include "Shapes.h"

#include "MM.h"
#include "Source.h"
#include <QString>
#include <QImage>

namespace mmp {

/**
 * @namespace Util Useful functions.
 */
namespace Util {

/**
 * Gives the glTexImage2D format/type that reads a 32-bit QImage's bits
 * directly: Format_RGB32/ARGB32 -> GL_BGRA + GL_UNSIGNED_INT_8_8_8_8_REV
 * (endian-safe), everything else (Format_RGBA8888/RGBX8888) -> GL_RGBA +
 * GL_UNSIGNED_BYTE.
 */
void glUploadFormat(QImage::Format imageFormat, GLenum& format, GLenum& type);

/**
 * Uploads pixel data into the currently bound GL_TEXTURE_2D. Storage is
 * (re)allocated with glTexImage2D only when the size differs from
 * allocWidth x allocHeight (which are then updated); otherwise the texture is
 * updated in place with glTexSubImage2D. Requires a current GL context.
 */
void uploadTextureBits(int width, int height, GLenum format, GLenum type,
                       const uchar* bits, int& allocWidth, int& allocHeight);

/**
 * Maps a number from a range to another.
 */
float map_float(float value, float istart, float istop, float ostart, float ostop);

/**
 * Maps a number from a range to another.
 */
int map_int(int value, int istart, int istop, int ostart, int ostop);

/**
 * Returns the scale that makes a content of the given size as large as
 * possible inside an area while keeping its proportions. Returns 1 if either
 * size is empty.
 */
qreal fitScale(const QSizeF& content, const QSizeF& area);

/**
 * Returns the largest rectangle with the proportions of content that fits
 * inside frame, centered in it. Returns frame if content is empty.
 */
QRectF fitRect(const QSizeF& content, const QRectF& frame);

/**
 * Moves points the way rectangle from has to move and stretch to become
 * rectangle to. Returns the points unchanged if from is empty.
 */
QVector<QPointF> mapRectToRect(const QVector<QPointF>& points, const QRectF& from, const QRectF& to);

// FIXME: these shape/drawing utilities should be moved to another file
/// Creates a quad on the corners of rect, clockwise from the top left.
Mesh* createMeshForRect(const QRectF& rect);
/// Creates a triangle standing on the bottom edge of rect, its top in the middle of the top edge.
Triangle* createTriangleForRect(const QRectF& rect);
/// Creates an ellipse touching the middle of each edge of rect.
Ellipse* createEllipseForRect(const QRectF& rect);

/// What the control of a vertex shows about it.
enum class VertexMark {
  None,     ///< Nearly no background.
  Selected, ///< The selected vertex: a white background that lets the image show through.
  Linked    ///< The vertex that goes with the one selected in the other editor: that background in the center only, as a dot.
};

/**
 * Draws the control of a vertex: a crossed circle, on the background of its
 * mark. The vertices of a locked shape are plain discs.
 */
void drawControlsVertex(QPainter* painter, const QPointF& vertex, VertexMark mark, bool locked, qreal radius = MM::VERTEX_SELECT_RADIUS, qreal strokeWidth = MM::VERTEX_SELECT_STROKE_WIDTH);

/**
 * Checks if a file exists or not.
 */
bool fileExists(const QString& filename);

/**
 * Erases a file if it exists.
 * @return True if it deleted it.
 */
bool eraseFile(const QString& filename);

/**
 * Erases MapMap's user settings file.
 * It should be located in ~/.config/MapMap/MapMap.conf
 */
bool eraseSettings();

/**
 * Checks if a string is a number.
 * @return True if it is a valid number.
 */
bool isNumeric(const QString& text);

} // end of namespace

}

#endif /* UTIL_H_ */
