/*
 * TestCameraSource.h
 *
 * Unit tests for the Camera source class: a camera must be saved under its
 * own class name so that a project reopens it with the capture back-end
 * instead of looking for a video file (issue #542).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_CAMERA_SOURCE_H_
#define TEST_CAMERA_SOURCE_H_

#include <QtTest/QtTest>

class TestCameraSource : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void cameraIsSavedUnderItsOwnClassName();
  void savedClassNameReloadsAsTheSameKindOfSource_data();
  void savedClassNameReloadsAsTheSameKindOfSource();
  void unknownUriIsNotACaptureDeviceId();
  void availableCameraIdIsACaptureDeviceId();
};

#endif /* TEST_CAMERA_SOURCE_H_ */
