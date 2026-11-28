/*
 * TestShapeGeometry.cpp
 *
 * Unit tests for the vertices that draw shapes (src/core/ShapeGeometry).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestShapeGeometry.h"
#include "ShapeGeometry.h"
#include "Ellipse.h"
#include <QPolygonF>
#include <cmath>

using namespace mmp;

namespace {

/// Tolerance on positions and texture coordinates, which are floats.
const float EPSILON = 1e-4f;

/// A texture placed at (100, 50) in the source view, 400 × 200 pixels.
const QRectF TEXTURE_RECT(100, 50, 400, 200);

/// Smallest area of a quad that is split, as MM::MESH_SUBDIVISION_MIN_AREA.
const int MIN_AREA = 400;

/// Number of triangles of the fan of an ellipse in these tests.
const int N_ELLIPSE_TRIANGLES = 32;

/// The point read in the texture, (s/q, t/q).
QPointF texturePoint(const TexturedVertex& v)
{
  return QPointF(v.s / v.q, v.t / v.q);
}

/// The position of a vertex.
QPointF position(const TexturedVertex& v)
{
  return QPointF(v.x, v.y);
}

bool near(const QPointF& a, const QPointF& b, qreal epsilon = EPSILON)
{
  return std::fabs(a.x() - b.x()) < epsilon && std::fabs(a.y() - b.y()) < epsilon;
}

/// The texture point of a point of the source view.
QPointF normalized(const QPointF& input)
{
  return QPointF((input.x() - TEXTURE_RECT.x()) / TEXTURE_RECT.width(),
                 (input.y() - TEXTURE_RECT.y()) / TEXTURE_RECT.height());
}

/// A quad whose source is the texture and whose destination is @a output.
QuadCorners quadOnTexture(const QPointF output[QuadCorners::N_CORNERS])
{
  QuadCorners quad;
  quad.input[0] = TEXTURE_RECT.topLeft();
  quad.input[1] = TEXTURE_RECT.topRight();
  quad.input[2] = TEXTURE_RECT.bottomRight();
  quad.input[3] = TEXTURE_RECT.bottomLeft();
  for (int i = 0; i < QuadCorners::N_CORNERS; i++)
    quad.output[i] = output[i];
  return quad;
}

/// Area of the bounding box of the output corners, as MeshTextureGraphicsItem gives it.
float outputArea(const QuadCorners& quad)
{
  QPolygonF polygon;
  for (const QPointF& p : quad.output)
    polygon << p;
  const QRectF box = polygon.boundingRect();
  return box.width() * box.height();
}

/// Area of a convex quad (shoelace formula).
qreal quadArea(const QPointF corners[QuadCorners::N_CORNERS])
{
  qreal twiceArea = 0;
  for (int i = 0; i < QuadCorners::N_CORNERS; i++)
  {
    const QPointF& a = corners[i];
    const QPointF& b = corners[(i + 1) % QuadCorners::N_CORNERS];
    twiceArea += a.x() * b.y() - b.x() * a.y();
  }
  return std::fabs(twiceArea) / 2;
}

/// A trapezoid narrower at the top: none of its sub-quads is a parallelogram.
const QPointF TRAPEZOID[QuadCorners::N_CORNERS] = {
  QPointF(300, 0), QPointF(500, 0), QPointF(800, 400), QPointF(0, 400)
};

} // namespace

void TestShapeGeometry::rectangleShowsWholeTexture()
{
  const QVector<TexturedVertex> vertices = ShapeGeometry::rectangle(TEXTURE_RECT);
  QCOMPARE(vertices.size(), 6);
  for (const TexturedVertex& v : vertices)
  {
    QCOMPARE(v.q, 1.0f);
    // Each corner of the rectangle reads the matching corner of the texture.
    QVERIFY(near(texturePoint(v), normalized(position(v))));
  }
}

void TestShapeGeometry::triangleMapsSourceToDestination()
{
  const QPointF input[3]  = { QPointF(100, 50), QPointF(500, 50), QPointF(300, 250) };
  const QPointF output[3] = { QPointF(10, 20), QPointF(70, 25), QPointF(40, 90) };
  const QVector<TexturedVertex> vertices = ShapeGeometry::triangle(TEXTURE_RECT, input, output);
  QCOMPARE(vertices.size(), 3);
  for (int i = 0; i < 3; i++)
  {
    QVERIFY(near(position(vertices[i]), output[i]));
    QVERIFY(near(texturePoint(vertices[i]), normalized(input[i])));
  }
  QVERIFY(near(texturePoint(vertices[2]), QPointF(0.5, 1)));
}

void TestShapeGeometry::parallelogramIsNotSplit()
{
  const QPointF output[QuadCorners::N_CORNERS] = {
    QPointF(100, 100), QPointF(500, 150), QPointF(600, 450), QPointF(200, 400)
  };
  const QuadCorners quad = quadOnTexture(output);
  QVector<QuadCorners> subQuads;
  ShapeGeometry::subdivideQuad(quad, outputArea(quad), MIN_AREA, ShapeGeometry::UNLIMITED_DEPTH, subQuads);
  QCOMPARE(subQuads.size(), 1);
}

void TestShapeGeometry::trapezoidIsSplitDownToMinimumArea()
{
  const QuadCorners quad = quadOnTexture(TRAPEZOID);
  const float area = outputArea(quad);
  QVector<QuadCorners> subQuads;
  ShapeGeometry::subdivideQuad(quad, area, MIN_AREA, ShapeGeometry::UNLIMITED_DEPTH, subQuads);

  // Every split divides the area by four, until it is below the minimum.
  int depth = 0;
  for (float a = area; a >= MIN_AREA; a /= 4)
    depth++;
  QCOMPARE(subQuads.size(), int(std::pow(4, depth)));
}

void TestShapeGeometry::depthLimitsSplits()
{
  const QuadCorners quad = quadOnTexture(TRAPEZOID);
  for (int maxDepth = 0; maxDepth <= 3; maxDepth++)
  {
    QVector<QuadCorners> subQuads;
    ShapeGeometry::subdivideQuad(quad, outputArea(quad), MIN_AREA, maxDepth, subQuads);
    QCOMPARE(subQuads.size(), int(std::pow(4, maxDepth)));
  }
}

void TestShapeGeometry::subQuadsCoverTheQuad()
{
  const QuadCorners quad = quadOnTexture(TRAPEZOID);
  QVector<QuadCorners> subQuads;
  ShapeGeometry::subdivideQuad(quad, outputArea(quad), MIN_AREA, 3, subQuads);

  qreal inputArea = 0;
  qreal outputAreaSum = 0;
  for (const QuadCorners& sub : subQuads)
  {
    inputArea += quadArea(sub.input);
    outputAreaSum += quadArea(sub.output);
  }
  QVERIFY(std::fabs(inputArea - quadArea(quad.input)) < EPSILON * inputArea);
  QVERIFY(std::fabs(outputAreaSum - quadArea(quad.output)) < EPSILON * outputAreaSum);

  // The first sub-quad keeps the first corner, on the source and on the destination.
  QCOMPARE(subQuads.first().input[0], quad.input[0]);
  QCOMPARE(subQuads.first().output[0], quad.output[0]);
}

void TestShapeGeometry::quadIsTwoTriangles()
{
  const QuadCorners quad = quadOnTexture(TRAPEZOID);
  QVector<TexturedVertex> vertices;
  ShapeGeometry::appendQuad(TEXTURE_RECT, quad, vertices);
  QCOMPARE(vertices.size(), 6);

  const int corners[] = { 0, 1, 2, 0, 2, 3 };
  for (int i = 0; i < 6; i++)
  {
    QVERIFY(near(position(vertices[i]), quad.output[corners[i]]));
    QVERIFY(near(texturePoint(vertices[i]), normalized(quad.input[corners[i]])));
  }

  // Appending adds to what is there.
  ShapeGeometry::appendQuad(TEXTURE_RECT, quad, vertices);
  QCOMPARE(vertices.size(), 12);
}

void TestShapeGeometry::ellipseIsAFanOfTriangles()
{
  const Ellipse output(QPointF(0, 50), QPointF(100, 0), QPointF(200, 50), QPointF(100, 100));
  const QVector<TexturedVertex> vertices = ShapeGeometry::ellipse(
    TEXTURE_RECT, QTransform(), output.fromUnitCircle(), output.getCenter(), QTransform(),
    N_ELLIPSE_TRIANGLES);
  QCOMPARE(vertices.size(), 3 * N_ELLIPSE_TRIANGLES);

  const QTransform toUnitCircle = output.toUnitCircle();
  for (int i = 0; i < N_ELLIPSE_TRIANGLES; i++)
  {
    // Every triangle starts at the center, and its two other vertices are on the ellipse.
    QVERIFY(near(position(vertices[3*i]), output.getCenter()));
    QVERIFY(std::fabs(QLineF(QPointF(), toUnitCircle.map(position(vertices[3*i + 1]))).length() - 1) < EPSILON);
    // Each triangle starts where the previous one ended.
    if (i > 0)
      QVERIFY(near(position(vertices[3*i + 1]), position(vertices[3*i - 1])));
  }
  // The fan closes.
  QVERIFY(near(position(vertices.last()), position(vertices[1])));

  QVERIFY(ShapeGeometry::ellipse(TEXTURE_RECT, QTransform(), output.fromUnitCircle(), output.getCenter(),
                                 QTransform(), 0).isEmpty());
}

void TestShapeGeometry::ellipseOnItselfShowsTextureAsItIs()
{
  // Same ellipse on the source and on the destination, moved by the item's transform.
  const Ellipse ellipse(QPointF(100, 150), QPointF(300, 50), QPointF(500, 150), QPointF(300, 250));
  const QTransform outputToInput = ellipse.toUnitCircle() * ellipse.fromUnitCircle();
  const QTransform sceneToItem = QTransform::fromTranslate(-10, 20);
  const QVector<TexturedVertex> vertices = ShapeGeometry::ellipse(
    TEXTURE_RECT, outputToInput, ellipse.fromUnitCircle(), ellipse.getCenter(), sceneToItem,
    N_ELLIPSE_TRIANGLES);

  for (const TexturedVertex& v : vertices)
  {
    const QPointF scenePoint = sceneToItem.inverted().map(position(v));
    QVERIFY(near(texturePoint(v), normalized(scenePoint)));
  }
}

void TestShapeGeometry::ellipseFollowsPerspective()
{
  // A circle on the source, an ellipse in perspective (off-center control point) on the destination.
  const Ellipse input(QPointF(200, 150), QPointF(300, 50), QPointF(400, 150), QPointF(300, 250));
  Ellipse output(QPointF(0, 50), QPointF(100, 0), QPointF(200, 50), QPointF(100, 100));
  output.setVertex(0, QPointF(10, 60));
  output.setVertex(1, QPointF(110, 10));
  QVERIFY(output.isValid());

  const QTransform outputToInput = output.toUnitCircle() * input.fromUnitCircle();
  const QVector<TexturedVertex> vertices = ShapeGeometry::ellipse(
    TEXTURE_RECT, outputToInput, output.fromUnitCircle(), output.getCenter(), QTransform(),
    N_ELLIPSE_TRIANGLES);

  // Every vertex reads the point of the source that the homography gives,
  // and the perspective center reads the center of the source circle.
  for (const TexturedVertex& v : vertices)
    QVERIFY(near(texturePoint(v), normalized(outputToInput.map(position(v)))));
  QVERIFY(near(texturePoint(vertices[0]), normalized(input.getCenter())));
}
