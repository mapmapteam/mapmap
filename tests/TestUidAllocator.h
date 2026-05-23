/*
 * TestUidAllocator.h
 *
 * Unit tests for src/core/UidAllocator.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_UID_ALLOCATOR_H_
#define TEST_UID_ALLOCATOR_H_

#include <QtTest/QtTest>

class TestUidAllocator: public QObject
{
  Q_OBJECT

private slots:
  void allocateIsSequential();
  void freeReusesLowestId();
  void reserve();
  void freeUnknownReturnsFalse();
};

#endif /* TEST_UID_ALLOCATOR_H_ */
