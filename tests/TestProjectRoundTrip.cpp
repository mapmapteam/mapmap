/*
 * TestProjectRoundTrip.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestProjectRoundTrip.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

#include "MappingManager.h"
#include "MetaObjectRegistry.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"

using namespace mmp;

namespace {

QString fixturesDir()
{
  return QStringLiteral(MAPMAP_TEST_FIXTURES_DIR);
}

QByteArray readFixture(const QString& path)
{
  QFile file(path);
  if (!file.open(QFile::ReadOnly))
    return QByteArray();
  return file.readAll();
}

bool readProject(MappingManager& manager, const QByteArray& data, QString* errorString = nullptr)
{
  QBuffer buffer;
  buffer.setData(data);
  buffer.open(QIODevice::ReadOnly);
  ProjectReader reader(manager);
  bool ok = reader.readFile(&buffer);
  if (errorString)
    *errorString = reader.errorString();
  return ok;
}

QByteArray writeProject(MappingManager& manager)
{
  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);
  ProjectWriter writer(manager);
  writer.writeFile(&buffer);
  return buffer.data();
}

/// Returns the JSON text of a value, shortened to keep failure messages readable.
QString jsonText(const QJsonValue& value)
{
  const int maxLength = 80;
  QString text;
  if (value.isObject())
    text = QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
  else if (value.isArray())
    text = QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
  else // Wrap scalars in an array to get their JSON text, then strip the brackets.
    text = QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
  return text.size() > maxLength ? text.left(maxLength) + "..." : text;
}

/// Appends to diffs every value of expected that is missing or different in
/// actual, named by its path (eg. "layers[0].destination.vertices").
/// Keys only present in actual are ignored: new properties with default values
/// are allowed to appear when an older project is saved again.
void collectLostValues(const QJsonValue& expected, const QJsonValue& actual,
                       const QString& path, QStringList& diffs)
{
  if (expected.isObject() && actual.isObject())
  {
    const QJsonObject expectedObj = expected.toObject();
    const QJsonObject actualObj   = actual.toObject();
    for (auto it = expectedObj.constBegin(); it != expectedObj.constEnd(); ++it)
    {
      QString childPath = path.isEmpty() ? it.key() : path + "." + it.key();
      if (!actualObj.contains(it.key()))
        diffs << QString("%1: lost (was %2)").arg(childPath, jsonText(it.value()));
      else
        collectLostValues(it.value(), actualObj.value(it.key()), childPath, diffs);
    }
  }
  else if (expected.isArray() && actual.isArray())
  {
    const QJsonArray expectedArr = expected.toArray();
    const QJsonArray actualArr   = actual.toArray();
    if (expectedArr.size() != actualArr.size())
    {
      diffs << QString("%1: %2 elements instead of %3")
               .arg(path).arg(actualArr.size()).arg(expectedArr.size());
      return;
    }
    for (int i = 0; i < expectedArr.size(); i++)
      collectLostValues(expectedArr[i], actualArr[i], QString("%1[%2]").arg(path).arg(i), diffs);
  }
  else if (expected != actual)
  {
    diffs << QString("%1: %2 instead of %3").arg(path, jsonText(actual), jsonText(expected));
  }
}

QJsonObject parse(const QByteArray& data)
{
  return QJsonDocument::fromJson(data).object();
}

} // namespace

void TestProjectRoundTrip::initTestCase()
{
  MetaObjectRegistry::instance().addBuiltinTypes();

  // Media paths in the fixtures are relative to the fixtures directory.
  _previousDir = QDir::currentPath();
  QVERIFY(QDir::setCurrent(fixturesDir()));
}

void TestProjectRoundTrip::cleanupTestCase()
{
  QDir::setCurrent(_previousDir);
}

void TestProjectRoundTrip::roundTrip_data()
{
  QTest::addColumn<QString>("path");

  QDir dir(fixturesDir());
  const QStringList files = dir.entryList(QStringList() << "*.mmp", QDir::Files, QDir::Name);
  QVERIFY2(!files.isEmpty(), qPrintable("No fixture found in " + dir.absolutePath()));
  for (const QString& file : files)
    QTest::newRow(qPrintable(file)) << dir.filePath(file);
}

void TestProjectRoundTrip::roundTrip()
{
  QFETCH(QString, path);

  const QByteArray original = readFixture(path);
  QVERIFY2(!original.isEmpty(), qPrintable("Cannot read " + path));
  const QJsonObject originalObj = parse(original);

  // First read.
  MappingManager manager;
  QString error;
  QVERIFY2(readProject(manager, original, &error), qPrintable(error));
  QCOMPARE(manager.nSources(), originalObj["sources"].toArray().size());
  QCOMPARE(manager.nLayers(),  originalObj["layers"].toArray().size());

  // First write: everything in the fixture must still be there (except the
  // version number, which becomes the current one).
  const QByteArray firstWrite = writeProject(manager);
  QJsonObject expected = originalObj;
  expected.remove("version");
  QStringList diffs;
  collectLostValues(expected, parse(firstWrite), QString(), diffs);
  QVERIFY2(diffs.isEmpty(), qPrintable("Lost after one save:\n  " + diffs.join("\n  ")));

  // Second read and write: the two writes must be identical.
  manager.clearAll();
  QVERIFY2(readProject(manager, firstWrite, &error), qPrintable(error));
  const QByteArray secondWrite = writeProject(manager);
  if (firstWrite != secondWrite)
  {
    diffs.clear();
    collectLostValues(parse(firstWrite), parse(secondWrite), QString(), diffs);
    collectLostValues(parse(secondWrite), parse(firstWrite), QString(), diffs);
    QFAIL(qPrintable("Second save differs from the first:\n  " + diffs.join("\n  ")));
  }
}

void TestProjectRoundTrip::rejectsLegacyXml()
{
  // Issue 604: projects from MapMap 0.6 and earlier are XML.
  MappingManager manager;
  QString error;
  QVERIFY(!readProject(manager, readFixture(fixturesDir() + "/invalid/legacy-0.6.3.xml.mmp"), &error));
  QVERIFY2(error.contains("XML"), qPrintable(error));
  QVERIFY2(error.contains("0.6.3"), qPrintable(error));
  QCOMPARE(manager.nSources(), 0);
  QCOMPARE(manager.nLayers(), 0);
}

void TestProjectRoundTrip::rejectsNonProjectJson()
{
  MappingManager manager;
  QString error;
  QVERIFY(!readProject(manager, "{\"hello\": \"world\"}", &error));
  QVERIFY(!error.isEmpty());

  QVERIFY(!readProject(manager, "{\"version\": \"0.6\", \"sources\": [], \"layers\": []}", &error));
  QVERIFY2(error.contains("0.6"), qPrintable(error));
}

void TestProjectRoundTrip::rejectsInvalidJson()
{
  MappingManager manager;
  QString error;
  QVERIFY(!readProject(manager, "{\"version\": \"1.0.0\", ", &error));
  QVERIFY(!error.isEmpty());
  QVERIFY(!error.contains("XML"));
}

void TestProjectRoundTrip::keepsMediaUriWhenLocateIsCancelled()
{
  // When a media file is missing, MapMap asks the user to locate it. If the
  // user cancels, the original path must be kept so that saving the project
  // does not lose it.
  const QStringList fixtures = QStringList() << "quad-missing-video.mmp" << "quad-image.mmp";
  for (const QString& fixture : fixtures)
  {
    const QByteArray original = readFixture(fixturesDir() + "/" + fixture);
    const QString uri = parse(original)["sources"].toArray()[0].toObject()["uri"].toString();

    QBuffer buffer;
    buffer.setData(original);
    buffer.open(QIODevice::ReadOnly);
    MappingManager manager;
    int nCalls = 0;
    ProjectReader reader(manager, [&nCalls](const QString&, bool) {
      nCalls++;
      return QString(); // Cancelled.
    });
    QVERIFY2(reader.readFile(&buffer), qPrintable(reader.errorString()));
    QCOMPARE(nCalls, 1);

    const QJsonObject written = parse(writeProject(manager));
    QCOMPARE(written["sources"].toArray()[0].toObject()["uri"].toString(), uri);
  }
}

void TestProjectRoundTrip::skipsLayerWithoutSource()
{
  // A layer whose source is missing (eg. a macOS-only Syphon source skipped
  // on another platform) is dropped; the rest of the project still loads.
  const QByteArray project = R"({
    "version": "1.0.0-alpha.1",
    "sources": [
      { "className": "Color", "id": 1, "name": "red", "color": "#ff0000" },
      { "className": "UnknownSource", "id": 2, "name": "elsewhere" }
    ],
    "layers": [
      { "className": "ColorLayer", "id": 1, "name": "kept", "sourceId": 1,
        "destination": { "className": "Quad", "vertices": [[0,0],[1,0],[1,1],[0,1]] } },
      { "className": "ColorLayer", "id": 2, "name": "dropped", "sourceId": 2,
        "destination": { "className": "Quad", "vertices": [[0,0],[1,0],[1,1],[0,1]] } }
    ]
  })";

  MappingManager manager;
  QString error;
  QVERIFY2(readProject(manager, project, &error), qPrintable(error));
  QCOMPARE(manager.nSources(), 1);
  QCOMPARE(manager.nLayers(), 1);
  QCOMPARE(manager.getLayer(0)->getName(), QString("kept"));
}
