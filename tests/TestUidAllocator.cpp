/*
 * TestUidAllocator.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestUidAllocator.h"

#include "UidAllocator.h"

using namespace mmp;

void TestUidAllocator::allocateIsSequential()
{
  UidAllocator allocator;
  QCOMPARE(allocator.allocate(), uid(1));
  QCOMPARE(allocator.allocate(), uid(2));
  QCOMPARE(allocator.allocate(), uid(3));

  QVERIFY(allocator.exists(1));
  QVERIFY(allocator.exists(2));
  QVERIFY(allocator.exists(3));
  QVERIFY(!allocator.exists(4));
  QCOMPARE(int(allocator.list().size()), 3);
}

void TestUidAllocator::freeReusesLowestId()
{
  UidAllocator allocator;
  allocator.allocate(); // 1
  allocator.allocate(); // 2
  allocator.allocate(); // 3

  QVERIFY(allocator.free(2));
  QVERIFY(!allocator.exists(2));

  // The next allocation should reuse the lowest free id (2).
  QCOMPARE(allocator.allocate(), uid(2));
  QVERIFY(allocator.exists(2));
}

void TestUidAllocator::reserve()
{
  UidAllocator allocator;

  // Reserving an unused id succeeds and marks it as existing.
  QVERIFY(allocator.reserve(42));
  QVERIFY(allocator.exists(42));

  // Reserving an already-reserved id fails.
  QVERIFY(!allocator.reserve(42));

  // A subsequent allocate must skip the reserved id.
  QCOMPARE(allocator.allocate(), uid(1));
}

void TestUidAllocator::freeUnknownReturnsFalse()
{
  UidAllocator allocator;
  QVERIFY(!allocator.free(99));

  allocator.allocate(); // 1
  QVERIFY(!allocator.free(99));
  QVERIFY(allocator.free(1));
  QVERIFY(!allocator.free(1)); // already freed
}
