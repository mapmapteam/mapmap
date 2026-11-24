/*
 * TestFrameStats.cpp
 *
 * Unit tests for FrameStats, the timing of each step of the image path.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestFrameStats.h"

#include <QThread>

#include "FrameStats.h"
#include "MM.h"

using namespace mmp;

namespace {

const qint64 MS = MM::NANOSECONDS_PER_MILLISECOND;
const qint64 SECOND = MM::NANOSECONDS_PER_SECOND;

// An arbitrary point in time, far from zero.
const qint64 T0 = 1000 * SECOND;

// Number of samples recorded by the thread test.
const int THREAD_SAMPLE_COUNT = 10000;

// Messages logged under the category of FrameStats, caught by _catchMessages().
QStringList _caughtMessages;
QtMessageHandler _previousHandler = nullptr;

void _catchMessages(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  if (QLatin1String(context.category) == QLatin1String("mapmap.framestats"))
    _caughtMessages << msg;
  else if (_previousHandler)
    _previousHandler(type, context, msg);
}

}

void TestFrameStats::cleanup()
{
  FrameStats::setEnabled(false);
  FrameStats::setSynchronous(false);
}

void TestFrameStats::disabledByDefault()
{
  QVERIFY(!FrameStats::isEnabled());
  QVERIFY(!FrameStats::isSynchronous());

  FrameStats::setEnabled(true);
  QVERIFY(FrameStats::isEnabled());
  FrameStats::setSynchronous(true);
  QVERIFY(FrameStats::isSynchronous());
}

void TestFrameStats::emptySummary()
{
  FrameStats stats("clip.mp4");
  const FrameStats::Summary summary = stats.summary(T0);
  for (int stage = 0; stage < FrameStats::STAGE_COUNT; stage++)
    QCOMPARE(summary.stages[stage].count, 0);
  QCOMPARE(summary.dropped, 0);
  QVERIFY(summary.isEmpty());
}

void TestFrameStats::meanAndMaximum()
{
  FrameStats stats("clip.mp4");
  stats.record(FrameStats::PREPARE, 2 * MS, T0);
  stats.record(FrameStats::PREPARE, 4 * MS, T0 + 100 * MS);
  stats.record(FrameStats::PREPARE, 9 * MS, T0 + 200 * MS);
  stats.record(FrameStats::UPLOAD, 1 * MS, T0 + 200 * MS);

  const FrameStats::Summary summary = stats.summary(T0 + 300 * MS);
  QCOMPARE(summary.stages[FrameStats::PREPARE].count, 3);
  QCOMPARE(summary.stages[FrameStats::PREPARE].meanNs, 5 * MS);
  QCOMPARE(summary.stages[FrameStats::PREPARE].maxNs, 9 * MS);
  QCOMPARE(summary.stages[FrameStats::UPLOAD].count, 1);
  QCOMPARE(summary.stages[FrameStats::UPLOAD].meanNs, 1 * MS);
  QCOMPARE(summary.stages[FrameStats::DRAW].count, 0);
  QVERIFY(!summary.isEmpty());
}

void TestFrameStats::oldSamplesLeaveTheWindow()
{
  FrameStats stats("output");
  stats.record(FrameStats::DRAW, 50 * MS, T0);
  stats.record(FrameStats::DRAW, 3 * MS, T0 + 900 * MS);

  // Both within the last second.
  QCOMPARE(stats.summary(T0 + SECOND - 1).stages[FrameStats::DRAW].count, 2);

  // The first one is more than a second old.
  const FrameStats::Summary summary = stats.summary(T0 + SECOND + 1);
  QCOMPARE(summary.stages[FrameStats::DRAW].count, 1);
  QCOMPARE(summary.stages[FrameStats::DRAW].maxNs, 3 * MS);
}

void TestFrameStats::fullBufferKeepsTheNewestSamples()
{
  FrameStats stats("output");
  // Large old samples, then enough small ones to overwrite all of them.
  for (int i = 0; i < FrameStats::SAMPLE_CAPACITY; i++)
    stats.record(FrameStats::INTERVAL, 100 * MS, T0);
  for (int i = 0; i < FrameStats::SAMPLE_CAPACITY; i++)
    stats.record(FrameStats::INTERVAL, 1 * MS, T0 + 1);

  const FrameStats::Summary summary = stats.summary(T0 + 2);
  QCOMPARE(summary.stages[FrameStats::INTERVAL].count, int(FrameStats::SAMPLE_CAPACITY));
  QCOMPARE(summary.stages[FrameStats::INTERVAL].maxNs, 1 * MS);
}

void TestFrameStats::droppedFrames()
{
  FrameStats stats("clip.mp4");
  stats.recordDropped(T0);
  stats.recordDropped(T0 + 500 * MS);
  QCOMPARE(stats.summary(T0 + 600 * MS).dropped, 2);
  QCOMPARE(stats.summary(T0 + 1200 * MS).dropped, 1);
  QVERIFY(!stats.summary(T0 + 1200 * MS).isEmpty());
}

void TestFrameStats::pathAndName()
{
  FrameStats stats;
  stats.setName("camera");
  stats.setPath("YUV on the GPU, NV12");
  const FrameStats::Summary summary = stats.summary(T0);
  QCOMPARE(summary.name, QString("camera"));
  QCOMPARE(summary.path, QString("YUV on the GPU, NV12"));
  QCOMPARE(stats.name(), QString("camera"));
}

void TestFrameStats::liveInstancesAreListed()
{
  FrameStats first("first");
  QList<FrameStats::Summary> summaries;
  {
    FrameStats second("second");
    summaries = FrameStats::summaries(T0);
    QStringList names;
    for (const FrameStats::Summary& summary : summaries)
      names << summary.name;
    QVERIFY(names.indexOf("first") >= 0);
    QVERIFY(names.indexOf("second") > names.indexOf("first"));
  }

  summaries = FrameStats::summaries(T0);
  QStringList names;
  for (const FrameStats::Summary& summary : summaries)
    names << summary.name;
  QVERIFY(names.contains("first"));
  QVERIFY(!names.contains("second"));
}

void TestFrameStats::summaryLine()
{
  FrameStats stats("clip.mp4");
  stats.setPath("YUV on the GPU, NV12");
  stats.record(FrameStats::PREPARE, 9 * MS, T0);
  stats.record(FrameStats::PREPARE, 11 * MS, T0);
  stats.recordDropped(T0);

  const QString line = FrameStats::summaryLine(stats.summary(T0));
  QVERIFY2(line.startsWith("clip.mp4 (YUV on the GPU, NV12):"), qPrintable(line));
  QVERIFY2(line.contains("prepare 10.00 ms, max 11.00 ms, 2/s"), qPrintable(line));
  QVERIFY2(line.contains("dropped 1/s"), qPrintable(line));
  // Stages without samples are left out.
  QVERIFY2(!line.contains("upload"), qPrintable(line));

  // No path: no parentheses.
  FrameStats view("output");
  view.record(FrameStats::DRAW, 2 * MS, T0);
  QVERIFY2(FrameStats::summaryLine(view.summary(T0)).startsWith("output: draw"),
           qPrintable(FrameStats::summaryLine(view.summary(T0))));
}

void TestFrameStats::emptySummaryIsNotLogged()
{
  _caughtMessages.clear();
  _previousHandler = qInstallMessageHandler(_catchMessages);
  {
    FrameStats stats("idle source");
    FrameStats::logSummaries();
  }
  qInstallMessageHandler(_previousHandler);

  for (const QString& message : std::as_const(_caughtMessages))
    QVERIFY2(!message.contains("idle source"), qPrintable(message));
}

void TestFrameStats::logsUnderItsOwnCategory()
{
  QCOMPARE(QLatin1String(lcFrameStats().categoryName()), QLatin1String("mapmap.framestats"));

  _caughtMessages.clear();
  _previousHandler = qInstallMessageHandler(_catchMessages);
  {
    FrameStats stats("logged source");
    stats.record(FrameStats::UPLOAD, 5 * MS);
    FrameStats::logSummaries();
  }
  qInstallMessageHandler(_previousHandler);

  bool found = false;
  for (const QString& message : std::as_const(_caughtMessages))
    found = found || message.contains("logged source: upload 5.00 ms");
  QVERIFY2(found, qPrintable(_caughtMessages.join("\n")));
}

void TestFrameStats::recordsFromAnotherThread()
{
  FrameStats stats("clip.mp4");
  QThread* thread = QThread::create([&stats] {
    for (int i = 0; i < THREAD_SAMPLE_COUNT; i++)
      stats.record(FrameStats::PREPARE, 1 * MS);
  });
  thread->start();
  // Read while the other thread writes.
  while (!thread->isFinished())
    stats.summary();
  thread->wait();
  delete thread;

  const int count = stats.summary().stages[FrameStats::PREPARE].count;
  QVERIFY(count > 0 && count <= FrameStats::SAMPLE_CAPACITY);
}
