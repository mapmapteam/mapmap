/*
 * TestUtil.h
 *
 * Unit tests for the helpers in src/core/Util.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <QtTest/QtTest>

class TestUtil: public QObject
{
  Q_OBJECT

private slots:
  void mapFloat();
  void mapFloatClips();
  void mapInt();
  void isNumeric();
  void fileExistsAndErase();
};

#endif /* TEST_UTIL_H_ */
