/*
 * TestProjectRoundTrip.h
 *
 * Reads every project in tests/fixtures/, writes it, reads it back and writes
 * it again. Nothing in the fixture may be lost or changed along the way, and
 * the two writes must be identical. To cover a new case, drop a new .mmp file
 * in tests/fixtures/: no code to add.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_PROJECT_ROUND_TRIP_H_
#define TEST_PROJECT_ROUND_TRIP_H_

#include <QtTest/QtTest>

class TestProjectRoundTrip : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  void roundTrip_data();
  void roundTrip();

  void rejectsLegacyXml();
  void rejectsNonProjectJson();
  void rejectsInvalidJson();
  void keepsMediaUriWhenLocateIsCancelled();
  void skipsLayerWithoutSource();
  void skipsUnusableLayers();

private:
  QString _previousDir;
};

#endif /* TEST_PROJECT_ROUND_TRIP_H_ */
