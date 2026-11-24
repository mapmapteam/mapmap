/*
 * TestFrameStats.h
 *
 * Unit tests for FrameStats, the timing of each step of the image path.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_FRAME_STATS_H_
#define TEST_FRAME_STATS_H_

#include <QtTest/QtTest>

class TestFrameStats: public QObject
{
  Q_OBJECT

private slots:
  void cleanup();

  void disabledByDefault();
  void emptySummary();
  void meanAndMaximum();
  void oldSamplesLeaveTheWindow();
  void fullBufferKeepsTheNewestSamples();
  void droppedFrames();
  void pathAndName();
  void liveInstancesAreListed();
  void summaryLine();
  void emptySummaryIsNotLogged();
  void logsUnderItsOwnCategory();
  void recordsFromAnotherThread();
};

#endif /* TEST_FRAME_STATS_H_ */
