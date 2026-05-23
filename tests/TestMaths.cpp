/*
 * TestMaths.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestMaths.h"

#include "Maths.h"

using namespace mmp;

void TestMaths::degreesRadiansRoundTrip()
{
  QVERIFY(qFuzzyCompare(degreesToRadians(180.0), M_PI));
  QVERIFY(qFuzzyCompare(radiansToDegrees(M_PI), 180.0));
  QVERIFY(qFuzzyCompare(radiansToDegrees(degreesToRadians(57.3)), 57.3));
}

void TestMaths::wrapAroundInt()
{
  // Example taken from the documentation in Maths.h.
  QCOMPARE(wrapAround(-1, 3), 2);
  QCOMPARE(wrapAround(0, 3), 0);
  QCOMPARE(wrapAround(3, 3), 0);
  QCOMPARE(wrapAround(5, 3), 2);
  QCOMPARE(wrapAround(-4, 3), 2);
}

void TestMaths::wrapAroundReal()
{
  QVERIFY(qFuzzyCompare(wrapAround(qreal(-0.5), qreal(1.0)), qreal(0.5)));
  QVERIFY(qFuzzyCompare(wrapAround(qreal(1.5), qreal(1.0)), qreal(0.5)));
  QVERIFY(qFuzzyCompare(wrapAround(qreal(0.25), qreal(1.0)), qreal(0.25)));
}

void TestMaths::squareAndDistance()
{
  QVERIFY(qFuzzyCompare(sq(3.0), 9.0));

  QPointF a(0, 0);
  QPointF b(3, 4);
  QVERIFY(qFuzzyCompare(distSq(a, b), 25.0));
  QVERIFY(qFuzzyCompare(dist(a, b), 5.0));
}

void TestMaths::distanceInside()
{
  QPointF a(0, 0);
  QPointF b(3, 4); // distance 5
  QVERIFY(distIsInside(a, b, 6.0));
  QVERIFY(!distIsInside(a, b, 5.0)); // strictly inside: equal is outside
  QVERIFY(!distIsInside(a, b, 4.0));
}

void TestMaths::booleanXor()
{
  QCOMPARE(xOr(true, false), true);
  QCOMPARE(xOr(false, true), true);
  QCOMPARE(xOr(true, true), false);
  QCOMPARE(xOr(false, false), false);
}
