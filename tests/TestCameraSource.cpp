/*
 * TestCameraSource.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestCameraSource.h"

#include <QCameraDevice>
#include <QJsonObject>
#include <QMediaDevices>
#include <QScopedPointer>

#include "MetaObjectRegistry.h"
#include "ProjectLabels.h"
#include "Source.h"

using namespace mmp;

void TestCameraSource::initTestCase()
{
  // Same registrations as the application does at start-up (see main.cpp).
  MetaObjectRegistry& registry = MetaObjectRegistry::instance();
  registry.add<Video>();
  registry.add<Camera>();
}

void TestCameraSource::cameraIsSavedUnderItsOwnClassName()
{
  Camera camera;
  QJsonObject savedCamera;
  camera.write(savedCamera);
  QCOMPARE(savedCamera[ProjectLabels::CLASS_NAME].toString(), QString("Camera"));

  Video video;
  QJsonObject savedVideo;
  video.write(savedVideo);
  QCOMPARE(savedVideo[ProjectLabels::CLASS_NAME].toString(), QString("Video"));
}

void TestCameraSource::savedClassNameReloadsAsTheSameKindOfSource_data()
{
  QTest::addColumn<QString>("savedClassName");
  QTest::addColumn<bool>("expectedIsCamera");

  QTest::newRow("camera") << "Camera" << true;
  QTest::newRow("video file") << "Video" << false;
}

void TestCameraSource::savedClassNameReloadsAsTheSameKindOfSource()
{
  QFETCH(QString, savedClassName);
  QFETCH(bool, expectedIsCamera);

  // Same steps as ProjectReader::parseSource().
  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(
      Serializable::classNameCleanToReal(savedClassName));
  QVERIFY(metaObject != nullptr);

  QScopedPointer<QObject> instance(metaObject->newInstance(Q_ARG(int, NULL_UID)));
  Video* source = qobject_cast<Video*>(instance.data());
  QVERIFY(source != nullptr);
  QCOMPARE(source->isCamera(), expectedIsCamera);
  QCOMPARE(source->getUri(), QString(""));
}

void TestCameraSource::unknownUriIsNotACaptureDeviceId()
{
  QVERIFY(!Camera::isCaptureDeviceId(""));
  QVERIFY(!Camera::isCaptureDeviceId("/no/such/folder/movie.mov"));
}

void TestCameraSource::availableCameraIdIsACaptureDeviceId()
{
  const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
  if (cameras.isEmpty())
    QSKIP("No camera on this machine.");

  for (const QCameraDevice& camera : cameras)
    QVERIFY(Camera::isCaptureDeviceId(QString::fromUtf8(camera.id())));
}
