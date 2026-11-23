/*
 * TestUtil.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestUtil.h"

#include "Util.h"
#include "Shapes.h"

#include <QTemporaryDir>
#include <QFile>

using namespace mmp;

void TestUtil::mapFloat()
{
  QVERIFY(qFuzzyCompare(Util::map_float(0.0f, 0.0f, 127.0f, 0.0f, 1.0f), 0.0f));
  QVERIFY(qFuzzyCompare(Util::map_float(127.0f, 0.0f, 127.0f, 0.0f, 1.0f), 1.0f));
  QVERIFY(qFuzzyCompare(Util::map_float(64.0f, 0.0f, 127.0f, 0.0f, 1.0f), 64.0f / 127.0f));
}

void TestUtil::mapFloatClips()
{
  // Values outside the input range are clipped to [ostart, ostop].
  QVERIFY(qFuzzyCompare(Util::map_float(200.0f, 0.0f, 127.0f, 0.0f, 1.0f), 1.0f));
  QVERIFY(qFuzzyCompare(Util::map_float(-10.0f, 0.0f, 127.0f, 0.0f, 1.0f) + 1.0f, 1.0f)); // == 0
}

void TestUtil::mapInt()
{
  QCOMPARE(Util::map_int(0, 0, 127, 0, 127), 0);
  QCOMPARE(Util::map_int(127, 0, 127, 0, 127), 127);
  QCOMPARE(Util::map_int(64, 0, 127, 0, 127), 64);
  // Clipping.
  QCOMPARE(Util::map_int(500, 0, 127, 0, 127), 127);
  QCOMPARE(Util::map_int(-20, 0, 127, 0, 127), 0);
}

void TestUtil::isNumeric()
{
  QVERIFY(Util::isNumeric("123"));
  QVERIFY(Util::isNumeric("007"));
  QVERIFY(!Util::isNumeric("12a"));
  QVERIFY(!Util::isNumeric("1.5"));
  QVERIFY(!Util::isNumeric("-5"));

  // Documents current behaviour: the regex "^\\d*$" also matches the empty
  // string. If this is ever tightened, this expectation should change.
  QVERIFY(Util::isNumeric(""));
}

void TestUtil::fileExistsAndErase()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString path = dir.filePath("sample.txt");

  // Erasing / probing a non-existent file.
  QVERIFY(!Util::fileExists(path));
  QVERIFY(!Util::eraseFile(path));

  // Create the file then verify detection and removal.
  {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hello");
    file.close();
  }

  QVERIFY(Util::fileExists(path));
  QVERIFY(Util::eraseFile(path));
  QVERIFY(!Util::fileExists(path));
}

void TestUtil::placeLargeShapeInArea()
{
  // Wider than the area: as wide as it, with the same proportions.
  Triangle triangle(QPointF(100, 2100), QPointF(4100, 2100), QPointF(2100, 100));
  Util::placeShapeInArea(&triangle, QRectF(0, 0, 1920, 1080));
  QCOMPARE(triangle.getBoundingRect(), QRectF(0, 60, 1920, 960));
  QCOMPARE(triangle.getVertex(2), QPointF(960, 60));

  // Higher than the area.
  mmp::Ellipse ellipse(QPointF(0, 2000), QPointF(1000, 0), QPointF(2000, 2000), QPointF(1000, 4000), true);
  Util::placeShapeInArea(&ellipse, QRectF(0, 0, 1920, 1080));
  QCOMPARE(ellipse.getBoundingRect(), QRectF(690, 0, 540, 1080));
  QCOMPARE(ellipse.getVertex(4), QPointF(960, 540)); // center
}

void TestUtil::placeSmallShapeInArea()
{
  // Moved, with the same size.
  Mesh mesh(QPointF(-640, -480), QPointF(0, -480), QPointF(0, 0), QPointF(-640, 0));
  Util::placeShapeInArea(&mesh, QRectF(0, 0, 1920, 1080));
  QCOMPARE(mesh.getBoundingRect(), QRectF(640, 300, 640, 480));
  QCOMPARE(mesh.getVertex(0), QPointF(640, 300));
}

void TestUtil::placeShapeInEmptyArea()
{
  Triangle triangle(QPointF(0, 10), QPointF(10, 10), QPointF(5, 0));
  Util::placeShapeInArea(&triangle, QRectF());
  QCOMPARE(triangle.getBoundingRect(), QRectF(0, 0, 10, 10));
}

void TestUtil::fitScale()
{
  // The content fills the area along its tighter side.
  QCOMPARE(Util::fitScale(QSizeF(1920, 1080), QSizeF(960, 1000)), 0.5);
  QCOMPARE(Util::fitScale(QSizeF(1920, 1080), QSizeF(3000, 540)), 0.5);
  QCOMPARE(Util::fitScale(QSizeF(100, 100), QSizeF(300, 400)), 3.0);

  // Nothing to fit, or nowhere to fit it: no scaling.
  QCOMPARE(Util::fitScale(QSizeF(0, 1080), QSizeF(960, 540)), 1.0);
  QCOMPARE(Util::fitScale(QSizeF(1920, 1080), QSizeF(0, 0)), 1.0);
  QCOMPARE(Util::fitScale(QSizeF(-5, 10), QSizeF(960, 540)), 1.0);
}

void TestUtil::fitRect()
{
  // A 4:3 image in a 16:9 frame: as high as the frame, centered horizontally.
  QCOMPARE(Util::fitRect(QSizeF(640, 480), QRectF(0, 0, 1920, 1080)), QRectF(240, 0, 1440, 1080));

  // A wide image in a square frame, which does not start at the origin.
  QCOMPARE(Util::fitRect(QSizeF(200, 100), QRectF(10, 20, 100, 100)), QRectF(10, 45, 100, 50));

  // An empty content fills the frame.
  QCOMPARE(Util::fitRect(QSizeF(), QRectF(0, 0, 1280, 720)), QRectF(0, 0, 1280, 720));
}
