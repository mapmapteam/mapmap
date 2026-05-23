/*
 * TestShape.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestShape.h"

#include "Quad.h"
#include "Triangle.h"

using namespace mmp;

namespace {
// A unit-ish axis-aligned square from (0,0) to (2,2).
Quad makeSquare()
{
  return Quad(QPointF(0, 0), QPointF(2, 0), QPointF(2, 2), QPointF(0, 2));
}

bool fuzzyPoint(const QPointF& a, const QPointF& b)
{
  return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
}
}

void TestShape::verticesAccessors()
{
  // Triangle has 3 vertices and (size <= 3) so setVertex does not constrain.
  Triangle tri(QPointF(0, 0), QPointF(4, 0), QPointF(0, 3));
  QCOMPARE(tri.nVertices(), 3);
  QVERIFY(fuzzyPoint(tri.getVertex(1), QPointF(4, 0)));

  tri.setVertex(0, QPointF(1, 1));
  QVERIFY(fuzzyPoint(tri.getVertex(0), QPointF(1, 1)));

  // setVertices replaces the whole set (deep copy).
  QVector<QPointF> newVerts{ QPointF(5, 5), QPointF(6, 6), QPointF(7, 7) };
  tri.setVertices(newVerts);
  QCOMPARE(tri.nVertices(), 3);
  QVERIFY(fuzzyPoint(tri.getVertex(2), QPointF(7, 7)));
}

void TestShape::getCenter()
{
  Quad square = makeSquare();
  QVERIFY(fuzzyPoint(square.getCenter(), QPointF(1, 1)));
}

void TestShape::includesPoint()
{
  Quad square = makeSquare();
  QVERIFY(square.includesPoint(QPointF(1, 1)));
  QVERIFY(square.includesPoint(QPointF(0.5, 1.5)));
  QVERIFY(!square.includesPoint(QPointF(3, 3)));
  QVERIFY(!square.includesPoint(QPointF(-1, -1)));
}

void TestShape::translate()
{
  Quad square = makeSquare();
  square.translate(QPointF(1, 1));

  QVERIFY(fuzzyPoint(square.getVertex(0), QPointF(1, 1)));
  QVERIFY(fuzzyPoint(square.getVertex(2), QPointF(3, 3)));
  QVERIFY(fuzzyPoint(square.getCenter(), QPointF(2, 2)));
}

void TestShape::applyTransform()
{
  Quad square = makeSquare();

  QTransform t;
  t.translate(5, 0);
  square.applyTransform(t);

  QVERIFY(fuzzyPoint(square.getVertex(0), QPointF(5, 0)));
  QVERIFY(fuzzyPoint(square.getVertex(1), QPointF(7, 0)));
}

void TestShape::polygonRoundTrip()
{
  Quad square = makeSquare();

  QPolygonF poly = square.toPolygon();
  QCOMPARE(poly.size(), 4);
  QVERIFY(fuzzyPoint(poly.at(2), QPointF(2, 2)));

  // Shift every point and feed it back; the shape must reflect the change.
  QPolygonF shifted = poly.translated(10, 10);
  square.fromPolygon(shifted);
  QVERIFY(fuzzyPoint(square.getVertex(0), QPointF(10, 10)));
  QVERIFY(fuzzyPoint(square.getCenter(), QPointF(11, 11)));
}
