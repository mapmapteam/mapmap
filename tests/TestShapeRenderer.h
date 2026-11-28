/*
 * TestShapeRenderer.h
 *
 * Unit tests for the drawing of shapes with shaders (src/core/ShapeRenderer).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_SHAPE_RENDERER_H_
#define TEST_SHAPE_RENDERER_H_

#include <QtTest/QtTest>

class TestShapeRenderer: public QObject
{
  Q_OBJECT

private slots:
  void viewMatrixMapsViewportCorners();
  void viewMatrixAppliesDeviceTransform();
  void drawingNeedsAContext();
  void drawsRgbaTexture();
  void opacityScalesAlpha();
  void textureCoordinateIsDividedByQ();
};

#endif /* TEST_SHAPE_RENDERER_H_ */
