/*
 * TestShape.h
 *
 * Unit tests for the shape geometry in src/shape (MShape / Polygon).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_SHAPE_H_
#define TEST_SHAPE_H_

#include <QtTest/QtTest>

class TestShape: public QObject
{
  Q_OBJECT

private slots:
  void nextModeCycles();
  void shiftRoundsOnlyWhatTheModeChanges();
  void frozenValueKeepsShownValue();
  void singleHandleAnchors();
  void mirroredShapesAreDetected();
  void freeDragKeepsHandleUnderPointer();
  void freeDragSteps();
  void switchingModeMidDragDoesNotJump();
  void rotateDragQuarterTurn();
  void rotateDragSteps();
  void rotateDragFine();
  void fineToggleMidDragDoesNotJump();
  void rotationAccumulatesPastHalfTurn();
  void dragThroughCenterIsIgnored();
  void scaleDrag();
  void scaleDragSteps();
  void scaleDragFine();
  void scaleNeverBelowMinimum();
  void degenerateShapeHasFallbackDirection();
  void verticesAccessors();
  void getCenter();
  void includesPoint();
  void translate();
  void applyTransform();
  void polygonRoundTrip();
  void ellipseAffineCenter();
  void ellipseAxisModeConstrainsVertices();
  void ellipsePerspectiveVerticesAreFree();
  void ellipsePerspectiveContour();
  void ellipseRejectsInvalidPerspective();
  void ellipseCenterDragTranslates();
  void ellipseResetsOffCenterControl();
  void flipTwiceRestoresShape();
  void rotateShapeTurnsOutline();
  void rotateFullTurnRestoresShape();
  void imageFlipKeepsOutline();
  void imageRotateKeepsOutline();
  void imageOrientationsAreAllDistinct();
};

#endif /* TEST_SHAPE_H_ */
