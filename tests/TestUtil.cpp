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
