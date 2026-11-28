/*
 * main.cpp
 *
 * Test runner for the MapMap unit tests. Runs every test suite in a single
 * executable and returns a non-zero status if any suite fails.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QApplication>
#include <QtTest/QtTest>

#ifdef Q_OS_MACOS
#include <QTimer>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "TestMaths.h"
#include "TestUtil.h"
#include "TestFrameStats.h"
#include "TestUidAllocator.h"
#include "TestShape.h"
#include "TestShapeGeometry.h"
#include "TestHandleModeKeys.h"
#include "TestOsc.h"
#include "TestFileVersion.h"
#include "TestVideo.h"
#include "TestCameraSource.h"
#include "TestMappingManager.h"
#include "TestProjectRoundTrip.h"
#include "TestSettings.h"
#include "TestSettingFormat.h"
#include "TestInspector.h"

#ifdef Q_OS_MACOS
namespace {

/// How often the macOS main run loop is run by hand, in milliseconds.
const int MAIN_RUN_LOOP_INTERVAL_MS = 5;

/**
 * Runs the macOS main run loop at a steady pace when Qt's event loop does
 * not, with a timer owned by the application.
 *
 * AVFoundation, which plays video on macOS, reports through the main run
 * loop: a file is loaded and frames are delivered only when that loop runs.
 * The native "cocoa" platform runs it, but the "offscreen" one used by the
 * tests does not (with Homebrew's Qt it runs a GLib event loop instead), so
 * without this a QMediaPlayer stays in LoadingMedia and never gives a frame.
 */
void runMainRunLoopIfNeeded(QApplication& app)
{
  if (QGuiApplication::platformName() == QLatin1String("cocoa"))
    return;

  QTimer* timer = new QTimer(&app);
  QObject::connect(timer, &QTimer::timeout, [] {
    // Handle what is pending, without waiting.
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
  });
  timer->start(MAIN_RUN_LOOP_INTERVAL_MS);
}

} // namespace
#endif

int main(int argc, char** argv)
{
  // QApplication: video decoding and OpenGL tests need a GUI platform, and
  // sources use QFileIconProvider (Qt Widgets) for their icons
  // (CI runs with QT_QPA_PLATFORM=offscreen).
  QApplication app(argc, argv);

#ifdef Q_OS_MACOS
  runMainRunLoopIfNeeded(app);
#endif

  int status = 0;
  {
    TestMaths test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestUtil test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestFrameStats test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestUidAllocator test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestShape test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestShapeGeometry test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestHandleModeKeys test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestOsc test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestFileVersion test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestVideo test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestCameraSource test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestMappingManager test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestProjectRoundTrip test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestSettings test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestSettingFormat test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestInspector test;
    status |= QTest::qExec(&test, argc, argv);
  }

  return status;
}
