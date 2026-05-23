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

#include <QCoreApplication>
#include <QtTest/QtTest>

#include "TestMaths.h"
#include "TestUtil.h"
#include "TestUidAllocator.h"
#include "TestShape.h"

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

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
    TestUidAllocator test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    TestShape test;
    status |= QTest::qExec(&test, argc, argv);
  }

  return status;
}
