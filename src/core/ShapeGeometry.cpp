/*
 * ShapeGeometry.cpp
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

#include "ShapeGeometry.h"

#include <QtMath>
#include <cmath>

namespace mmp {

namespace ShapeGeometry {

namespace {

/// A split cuts a quad in this many quads.
const int N_SUB_QUADS = 4;

/// Each split divides the area of a quad by four.
const float SPLIT_AREA_FACTOR = 0.25f;

/// Weight of each end of a segment in its middle.
const qreal HALF = 0.5;

/// Number of vertices of a triangle.
const int N_TRIANGLE_VERTICES = 3;

/// Vertex at @a position reading the texture at (s/q, t/q).
TexturedVertex makeVertex(const QPointF& position, qreal s, qreal t, qreal q)
{
  return { float(position.x()), float(position.y()), float(s), float(t), float(q) };
}

/// Vertex at @a position reading the texture at the source point @a input.
TexturedVertex makeVertex(const QRectF& textureRect, const QPointF& input, const QPointF& position)
{
  return makeVertex(position,
                    (input.x() - textureRect.x()) / textureRect.width(),
                    (input.y() - textureRect.y()) / textureRect.height(),
                    1);
}

/// Whether the quad a, b, c, d is close enough to a parallelogram.
bool isNearlyParallelogram(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d,
                           float threshold)
{
  const QPointF v1 = a - b;
  const QPointF v2 = c - b;
  const QPointF v3 = c - d;
  const QPointF v4 = a - d;
  return std::fabs(QPointF::dotProduct(v1, v2) - QPointF::dotProduct(v3, v4)) < threshold &&
         std::fabs(QPointF::dotProduct(v1, v4) - QPointF::dotProduct(v2, v3)) < threshold;
}

/// Splits the corners a, b, c, d of a quad in four quads, each given by its four corners.
void splitCorners(const QPointF corners[QuadCorners::N_CORNERS], QPointF sub[N_SUB_QUADS][QuadCorners::N_CORNERS])
{
  const QPointF& a = corners[0];
  const QPointF& b = corners[1];
  const QPointF& c = corners[2];
  const QPointF& d = corners[3];

  const QPointF ab = (a + b) * HALF;
  const QPointF bc = (b + c) * HALF;
  const QPointF cd = (c + d) * HALF;
  const QPointF ad = (a + d) * HALF;
  const QPointF abcd = (ab + cd) * HALF;

  const QPointF quads[N_SUB_QUADS][QuadCorners::N_CORNERS] = {
    { a,    ab, abcd, ad },
    { ab,   b,  bc,   abcd },
    { abcd, bc, c,    cd },
    { ad, abcd, cd,   d },
  };
  for (int i = 0; i < N_SUB_QUADS; i++)
    for (int j = 0; j < QuadCorners::N_CORNERS; j++)
      sub[i][j] = quads[i][j];
}

} // namespace

QVector<TexturedVertex> rectangle(const QRectF& rect)
{
  const TexturedVertex topLeft     = makeVertex(rect.topLeft(),     0, 0, 1);
  const TexturedVertex topRight    = makeVertex(rect.topRight(),    1, 0, 1);
  const TexturedVertex bottomRight = makeVertex(rect.bottomRight(), 1, 1, 1);
  const TexturedVertex bottomLeft  = makeVertex(rect.bottomLeft(),  0, 1, 1);
  return { topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft };
}

QVector<TexturedVertex> triangle(const QRectF& textureRect, const QPointF input[3], const QPointF output[3])
{
  QVector<TexturedVertex> vertices;
  vertices.reserve(N_TRIANGLE_VERTICES);
  for (int i = 0; i < N_TRIANGLE_VERTICES; i++)
    vertices.append(makeVertex(textureRect, input[i], output[i]));
  return vertices;
}

void subdivideQuad(const QuadCorners& quad, float outputArea, int minArea, int maxDepth,
                   QVector<QuadCorners>& subQuads)
{
  const bool stop =
    maxDepth == 0 ||
    outputArea < minArea ||
    (isNearlyParallelogram(quad.output[0], quad.output[1], quad.output[2], quad.output[3],
                           OUTPUT_PARALLELOGRAM_THRESHOLD) &&
     isNearlyParallelogram(quad.input[0], quad.input[1], quad.input[2], quad.input[3],
                           INPUT_PARALLELOGRAM_THRESHOLD));
  if (stop)
  {
    subQuads.append(quad);
    return;
  }

  QPointF inputs[N_SUB_QUADS][QuadCorners::N_CORNERS];
  QPointF outputs[N_SUB_QUADS][QuadCorners::N_CORNERS];
  splitCorners(quad.input, inputs);
  splitCorners(quad.output, outputs);
  const int subDepth = (maxDepth == UNLIMITED_DEPTH ? UNLIMITED_DEPTH : maxDepth - 1);
  for (int i = 0; i < N_SUB_QUADS; i++)
  {
    QuadCorners sub;
    for (int j = 0; j < QuadCorners::N_CORNERS; j++)
    {
      sub.input[j]  = inputs[i][j];
      sub.output[j] = outputs[i][j];
    }
    subdivideQuad(sub, outputArea * SPLIT_AREA_FACTOR, minArea, subDepth, subQuads);
  }
}

void appendQuad(const QRectF& textureRect, const QuadCorners& quad, QVector<TexturedVertex>& vertices)
{
  static const int TRIANGLE_CORNERS[] = { 0, 1, 2, 0, 2, 3 };
  for (int corner : TRIANGLE_CORNERS)
    vertices.append(makeVertex(textureRect, quad.input[corner], quad.output[corner]));
}

QVector<TexturedVertex> ellipse(const QRectF& textureRect, const QTransform& outputToInput,
                                const QTransform& fromUnitCircle, const QPointF& center,
                                const QTransform& sceneToItem, int nTriangles)
{
  // Homogeneous input coordinates (a, b, q) of an output point: the input point is
  // (a/q, b/q). Since (a, b, q) varies linearly across each output triangle, dividing
  // by q per pixel yields an exact perspective mapping.
  auto projectiveVertex = [&](const QPointF& outputPoint) {
    const qreal x = outputPoint.x();
    const qreal y = outputPoint.y();
    const qreal a = outputToInput.m11()*x + outputToInput.m21()*y + outputToInput.m31();
    const qreal b = outputToInput.m12()*x + outputToInput.m22()*y + outputToInput.m32();
    const qreal q = outputToInput.m13()*x + outputToInput.m23()*y + outputToInput.m33();
    return makeVertex(sceneToItem.map(outputPoint),
                      (a - textureRect.x()*q) / textureRect.width(),
                      (b - textureRect.y()*q) / textureRect.height(),
                      q);
  };

  QVector<TexturedVertex> vertices;
  if (nTriangles <= 0)
    return vertices;
  vertices.reserve(N_TRIANGLE_VERTICES * nTriangles);

  const TexturedVertex centerVertex = projectiveVertex(center);
  TexturedVertex previous = projectiveVertex(fromUnitCircle.map(QPointF(1, 0)));
  for (int i = 1; i <= nTriangles; i++)
  {
    const qreal angle = 2 * M_PI * i / nTriangles;
    const TexturedVertex next = projectiveVertex(fromUnitCircle.map(QPointF(std::cos(angle), std::sin(angle))));
    vertices.append(centerVertex);
    vertices.append(previous);
    vertices.append(next);
    previous = next;
  }
  return vertices;
}

} // namespace ShapeGeometry

} // namespace mmp
