/*
 * TestVideo.h
 *
 * Tests for the video/camera frame pipeline: decoding through VideoPlayerImpl,
 * frame format and colours, new-frame flags, and the OpenGL texture upload.
 *
 * Test clips are generated at runtime with the ffmpeg CLI (tests are skipped
 * if it is missing). OpenGL and camera tests are skipped when unavailable.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_VIDEO_H_
#define TEST_VIDEO_H_

#include <QtTest/QtTest>
#include <QTemporaryDir>

class TestVideo: public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void decodeFrame_data();
  void decodeFrame();

  void videoBitsChangedFlag();

  void glUpload_data();
  void glUpload();

  void yuvShaderDraw_data();
  void yuvShaderDraw();

  void decodedYuvShaderDraw();

  void cameraFrames();

  void preparationIsTimedWhenEnabled();
  void preparationIsNotTimedWhenDisabled();
  void replacedFramesAreDropped();
  void uploadIsTimedWhenEnabled();

private:
  // Generates a quadrant test clip with the given pixel format.
  QString _makeClip(const QString& pixelFormat);

  QTemporaryDir _tmpDir;
  QString _ffmpeg;
  QMap<QString, QString> _clips; // pixel format -> path
};

#endif /* TEST_VIDEO_H_ */
