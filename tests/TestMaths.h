/*
 * TestMaths.h
 *
 * Unit tests for the math helpers in src/core/Maths.h.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_MATHS_H_
#define TEST_MATHS_H_

#include <QtTest/QtTest>

class TestMaths: public QObject
{
  Q_OBJECT

private slots:
  void degreesRadiansRoundTrip();
  void wrapAroundInt();
  void wrapAroundReal();
  void squareAndDistance();
  void distanceInside();
  void booleanXor();
};

#endif /* TEST_MATHS_H_ */
