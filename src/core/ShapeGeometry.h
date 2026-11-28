/*
 * ShapeGeometry.h
 *
 * (c) 2026 MapMap contributors
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

#ifndef SHAPE_GEOMETRY_H_
#define SHAPE_GEOMETRY_H_

#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <QVector>

namespace mmp {

/**
 * A vertex of a textured shape, as sent to the graphics card.
 *
 * The position is in the coordinates of the graphics item that draws the shape.
 * The texture coordinate is homogeneous: the point read in the texture is
 * (s/q, t/q), where (0, 0) and (1, 1) are two opposite corners of the texture.
 * Dividing per pixel, rather than per vertex, gives an exact perspective.
 */
struct TexturedVertex
{
  float x;
  float y;
  float s;
  float t;
  float q;
};

/// The four corners of a quad, in order around it, on the source and on the destination.
struct QuadCorners
{
  /// Number of corners of a quad.
  static const int N_CORNERS = 4;

  QPointF input[N_CORNERS];
  QPointF output[N_CORNERS];
};

/**
 * Computes the vertices that draw shapes, without OpenGL, so that they can be tested.
 *
 * Every function returns triangles: three vertices per triangle, to be drawn with
 * GL_TRIANGLES, which exists in every version of OpenGL and OpenGL ES.
 */
namespace ShapeGeometry {

/// Below this difference of dot products, a source quad counts as a parallelogram.
const float INPUT_PARALLELOGRAM_THRESHOLD = 0.0001f;

/// Below this difference of dot products, a destination quad counts as a parallelogram.
const float OUTPUT_PARALLELOGRAM_THRESHOLD = 0.001f;

/// No limit to the number of times a quad is split in four.
const int UNLIMITED_DEPTH = -1;

/**
 * Returns the two triangles that show the whole texture on a rectangle.
 * Used by the source view, which shows the texture as it is.
 */
QVector<TexturedVertex> rectangle(const QRectF& rect);

/**
 * Returns the triangle that maps the source triangle @a input of the texture
 * onto the destination triangle @a output.
 *
 * @param textureRect the place of the texture in the source view
 * @param input three points in the source view
 * @param output three points in the coordinates of the graphics item
 */
QVector<TexturedVertex> triangle(const QRectF& textureRect, const QPointF input[3], const QPointF output[3]);

/**
 * Splits a quad of a mesh in smaller quads until each one is close enough to a
 * parallelogram, on the source and on the destination, for two triangles to show
 * it without visible distortion. Technique from Oliveira, M. "Correcting Texture
 * Mapping Errors Introduced by Graphics Hardware".
 *
 * @param quad the corners of the quad; its output corners are in the coordinates of the graphics item
 * @param outputArea the area of the bounding box of the output corners
 * @param minArea a quad whose output area is below this is not split
 * @param maxDepth how many times a quad may be split, or UNLIMITED_DEPTH
 * @param subQuads receives the quads to draw
 */
void subdivideQuad(const QuadCorners& quad, float outputArea, int minArea, int maxDepth,
                   QVector<QuadCorners>& subQuads);

/**
 * Appends the two triangles that show a quad: corners 0, 1, 2 and corners 0, 2, 3.
 *
 * @param textureRect the place of the texture in the source view
 * @param quad the corners of the quad; its output corners are in the coordinates of the graphics item
 * @param vertices receives the six vertices
 */
void appendQuad(const QRectF& textureRect, const QuadCorners& quad, QVector<TexturedVertex>& vertices);

/**
 * Returns a fan of triangles around the center of an ellipse, with homogeneous
 * texture coordinates that give an exact perspective.
 *
 * @param textureRect the place of the texture in the source view
 * @param outputToInput homography from the destination ellipse to the source ellipse
 * @param fromUnitCircle homography from the unit circle to the destination ellipse
 * @param center the perspective center of the destination ellipse
 * @param sceneToItem from scene coordinates to the coordinates of the graphics item
 * @param nTriangles the number of triangles of the fan
 */
QVector<TexturedVertex> ellipse(const QRectF& textureRect, const QTransform& outputToInput,
                                const QTransform& fromUnitCircle, const QPointF& center,
                                const QTransform& sceneToItem, int nTriangles);

} // namespace ShapeGeometry

} // namespace mmp

#endif /* SHAPE_GEOMETRY_H_ */
