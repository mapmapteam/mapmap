/*
 * TestGenerators.h
 *
 * Unit tests for the generators (src/core/Generator, Generators), the intermediate
 * image they draw into (src/core/IntermediateImage) and the frame counter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_GENERATORS_H_
#define TEST_GENERATORS_H_

#include <QtTest/QtTest>

class TestGenerators: public QObject
{
  Q_OBJECT

private slots:
  void generatorsDeclareTheirSettings();
  void frameCounterGrows();
  void sizeStaysWithinBounds();
  void sizeChangeIsSignalled();
  void colorFillsTheImage();
  void checkerStartsWithFrontColorAtTopLeft();
  void gridLinesArePixelExact();
  void stillGeneratorDrawsOnlyWhenChanged();
  void animatedGeneratorDrawsOncePerFrameAcrossContexts();
  void colorWheelTurnsWithTime();
  void intermediateImageGivesBackTheFramebuffer();
  void intermediateImagesTakeTurns();
};

#endif /* TEST_GENERATORS_H_ */
