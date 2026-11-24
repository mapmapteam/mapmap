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
#include <QOpenGLFramebufferObject>

#include "FrameStats.h"
#include "Source.h"
#include "VideoPlayerImpl.h"
#include "CameraImpl.h"
#include "Util.h"
#include "YuvTexture.h"

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

// Builds a YUV frame of the quadrant pattern in the given layout, encoding
// with the BT.601 matrix in the given range (rows padded to test strides).
QVideoFrame _makeYuvFrame(QVideoFrameFormat::PixelFormat pixelFormat, QVideoFrameFormat::ColorRange range)
{
  QVideoFrameFormat format(QSize(CLIP_SIZE, CLIP_SIZE), pixelFormat);
  format.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);
  format.setColorRange(range);
  QVideoFrame frame(format);
  if (!frame.map(QVideoFrame::WriteOnly))
    return QVideoFrame();

  const bool full = (range == QVideoFrameFormat::ColorRange_Full);
  auto yuvOf = [&](const QColor& c, float& y, float& u, float& v) {
    const float r = c.redF(), g = c.greenF(), b = c.blueF();
    const float yy = 0.299f * r + 0.587f * g + 0.114f * b;
    const float cb = (b - yy) / 1.772f, cr = (r - yy) / 1.402f;
    y = full ? yy * 255 : 16 + yy * 219;
    u = (full ? 255 : 224) * cb + 128;
    v = (full ? 255 : 224) * cr + 128;
  };
  auto toByte = [](float f) { return uchar(qBound(0, qRound(f), 255)); };
  auto colorAt = [](int x, int y) {
    const int q = (y >= QUADRANT_SIZE ? 2 : 0) + (x >= QUADRANT_SIZE ? 1 : 0);
    return QUADRANTS[q].color;
  };

  for (int y = 0; y < CLIP_SIZE; y++)
    for (int x = 0; x < CLIP_SIZE; x++) {
      float yy, u, v;
      yuvOf(colorAt(x, y), yy, u, v);
      frame.bits(0)[y * frame.bytesPerLine(0) + x] = toByte(yy);
      if (x % 2 || y % 2)
        continue;
      const int cx = x / 2, cy = y / 2;
      switch (pixelFormat) {
      case QVideoFrameFormat::Format_NV12:
      case QVideoFrameFormat::Format_NV21: {
        uchar* p = frame.bits(1) + cy * frame.bytesPerLine(1) + cx * 2;
        const bool nv12 = (pixelFormat == QVideoFrameFormat::Format_NV12);
        p[0] = toByte(nv12 ? u : v);
        p[1] = toByte(nv12 ? v : u);
        break;
      }
      default: {
        const bool yv12 = (pixelFormat == QVideoFrameFormat::Format_YV12);
        frame.bits(yv12 ? 2 : 1)[cy * frame.bytesPerLine(1) + cx] = toByte(u);
        frame.bits(yv12 ? 1 : 2)[cy * frame.bytesPerLine(2) + cx] = toByte(v);
        break;
      }
      }
    }
  frame.unmap();
  return frame;
}

// Draws a YuvTexture on a full-size quad into an FBO (with the fixed-function
// pipeline, like MapMap's renderer) and returns the result.
QImage _drawYuv(YuvTexture& yuv)
{
  QOpenGLFramebufferObject fbo(CLIP_SIZE, CLIP_SIZE);
  fbo.bind();
  glViewport(0, 0, CLIP_SIZE, CLIP_SIZE);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, CLIP_SIZE, CLIP_SIZE, 0, -1, 1); // y down, like the canvases
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);
  glColor4f(1, 1, 1, 1);
  yuv.bind();
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(0, 0);
  glTexCoord2f(1, 0); glVertex2f(CLIP_SIZE, 0);
  glTexCoord2f(1, 1); glVertex2f(CLIP_SIZE, CLIP_SIZE);
  glTexCoord2f(0, 1); glVertex2f(0, CLIP_SIZE);
  glEnd();
  yuv.release();
  glDisable(GL_TEXTURE_2D);

  QImage img = fbo.toImage();
  fbo.release();
  return img;
}

/// Video implementation fed by hand, to test what VideoImpl does with frames.
class FrameFeeder : public VideoImpl
{
public:
  bool isLive() override { return true; }
  bool seekTo(qint64) override { return false; }
  void feed(const QVideoFrame& frame) { _setFrame(frame); }
};

/// Turns FrameStats on for the life of the object.
struct FrameStatsOn
{
  FrameStatsOn() { FrameStats::setEnabled(true); }
  ~FrameStatsOn() { FrameStats::setEnabled(false); }
};

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

void TestVideo::yuvShaderDraw_data()
{
  QTest::addColumn<int>("pixelFormat");
  QTest::addColumn<int>("range");
  const int video = QVideoFrameFormat::ColorRange_Video, full = QVideoFrameFormat::ColorRange_Full;
  QTest::newRow("yuv420p video") << int(QVideoFrameFormat::Format_YUV420P) << video;
  QTest::newRow("yuv420p full")  << int(QVideoFrameFormat::Format_YUV420P) << full;
  QTest::newRow("yv12")          << int(QVideoFrameFormat::Format_YV12)    << video;
  QTest::newRow("nv12")          << int(QVideoFrameFormat::Format_NV12)    << video;
  QTest::newRow("nv21")          << int(QVideoFrameFormat::Format_NV21)    << full;
}

void TestVideo::yuvShaderDraw()
{
  QFETCH(int, pixelFormat);
  QFETCH(int, range);

  QOffscreenSurface surface;
  surface.create();
  QOpenGLContext context;
  if (!context.create() || !context.makeCurrent(&surface))
    QSKIP("No OpenGL context available");
  if (context.isOpenGLES())
    QSKIP("Test needs desktop OpenGL (fixed-function pipeline)");

  QVideoFrame frame = _makeYuvFrame(QVideoFrameFormat::PixelFormat(pixelFormat),
                                    QVideoFrameFormat::ColorRange(range));
  QVERIFY(frame.isValid());
  QVERIFY(YuvTexture::supportsFormat(frame.pixelFormat()));

  YuvTexture yuv;
  QVERIFY(yuv.upload(frame));
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));

  QImage img = _drawYuv(yuv);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
  _checkQuadrants([&](int x, int y) { return QColor(img.pixel(x, y)); }, "shader output");

  // Re-upload in place.
  QVERIFY(yuv.upload(frame));
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));

  for (GLuint id : yuv.textureIds())
    glDeleteTextures(1, &id);
  context.doneCurrent();
}

void TestVideo::decodedYuvShaderDraw()
{
  if (!_clips.contains("yuv420p"))
    QSKIP("ffmpeg (with libx264) not available to generate test clips");

  QOffscreenSurface surface;
  surface.create();
  QOpenGLContext context;
  if (!context.create() || !context.makeCurrent(&surface))
    QSKIP("No OpenGL context available");
  if (context.isOpenGLES())
    QSKIP("Test needs desktop OpenGL (fixed-function pipeline)");

  // Decoded YUV frames are kept unconverted for the GPU path.
  VideoPlayerImpl player;
  QVERIFY(player.loadMovie(_clips["yuv420p"]));
  QTRY_VERIFY_WITH_TIMEOUT(player.hasBits() && player.bitsHaveChanged(), FRAME_TIMEOUT_MS);
  QVERIFY(player.hasVideoFrame());
  QVideoFrame frame = player.takeVideoFrame();
  QVERIFY(!player.bitsHaveChanged());
  player.setPlayState(false);

  QVERIFY(context.makeCurrent(&surface));
  YuvTexture yuv;
  QVERIFY(yuv.upload(frame));
  QImage img = _drawYuv(yuv);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
  _checkQuadrants([&](int x, int y) { return QColor(img.pixel(x, y)); }, "decoded frame via shader");

  for (GLuint id : yuv.textureIds())
    glDeleteTextures(1, &id);
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

void TestVideo::preparationIsTimedWhenEnabled()
{
  FrameStatsOn on;
  FrameFeeder feeder;
  feeder.feed(_makeYuvFrame(QVideoFrameFormat::Format_NV12, QVideoFrameFormat::ColorRange_Video));

  const FrameStats::Summary summary = feeder.frameStats()->summary();
  QCOMPARE(summary.stages[FrameStats::PREPARE].count, 1);
  QVERIFY(summary.stages[FrameStats::PREPARE].maxNs > 0);
  // The path says where the colours are converted, and from what.
  if (YuvTexture::isAvailable())
    QCOMPARE(summary.path, QString("converted to RGB on the GPU, NV12"));
  else
    QCOMPARE(summary.path, QString("converted to RGB on the CPU, NV12"));

  // An RGB frame needs no conversion.
  QVideoFrame rgb(QVideoFrameFormat(QSize(CLIP_SIZE, CLIP_SIZE), QVideoFrameFormat::Format_RGBX8888));
  feeder.feed(rgb);
  QCOMPARE(feeder.frameStats()->summary().path, QString("RGB, RGBX8888"));
}

void TestVideo::preparationIsNotTimedWhenDisabled()
{
  FrameFeeder feeder;
  feeder.feed(_makeYuvFrame(QVideoFrameFormat::Format_NV12, QVideoFrameFormat::ColorRange_Video));
  QVERIFY(feeder.frameStats()->summary().isEmpty());
}

void TestVideo::replacedFramesAreDropped()
{
  FrameStatsOn on;
  FrameFeeder feeder;
  const QVideoFrame frame = _makeYuvFrame(QVideoFrameFormat::Format_NV12, QVideoFrameFormat::ColorRange_Video);

  // The first frame is replaced before anyone takes it.
  feeder.feed(frame);
  feeder.feed(frame);
  QCOMPARE(feeder.frameStats()->summary().dropped, 1);

  // Taken before the next one: not dropped.
  feeder.getFrameImage(); // does not take the frame
  feeder.getBits();
  feeder.feed(frame);
  QCOMPARE(feeder.frameStats()->summary().dropped, 1);
}

void TestVideo::forcedCpuConversion()
{
  if (!YuvTexture::isAvailable())
    QSKIP("YUV frames are already converted on the CPU here");

  VideoImpl::setForceCpuConversion(true);
  FrameStatsOn on;
  FrameFeeder feeder;
  feeder.feed(_makeYuvFrame(QVideoFrameFormat::Format_NV12, QVideoFrameFormat::ColorRange_Video));
  VideoImpl::setForceCpuConversion(false);

  QVERIFY(feeder.hasBits());
  QVERIFY(!feeder.hasVideoFrame());
  QCOMPARE(feeder.frameStats()->summary().path, QString("converted to RGB on the CPU, NV12"));
  _checkQuadrants([&](int x, int y) { return QColor(feeder.getFrameImage().pixel(x, y)); },
                  "frame converted on the CPU");
}

void TestVideo::uploadIsTimedWhenEnabled()
{
  if (!_clips.contains("yuv420p"))
    QSKIP("ffmpeg (with libx264) not available to generate test clips");

  QOffscreenSurface surface;
  surface.create();
  QOpenGLContext context;
  if (!context.create() || !context.makeCurrent(&surface))
    QSKIP("No OpenGL context available");
  if (context.isOpenGLES())
    QSKIP("Test needs desktop OpenGL (fixed-function pipeline)");

  FrameStatsOn on;
  Video video(_clips["yuv420p"], VIDEO_URI, 1.0);
  QTRY_VERIFY_WITH_TIMEOUT(video.bitsHaveChanged(), FRAME_TIMEOUT_MS);
  video.pause();

  QVERIFY(context.makeCurrent(&surface));
  video.update();
  video.bind();
  video.release();
  QCOMPARE(video.frameStats()->summary().stages[FrameStats::UPLOAD].count, 1);

  // No new frame: nothing to upload.
  if (!video.bitsHaveChanged())
  {
    video.bind();
    video.release();
    QCOMPARE(video.frameStats()->summary().stages[FrameStats::UPLOAD].count, 1);
  }
  context.doneCurrent();
}
