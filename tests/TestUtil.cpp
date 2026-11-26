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

void TestUtil::createShapesForRect()
{
  const QRectF rect(10, 20, 100, 50);

  // A quad on the corners, clockwise from the top left.
  QScopedPointer<Mesh> mesh(Util::createMeshForRect(rect));
  QCOMPARE(mesh->nVertices(), 4);
  QCOMPARE(mesh->toPolygon().boundingRect(), rect);
  QCOMPARE(mesh->getVertex2d(0, 0), rect.topLeft());
  QCOMPARE(mesh->getVertex2d(1, 1), rect.bottomRight());

  // A triangle standing on the bottom edge.
  QScopedPointer<Triangle> triangle(Util::createTriangleForRect(rect));
  QCOMPARE(triangle->getVertex(0), QPointF(10, 70));
  QCOMPARE(triangle->getVertex(1), QPointF(110, 70));
  QCOMPARE(triangle->getVertex(2), QPointF(60, 20));

  // An ellipse touching the middle of each edge.
  // Qualified: Windows also has an Ellipse() function.
  QScopedPointer<mmp::Ellipse> ellipse(Util::createEllipseForRect(rect));
  QCOMPARE(ellipse->getVertex(0), QPointF(10, 45));
  QCOMPARE(ellipse->getVertex(1), QPointF(60, 20));
  QCOMPARE(ellipse->getVertex(2), QPointF(110, 45));
  QCOMPARE(ellipse->getVertex(3), QPointF(60, 70));
  // Its center control point comes with it.
  QCOMPARE(ellipse->nVertices(), mmp::Ellipse::N_DEFINING_VERTICES + 1);
  QCOMPARE(ellipse->getVertex(mmp::Ellipse::CENTER_VERTEX), QPointF(60, 45));
}

void TestUtil::mapRectToRect()
{
  // Corners go to corners, the middle to the middle.
  const QVector<QPointF> points = { QPointF(0, 0), QPointF(640, 480), QPointF(320, 240) };
  const QVector<QPointF> mapped = Util::mapRectToRect(points, QRectF(0, 0, 640, 480), QRectF(100, 50, 1920, 1080));
  QCOMPARE(mapped, QVector<QPointF>({ QPointF(100, 50), QPointF(2020, 1130), QPointF(1060, 590) }));

  // An empty source rectangle cannot be mapped: points are left as they are.
  QCOMPARE(Util::mapRectToRect(points, QRectF(0, 0, 0, 480), QRectF(0, 0, 10, 10)), points);
}
