/*
 * TestFileVersion.h
 *
 * Unit tests for the project-file version compatibility contract
 * (MM::SUPPORTED_FILE_VERSIONS), which ProjectReader uses to decide whether a
 * .mmp file is loadable. Regression guard for the 1.0.0-alpha.1 version bump:
 * a saved pre-release project must remain loadable.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_FILE_VERSION_H_
#define TEST_FILE_VERSION_H_

#include <QtTest/QtTest>

class TestFileVersion : public QObject
{
  Q_OBJECT

private slots:
  void acceptsReleaseVersions();
  void acceptsPreReleaseAndBuildSuffixes();
  void rejectsMalformedVersions();
};

#endif /* TEST_FILE_VERSION_H_ */
