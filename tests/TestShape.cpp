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
#include "Ellipse.h"

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

bool nearPoint(const QPointF& a, const QPointF& b, qreal eps=1e-6)
{
  return QLineF(a, b).length() < eps;
}

// Axis-aligned ellipse centered at (10,20) with radii 4 and 2, with center control.
Ellipse makeEllipse()
{
  return Ellipse(QPointF(6, 20), QPointF(10, 18), QPointF(14, 20), QPointF(10, 22), true);
}

// Intersection of lines (a,b) and (c,d).
QPointF intersect(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d)
{
  QPointF p;
  QLineF(a, b).intersects(QLineF(c, d), &p);
  return p;
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

void TestShape::ellipseAffineCenter()
{
  Ellipse ellipse = makeEllipse();
  QVERIFY(ellipse.isValid());
  QVERIFY(nearPoint(ellipse.getCenter(), QPointF(10, 20)));
  QVERIFY(nearPoint(ellipse.getVertex(4), QPointF(10, 20)));

  // Quarter points of the unit circle map onto vertices 0-3.
  QTransform t = ellipse.fromUnitCircle();
  QVERIFY(nearPoint(t.map(QPointF(-1, 0)), ellipse.getVertex(0)));
  QVERIFY(nearPoint(t.map(QPointF(0, -1)), ellipse.getVertex(1)));
  QVERIFY(nearPoint(t.map(QPointF(1, 0)),  ellipse.getVertex(2)));
  QVERIFY(nearPoint(t.map(QPointF(0, 1)),  ellipse.getVertex(3)));

  QVERIFY(ellipse.includesPoint(10, 20));
  QVERIFY(ellipse.includesPoint(13, 20));
  QVERIFY(!ellipse.includesPoint(10, 23));
}

void TestShape::ellipseAxisModeConstrainsVertices()
{
  Ellipse ellipse = makeEllipse();
  ellipse.setPerspective(false);

  // Vertex 1 gets projected back onto the vertical axis; vertex 3 mirrors it.
  ellipse.setVertex(1, QPointF(12, 17));
  QVERIFY(nearPoint(ellipse.getVertex(1), QPointF(10, 17)));
  QVERIFY(nearPoint(ellipse.getVertex(3), QPointF(10, 23)));
  QVERIFY(nearPoint(ellipse.getVertex(4), QPointF(10, 20)));
}

void TestShape::ellipsePerspectiveVerticesAreFree()
{
  Ellipse ellipse = makeEllipse();
  QVERIFY(ellipse.isPerspective());

  // Squash the top half: vertex 1 moves closer to the horizontal axis and sideways.
  ellipse.setVertex(1, QPointF(11, 19));
  QVERIFY(nearPoint(ellipse.getVertex(1), QPointF(11, 19)));
  QVERIFY(nearPoint(ellipse.getVertex(3), QPointF(10, 22)));
  QVERIFY(ellipse.isValid());

  // Perspective center lies at the intersection of chords 0-2 and 1-3.
  QPointF expected = intersect(ellipse.getVertex(0), ellipse.getVertex(2),
                               ellipse.getVertex(1), ellipse.getVertex(3));
  QVERIFY(nearPoint(ellipse.getCenter(), expected));
  QVERIFY(nearPoint(ellipse.getVertex(4), expected));

  // Switching off perspective brings back an axis-aligned ellipse.
  ellipse.setPerspective(false);
  QVERIFY(nearPoint(ellipse.getVertex(1), QPointF(10, 19)));
  QVERIFY(nearPoint(ellipse.getVertex(3), QPointF(10, 21)));
}

void TestShape::ellipsePerspectiveContour()
{
  Ellipse ellipse = makeEllipse();
  ellipse.setVertex(0, QPointF(7, 21));
  ellipse.setVertex(1, QPointF(11, 19));
  QVERIFY(ellipse.isValid());

  // Contour passes through all four control points...
  QPolygonF contour = ellipse.toPolygon(4);
  QCOMPARE(contour.size(), 4);
  QVERIFY(nearPoint(contour.at(0), ellipse.getVertex(2)));
  QVERIFY(nearPoint(contour.at(1), ellipse.getVertex(3)));
  QVERIFY(nearPoint(contour.at(2), ellipse.getVertex(0)));
  QVERIFY(nearPoint(contour.at(3), ellipse.getVertex(1)));

  // ... and every contour point satisfies a single conic equation (fit on 5 points, check others).
  QPolygonF points = ellipse.toPolygon(64);
  QTransform toUnit = ellipse.toUnitCircle();
  for (const QPointF& p : points)
    QVERIFY(qAbs(QVector2D(toUnit.map(p)).length() - 1) < 1e-6);

  QVERIFY(ellipse.includesPoint(ellipse.getCenter()));
}

void TestShape::ellipseRejectsInvalidPerspective()
{
  Ellipse ellipse = makeEllipse();

  // Pulling vertex 3 very far would turn the circle's image into a hyperbola.
  ellipse.setVertex(3, QPointF(10, 1000));
  QVERIFY(ellipse.isValid());
  QVERIFY(ellipse.getVertex(3).y() > 22);
  QVERIFY(ellipse.getVertex(3).y() < 1000);

  // Crossing vertices (wrong cyclic order) is not allowed either.
  Ellipse other = makeEllipse();
  other.setVertex(1, QPointF(10, 30));
  QVERIFY(other.isValid());
  QVERIFY(other.getVertex(1).y() < 22);
}

void TestShape::ellipseCenterDragTranslates()
{
  Ellipse ellipse = makeEllipse();
  ellipse.setVertex(1, QPointF(11, 19));
  QPointF center = ellipse.getCenter();
  QVector<QPointF> before = ellipse.getVertices();

  ellipse.setVertex(4, center + QPointF(3, -2));
  for (int i=0; i<5; i++)
    QVERIFY(nearPoint(ellipse.getVertex(i), before[i] + QPointF(3, -2)));
}

void TestShape::ellipseResetsOffCenterControl()
{
  // Old projects stored a free center control point: it now gets recomputed.
  Ellipse ellipse(QPointF(6, 20), QPointF(10, 18), QPointF(14, 20), QPointF(10, 22), QPointF(12, 21));
  QVERIFY(nearPoint(ellipse.getVertex(4), QPointF(10, 20)));
}
