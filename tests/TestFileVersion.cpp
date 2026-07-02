/*
 * TestFileVersion.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestFileVersion.h"

#include <QRegularExpression>

#include "MM.h"

using namespace mmp;

namespace {
// Mirrors ProjectReader::isValidVersion(): a file version is accepted iff it
// matches MM::SUPPORTED_FILE_VERSIONS. Kept in sync with that method by design.
bool isValidVersion(const QString& version)
{
  return QRegularExpression(MM::SUPPORTED_FILE_VERSIONS).match(version).hasMatch();
}
}

void TestFileVersion::acceptsReleaseVersions()
{
  QVERIFY(isValidVersion("0.6.3"));   // previous stable format
  QVERIFY(isValidVersion("1.0.0"));
  QVERIFY(isValidVersion("2.10.15"));
}

void TestFileVersion::acceptsPreReleaseAndBuildSuffixes()
{
  // The 1.0 series ships pre-release builds; projects they save must reload.
  QVERIFY(isValidVersion("1.0.0-alpha.1"));
  QVERIFY(isValidVersion("1.0.0-rc.2"));
  QVERIFY(isValidVersion("1.0.0+build.5"));
}

void TestFileVersion::rejectsMalformedVersions()
{
  QVERIFY(!isValidVersion(""));
  QVERIFY(!isValidVersion("abc"));
  QVERIFY(!isValidVersion("1.0"));       // too few components
  QVERIFY(!isValidVersion("1.0.0.0"));   // too many components
  QVERIFY(!isValidVersion("1.0.0-"));    // empty suffix
  QVERIFY(!isValidVersion("v1.0.0"));    // stray prefix
  QVERIFY(!isValidVersion(" 1.0.0"));    // leading whitespace
}
