/*
 * TestShapeGeometry.h
 *
 * Unit tests for the vertices that draw shapes (src/core/ShapeGeometry).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_SHAPE_GEOMETRY_H_
#define TEST_SHAPE_GEOMETRY_H_

#include <QtTest/QtTest>

class TestShapeGeometry: public QObject
{
  Q_OBJECT

private slots:
  void rectangleShowsWholeTexture();
  void triangleMapsSourceToDestination();
  void parallelogramIsNotSplit();
  void trapezoidIsSplitDownToMinimumArea();
  void depthLimitsSplits();
  void subQuadsCoverTheQuad();
  void quadIsTwoTriangles();
  void ellipseIsAFanOfTriangles();
  void ellipseOnItselfShowsTextureAsItIs();
  void ellipseFollowsPerspective();
};

#endif /* TEST_SHAPE_GEOMETRY_H_ */
