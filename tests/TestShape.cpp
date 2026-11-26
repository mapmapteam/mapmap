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
#include "Mesh.h"
#include "ShapeHandles.h"

#include <cmath>

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
  return Ellipse(QPointF(6, 20), QPointF(10, 18), QPointF(14, 20), QPointF(10, 22));
}

// Intersection of lines (a,b) and (c,d).
QPointF intersect(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d)
{
  QPointF p;
  QLineF(a, b).intersects(QLineF(c, d), &p);
  return p;
}

// A non-square rectangle from (0,0) to (4,2), corners in drawing order:
// top-left, top-right, bottom-right, bottom-left.
Quad makeRectangle()
{
  return Quad(QPointF(0, 0), QPointF(4, 0), QPointF(4, 2), QPointF(0, 2));
}

bool sameVertices(const MShape& shape, const QVector<QPointF>& expected)
{
  if (shape.nVertices() != expected.size())
    return false;
  for (int i = 0; i < expected.size(); ++i)
    if (!nearPoint(shape.getVertex(i), expected.at(i)))
      return false;
  return true;
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

void TestShape::flipTwiceRestoresShape()
{
  const Quad original = makeRectangle();
  for (MShape::FlipDirection d : { MShape::Horizontal, MShape::Vertical })
  {
    Quad shape = makeRectangle();
    shape.applyTransform(shape.flipTransform(d));
    QVERIFY(!sameVertices(shape, original.getVertices()));
    shape.applyTransform(shape.flipTransform(d));
    QVERIFY(sameVertices(shape, original.getVertices()));

    Quad image = makeRectangle();
    image.applyTransform(image.imageFlipTransform(d));
    image.applyTransform(image.imageFlipTransform(d));
    QVERIFY(sameVertices(image, original.getVertices()));
  }
}

void TestShape::rotateShapeTurnsOutline()
{
  // In the output view, a quarter turn turns the outline itself:
  // a 4 x 2 rectangle becomes 2 x 4, around the same center.
  Quad shape = makeRectangle();
  shape.applyTransform(shape.rotateTransform(MShape::Rotate90CW));

  const QRectF bounds = shape.getBoundingRect();
  QVERIFY(qAbs(bounds.width() - 2) < 1e-9);
  QVERIFY(qAbs(bounds.height() - 4) < 1e-9);
  QVERIFY(nearPoint(bounds.center(), QPointF(2, 1)));
  // Clockwise on screen (y down): the top-left corner goes to the top-right.
  QVERIFY(nearPoint(shape.getVertex(0), QPointF(3, -1)));
}

void TestShape::rotateFullTurnRestoresShape()
{
  const Quad original = makeRectangle();

  Quad shape = makeRectangle();
  for (int i = 0; i < 4; ++i)
    shape.applyTransform(shape.rotateTransform(MShape::Rotate90CW));
  QVERIFY(sameVertices(shape, original.getVertices()));

  Quad image = makeRectangle();
  for (int i = 0; i < 4; ++i)
    image.applyTransform(image.imageRotateTransform(MShape::Rotate90CW));
  QVERIFY(sameVertices(image, original.getVertices()));

  Quad back = makeRectangle();
  back.applyTransform(back.imageRotateTransform(MShape::Rotate90CW));
  back.applyTransform(back.imageRotateTransform(MShape::Rotate90CCW));
  QVERIFY(sameVertices(back, original.getVertices()));

  Quad half = makeRectangle();
  half.applyTransform(half.imageRotateTransform(MShape::Rotate180));
  half.applyTransform(half.imageRotateTransform(MShape::Rotate180));
  QVERIFY(sameVertices(half, original.getVertices()));
}

void TestShape::imageFlipKeepsOutline()
{
  // Flipping the picked region swaps which corner of the image each output
  // corner shows; the outline in the input view does not move.
  Quad image = makeRectangle();
  image.applyTransform(image.imageFlipTransform(MShape::Horizontal));
  QVERIFY(sameVertices(image, { QPointF(4, 0), QPointF(0, 0), QPointF(0, 2), QPointF(4, 2) }));

  Quad vertical = makeRectangle();
  vertical.applyTransform(vertical.imageFlipTransform(MShape::Vertical));
  QVERIFY(sameVertices(vertical, { QPointF(0, 2), QPointF(4, 2), QPointF(4, 0), QPointF(0, 0) }));
}

void TestShape::imageRotateKeepsOutline()
{
  // Turning the image clockwise: the output's top-left corner must now show
  // the image's bottom-left corner, and so on around. The input outline stays
  // the same 4 x 2 rectangle (it is not squashed into 2 x 4).
  Quad cw = makeRectangle();
  cw.applyTransform(cw.imageRotateTransform(MShape::Rotate90CW));
  QVERIFY(sameVertices(cw, { QPointF(0, 2), QPointF(0, 0), QPointF(4, 0), QPointF(4, 2) }));

  Quad ccw = makeRectangle();
  ccw.applyTransform(ccw.imageRotateTransform(MShape::Rotate90CCW));
  QVERIFY(sameVertices(ccw, { QPointF(4, 0), QPointF(4, 2), QPointF(0, 2), QPointF(0, 0) }));

  Quad half = makeRectangle();
  half.applyTransform(half.imageRotateTransform(MShape::Rotate180));
  QVERIFY(sameVertices(half, { QPointF(4, 2), QPointF(0, 2), QPointF(0, 0), QPointF(4, 0) }));
}

void TestShape::imageOrientationsAreAllDistinct()
{
  // The eight orientations of an image (four quarter turns, each with or
  // without a mirror) must each keep the outline and be different from one
  // another: no two combinations of commands may collapse to the same result
  // unless they really are the same orientation.
  const QVector<QPointF> corners = makeRectangle().getVertices();
  QList<QVector<QPointF>> seen;

  for (bool mirrored : { false, true })
  {
    for (int quarterTurns = 0; quarterTurns < 4; ++quarterTurns)
    {
      Quad image = makeRectangle();
      if (mirrored)
        image.applyTransform(image.imageFlipTransform(MShape::Horizontal));
      for (int i = 0; i < quarterTurns; ++i)
        image.applyTransform(image.imageRotateTransform(MShape::Rotate90CW));

      // Outline kept: every vertex is still one of the four corners.
      for (const QPointF& v : image.getVertices())
      {
        bool isCorner = false;
        for (const QPointF& c : corners)
          isCorner |= nearPoint(v, c);
        QVERIFY2(isCorner, "image orientation moved the input outline");
      }

      for (const QVector<QPointF>& other : seen)
      {
        Quad tmp(other.at(0), other.at(1), other.at(2), other.at(3));
        QVERIFY2(!sameVertices(tmp, image.getVertices()), "two orientations collapsed into one");
      }
      seen.append(image.getVertices());
    }
  }
  QCOMPARE(seen.size(), 8);

  // Flipping both ways is the same as a half turn.
  Quad both = makeRectangle();
  both.applyTransform(both.imageFlipTransform(MShape::Horizontal));
  both.applyTransform(both.imageFlipTransform(MShape::Vertical));
  Quad half = makeRectangle();
  half.applyTransform(half.imageRotateTransform(MShape::Rotate180));
  QVERIFY(sameVertices(both, half.getVertices()));
}

void TestShape::degenerateShapeHasFallbackDirection()
{
  Quad point(QPointF(5, 5), QPointF(5, 5), QPointF(5, 5), QPointF(5, 5));
  QVERIFY(nearPoint(handleAnchor(point).outward, QPointF(1, 0)));
  QVERIFY(nearPoint(handleAnchor(point).along, QPointF(0, 1)));
  QVERIFY(nearPoint(handlePosition(point, 28), QPointF(33, 5)));
}

namespace {
// A point at distance 10 from the origin, at the given angle (clockwise on screen).
QPointF pointAtDegrees(qreal degrees)
{
  return QPointF(10 * std::cos(qDegreesToRadians(degrees)), 10 * std::sin(qDegreesToRadians(degrees)));
}
}

void TestShape::rotateDragQuarterTurn()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Rotate);
  drag.moveTo(QPointF(0, 10), false);
  QCOMPARE(drag.rotation(), 90.0);
  QVERIFY(nearPoint(drag.transform().map(QPointF(10, 0)), QPointF(0, 10)));
}

void TestShape::rotateDragSteps()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Rotate);
  drag.setStepped(true);
  drag.moveTo(pointAtDegrees(20), false);
  QCOMPARE(drag.rotation(), 15.0);
  drag.moveTo(pointAtDegrees(25), false);
  QCOMPARE(drag.rotation(), 30.0);
}

void TestShape::rotateDragFine()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Rotate);
  drag.moveTo(QPointF(0, 10), true);
  QCOMPARE(drag.rotation(), 9.0);
}

void TestShape::fineToggleMidDragDoesNotJump()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Rotate);
  drag.moveTo(QPointF(10, 10), false); // 45 degrees
  drag.moveTo(QPointF(10, 10), true);  // Ctrl pressed, pointer still
  QCOMPARE(drag.rotation(), 45.0);
  drag.moveTo(QPointF(0, 10), true);   // 45 degrees more, at a tenth
  QCOMPARE(drag.rotation(), 49.5);
}

void TestShape::rotationAccumulatesPastHalfTurn()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Rotate);
  for (const QPointF& p : { QPointF(0, 10), QPointF(-10, 0), QPointF(0, -10), QPointF(10, 0) })
    drag.moveTo(p, false);
  QCOMPARE(drag.rotation(), 360.0);
}

void TestShape::dragThroughCenterIsIgnored()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Scale);
  drag.moveTo(QPointF(0, 0), false);
  QCOMPARE(drag.scale(), 1.0);
  drag.moveTo(QPointF(20, 0), false);
  QCOMPARE(drag.scale(), 2.0);
}

void TestShape::scaleDrag()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Scale);
  drag.moveTo(QPointF(20, 0), false);
  QCOMPARE(drag.scale(), 2.0);
  QVERIFY(nearPoint(drag.transform().map(QPointF(10, 0)), QPointF(20, 0)));
  drag.moveTo(QPointF(0, 15), false); // only the distance to the center counts
  QCOMPARE(drag.scale(), 1.5);
  QCOMPARE(drag.rotation(), 0.0);
}

void TestShape::scaleDragSteps()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Scale);
  drag.setStepped(true);
  drag.moveTo(QPointF(12.3, 0), false);
  QCOMPARE(drag.scale(), 1.2);
  drag.moveTo(QPointF(12.7, 0), false);
  QCOMPARE(drag.scale(), 1.3);
}

void TestShape::scaleDragFine()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Scale);
  drag.moveTo(QPointF(20, 0), true);
  QVERIFY(qFuzzyCompare(drag.scale(), std::pow(2.0, 0.1)));
}

void TestShape::scaleNeverBelowMinimum()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Scale);
  drag.moveTo(QPointF(0.01, 0), false);
  QCOMPARE(drag.scale(), HandleDrag::MIN_SCALE);
  drag.setStepped(true);
  QCOMPARE(drag.scale(), HandleDrag::MIN_SCALE);
}

void TestShape::freeDragKeepsHandleUnderPointer()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Free);
  drag.moveTo(QPointF(0, 20), false);
  QCOMPARE(drag.rotation(), 90.0);
  QCOMPARE(drag.scale(), 2.0);
  QVERIFY(nearPoint(drag.transform().map(QPointF(10, 0)), QPointF(0, 20)));
}

void TestShape::freeDragSteps()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Free);
  drag.setStepped(true);
  drag.moveTo(12.3 * pointAtDegrees(20) / 10, false);
  QCOMPARE(drag.rotation(), 15.0);
  QCOMPARE(drag.scale(), 1.2);
}

void TestShape::switchingModeMidDragDoesNotJump()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Free);
  drag.moveTo(QPointF(0, 20), false);   // 90 degrees, x2
  drag.setMode(HandleMode::Rotate);
  drag.moveTo(QPointF(-20, 0), false);  // turns on, size kept
  QCOMPARE(drag.rotation(), 180.0);
  QCOMPARE(drag.scale(), 2.0);
  drag.setMode(HandleMode::Scale);
  drag.moveTo(QPointF(-30, 0), false);  // grows, angle kept
  QCOMPARE(drag.rotation(), 180.0);
  QCOMPARE(drag.scale(), 3.0);
}

void TestShape::nextModeCycles()
{
  QCOMPARE(nextHandleMode(HandleMode::Free), HandleMode::Rotate);
  QCOMPARE(nextHandleMode(HandleMode::Rotate), HandleMode::Scale);
  QCOMPARE(nextHandleMode(HandleMode::Scale), HandleMode::Free);
}

void TestShape::shiftRoundsOnlyWhatTheModeChanges()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Free);
  drag.moveTo(QPointF(12.34, 0), false); // scale 1.234
  drag.setMode(HandleMode::Rotate);
  drag.setStepped(true);
  drag.moveTo(12.34 * pointAtDegrees(20) / 10, false);
  QCOMPARE(drag.rotation(), 15.0);
  QCOMPARE(drag.scale(), 1.234);
}

void TestShape::frozenValueKeepsShownValue()
{
  HandleDrag drag(QPointF(0, 0), QPointF(10, 0), HandleMode::Free);
  drag.setStepped(true);
  drag.moveTo(12.3 * pointAtDegrees(20) / 10, false); // shown: 15 degrees, x1.2
  drag.setMode(HandleMode::Rotate);                    // the size freezes at 1.2
  drag.setStepped(false);
  QCOMPARE(drag.scale(), 1.2);
}

void TestShape::singleHandleAnchors()
{
  Quad rect(QPointF(0, 0), QPointF(40, 0), QPointF(40, 20), QPointF(0, 20));
  QVERIFY(nearPoint(handleAnchor(rect).point, QPointF(40, 10)));
  QVERIFY(nearPoint(handleAnchor(rect).outward, QPointF(1, 0)));
  QVERIFY(nearPoint(handleAnchor(rect).along, QPointF(0, 1)));
  QVERIFY(!handleAnchor(rect).isMirrored());

  Triangle tri(QPointF(0, 30), QPointF(40, 30), QPointF(20, 0));
  const qreal a = 30 / std::hypot(30.0, 20.0);
  const qreal b = 20 / std::hypot(30.0, 20.0);
  QVERIFY(nearPoint(handleAnchor(tri).point, QPointF(30, 15)));
  QVERIFY(nearPoint(handleAnchor(tri).outward, QPointF(a, -b)));
  QVERIFY(nearPoint(handleAnchor(tri).along, QPointF(b, a)));
  QVERIFY(!handleAnchor(tri).isMirrored());

  Mesh mesh(QPointF(0, 0), QPointF(40, 0), QPointF(40, 20), QPointF(0, 20));
  mesh.resize(3, 2);
  QVERIFY(nearPoint(handleAnchor(mesh).point, QPointF(40, 10)));
  QVERIFY(!handleAnchor(mesh).isMirrored());

  Ellipse ellipse(QPointF(0, 10), QPointF(20, 0), QPointF(40, 10), QPointF(20, 20));
  QVERIFY(nearPoint(handleAnchor(ellipse).point, QPointF(40, 10)));
  QVERIFY(nearPoint(handleAnchor(ellipse).along, QPointF(0, 1)));
  QVERIFY(!handleAnchor(ellipse).isMirrored());

  QVERIFY(nearPoint(handlePosition(rect, 28), QPointF(68, 10)));
}

void TestShape::mirroredShapesAreDetected()
{
  Quad mirrored(QPointF(40, 0), QPointF(0, 0), QPointF(0, 20), QPointF(40, 20));
  QVERIFY(handleAnchor(mirrored).isMirrored());
  Triangle tri(QPointF(40, 30), QPointF(0, 30), QPointF(20, 0));
  QVERIFY(handleAnchor(tri).isMirrored());
  Ellipse ellipse(QPointF(40, 10), QPointF(20, 0), QPointF(0, 10), QPointF(20, 20));
  QVERIFY(handleAnchor(ellipse).isMirrored());

  // Upside down is a mirror too.
  Quad flipped(QPointF(0, 20), QPointF(40, 20), QPointF(40, 0), QPointF(0, 0));
  QVERIFY(handleAnchor(flipped).isMirrored());
}
