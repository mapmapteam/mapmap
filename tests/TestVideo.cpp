/*
 * TestVideo.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestVideo.h"

#include <QMediaDevices>
#include <QCameraDevice>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QProcess>
#include <QStandardPaths>

#include "VideoPlayerImpl.h"
#include "CameraImpl.h"
#include "Util.h"

using namespace mmp;

namespace {

// Test clip layout: four solid quadrants.
//   red   | green
//   ------+------
//   blue  | white
const int QUADRANT_SIZE = 64;
const int CLIP_SIZE     = 2 * QUADRANT_SIZE;

// Colour tolerance after the YUV round trip.
const int TOLERANCE = 24;

// Time allowed for media to deliver frames.
const int FRAME_TIMEOUT_MS = 5000;

struct Quadrant {
  const char* name;
  int x, y; // centre pixel
  QColor color;
};

const Quadrant QUADRANTS[] = {
  { "top-left (red)",      QUADRANT_SIZE/2,                 QUADRANT_SIZE/2,                 QColor(255,   0,   0) },
  { "top-right (green)",   QUADRANT_SIZE + QUADRANT_SIZE/2, QUADRANT_SIZE/2,                 QColor(  0, 255,   0) },
  { "bottom-left (blue)",  QUADRANT_SIZE/2,                 QUADRANT_SIZE + QUADRANT_SIZE/2, QColor(  0,   0, 255) },
  { "bottom-right (white)",QUADRANT_SIZE + QUADRANT_SIZE/2, QUADRANT_SIZE + QUADRANT_SIZE/2, QColor(255, 255, 255) },
};

bool _colorsClose(const QColor& a, const QColor& b)
{
  return qAbs(a.red()   - b.red())   <= TOLERANCE &&
         qAbs(a.green() - b.green()) <= TOLERANCE &&
         qAbs(a.blue()  - b.blue())  <= TOLERANCE;
}

QString _colorString(const QColor& c)
{
  return QString("(%1, %2, %3)").arg(c.red()).arg(c.green()).arg(c.blue());
}

// Checks the four quadrants given a pixel accessor.
template <typename PixelAt>
void _checkQuadrants(PixelAt pixelAt, const char* where)
{
  for (const Quadrant& q : QUADRANTS)
  {
    QColor got = pixelAt(q.x, q.y);
    QVERIFY2(_colorsClose(got, q.color),
             qPrintable(QString("%1: %2 is %3, expected %4")
                        .arg(where, q.name, _colorString(got), _colorString(q.color))));
  }
}

} // namespace

QString TestVideo::_makeClip(const QString& pixelFormat)
{
  QString path = _tmpDir.filePath(QString("quadrants-%1.mp4").arg(pixelFormat));
  QString src = QString("color=%2:s=%1x%1:d=1:r=15").arg(QUADRANT_SIZE);
  QStringList args = {
    "-y", "-hide_banner", "-loglevel", "error",
    "-f", "lavfi", "-i", src.arg("red"),
    "-f", "lavfi", "-i", src.arg("lime"),
    "-f", "lavfi", "-i", src.arg("blue"),
    "-f", "lavfi", "-i", src.arg("white"),
    "-filter_complex", "[0][1]hstack[t];[2][3]hstack[b];[t][b]vstack",
    "-c:v", "libx264", "-pix_fmt", pixelFormat,
    path
  };
  QProcess proc;
  proc.start(_ffmpeg, args);
  if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
  {
    qWarning() << "ffmpeg failed:" << proc.readAllStandardError();
    return QString();
  }
  return path;
}

void TestVideo::initTestCase()
{
  QVERIFY(_tmpDir.isValid());
  _ffmpeg = QStandardPaths::findExecutable("ffmpeg");
  if (_ffmpeg.isEmpty())
    return; // Video tests will skip.

  for (const QString& fmt : { QString("yuv420p"), QString("yuv444p") })
  {
    QString path = _makeClip(fmt);
    if (!path.isEmpty())
      _clips[fmt] = path;
  }
}

void TestVideo::decodeFrame_data()
{
  QTest::addColumn<QString>("pixelFormat");
  QTest::newRow("yuv420p") << "yuv420p";
  QTest::newRow("yuv444p") << "yuv444p";
}

void TestVideo::decodeFrame()
{
  QFETCH(QString, pixelFormat);
  if (!_clips.contains(pixelFormat))
    QSKIP("ffmpeg (with libx264) not available to generate test clips");

  VideoPlayerImpl player;
  QVERIFY(player.loadMovie(_clips[pixelFormat]));
  QTRY_VERIFY_WITH_TIMEOUT(player.hasBits(), FRAME_TIMEOUT_MS);

  QCOMPARE(player.getWidth(),  CLIP_SIZE);
  QCOMPARE(player.getHeight(), CLIP_SIZE);

  // Opaque video must come out in a directly uploadable opaque format, with
  // no per-pixel conversion pass (Qt gives RGBA8888_Premultiplied for YUV
  // video, which frameToImage() just relabels as RGBX8888).
  QImage frame = player.getFrameImage();
  QVERIFY2(frame.format() == QImage::Format_RGB32 || frame.format() == QImage::Format_RGBX8888,
           qPrintable(QString("unexpected format %1").arg(frame.format())));

  // Upright, correct colours.
  _checkQuadrants([&](int x, int y) { return QColor(frame.pixel(x, y)); }, "decoded frame");
  if (QTest::currentTestFailed())
    return;
}

void TestVideo::videoBitsChangedFlag()
{
  if (!_clips.contains("yuv420p"))
    QSKIP("ffmpeg (with libx264) not available to generate test clips");

  VideoPlayerImpl player;
  QVERIFY(player.loadMovie(_clips["yuv420p"]));
  QTRY_VERIFY_WITH_TIMEOUT(player.hasBits() && player.bitsHaveChanged(), FRAME_TIMEOUT_MS);

  // Reading consumes the frame...
  QVERIFY(player.getBits() != nullptr);
  QVERIFY(!player.bitsHaveChanged());

  // ...but reading the frame image (thumbnail path) does not.
  QTRY_VERIFY_WITH_TIMEOUT(player.bitsHaveChanged(), FRAME_TIMEOUT_MS); // next frame (15 fps)
  QVERIFY(!player.getFrameImage().isNull());
  QVERIFY(player.bitsHaveChanged());
}

void TestVideo::glUpload_data()
{
  decodeFrame_data();
}

void TestVideo::glUpload()
{
  QFETCH(QString, pixelFormat);
  if (!_clips.contains(pixelFormat))
    QSKIP("ffmpeg (with libx264) not available to generate test clips");

  // Offscreen desktop-GL context.
  QOffscreenSurface surface;
  surface.create();
  QOpenGLContext context;
  if (!context.create() || !context.makeCurrent(&surface))
    QSKIP("No OpenGL context available");
  if (context.isOpenGLES())
    QSKIP("Test needs desktop OpenGL (glGetTexImage)");

  // Decode one frame.
  VideoPlayerImpl player;
  QVERIFY(player.loadMovie(_clips[pixelFormat]));
  QTRY_VERIFY_WITH_TIMEOUT(player.hasBits(), FRAME_TIMEOUT_MS);
  QImage frame = player.getFrameImage().copy(); // detach from the player
  player.setPlayState(false);

  // Make sure the decode wait didn't leave another context current.
  QVERIFY(context.makeCurrent(&surface));

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  // Same format/type choice as Video::getBitsFormat()/getBitsType().
  GLenum format, type;
  int allocWidth = 0, allocHeight = 0;

  auto readBack = [&](int w, int h) {
    QByteArray rgba(w * h * 4, 0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return rgba;
  };

  // 1. First upload allocates storage.
  Util::glUploadFormat(frame.format(), format, type);
  Util::uploadTextureBits(frame.width(), frame.height(), format, type,
                          frame.constBits(), allocWidth, allocHeight);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
  QCOMPARE(allocWidth,  CLIP_SIZE);
  QCOMPARE(allocHeight, CLIP_SIZE);

  QByteArray rgba = readBack(CLIP_SIZE, CLIP_SIZE);
  auto texelAt = [&](int x, int y) {
    const uchar* p = reinterpret_cast<const uchar*>(rgba.constData()) + (y * CLIP_SIZE + x) * 4;
    return QColor(p[0], p[1], p[2]);
  };
  _checkQuadrants(texelAt, "uploaded texture");
  if (QTest::currentTestFailed())
    return;

  // 2. Same-size upload updates in place (glTexSubImage2D path).
  QImage green(frame.size(), QImage::Format_RGB32);
  green.fill(QColor(0, 255, 0));
  Util::glUploadFormat(green.format(), format, type); // BGRA path
  Util::uploadTextureBits(green.width(), green.height(), format, type,
                          green.constBits(), allocWidth, allocHeight);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
  QCOMPARE(allocWidth, CLIP_SIZE);
  rgba = readBack(CLIP_SIZE, CLIP_SIZE);
  QVERIFY2(_colorsClose(texelAt(10, 10), QColor(0, 255, 0)),
           qPrintable("in-place update: got " + _colorString(texelAt(10, 10))));

  // 3. A size change reallocates.
  QImage small(32, 16, QImage::Format_RGBX8888);
  small.fill(QColor(255, 0, 0));
  Util::glUploadFormat(small.format(), format, type); // RGBA path
  Util::uploadTextureBits(small.width(), small.height(), format, type,
                          small.constBits(), allocWidth, allocHeight);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
  QCOMPARE(allocWidth,  32);
  QCOMPARE(allocHeight, 16);
  GLint texWidth = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
  QCOMPARE(texWidth, 32);
  {
    QByteArray smallRgba(32 * 16 * 4, 0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, smallRgba.data());
    const uchar* p = reinterpret_cast<const uchar*>(smallRgba.constData());
    QVERIFY2(_colorsClose(QColor(p[0], p[1], p[2]), QColor(255, 0, 0)),
             qPrintable("RGBA path: got " + _colorString(QColor(p[0], p[1], p[2]))));
  }

  glDeleteTextures(1, &tex);
  context.doneCurrent();
}

void TestVideo::cameraFrames()
{
  const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
  if (cameras.isEmpty())
    QSKIP("No camera available");

  CameraImpl camera;
  if (!camera.loadMovie(QString::fromUtf8(cameras.first().id())))
    QSKIP("Camera could not be started");

  QTRY_VERIFY_WITH_TIMEOUT(camera.hasBits() && camera.bitsHaveChanged(), FRAME_TIMEOUT_MS);

  // Frame is ready for BGRA upload.
  QImage frame = camera.getFrameImage();
  QVERIFY(!frame.isNull());
  QVERIFY2(frame.format() == QImage::Format_RGB32  || frame.format() == QImage::Format_ARGB32 ||
           frame.format() == QImage::Format_RGBX8888 || frame.format() == QImage::Format_RGBA8888,
           qPrintable(QString("unexpected format %1").arg(frame.format())));
  QCOMPARE(camera.getWidth(),  frame.width());
  QCOMPARE(camera.getHeight(), frame.height());

  // Reading consumes the frame; no re-upload until a new one arrives
  // (this used to always report true).
  QVERIFY(camera.getBits() != nullptr);
  QVERIFY(!camera.bitsHaveChanged());
  QTRY_VERIFY_WITH_TIMEOUT(camera.bitsHaveChanged(), FRAME_TIMEOUT_MS);
}
