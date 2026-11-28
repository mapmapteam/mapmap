/*
 * TestGenerators.cpp
 *
 * Unit tests for the generators (src/core/Generator, Generators), the intermediate
 * image they draw into (src/core/IntermediateImage) and the frame counter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestGenerators.h"

#include "FrameCounter.h"
#include "Generators.h"
#include "IntermediateImage.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSignalSpy>
#include <memory>

using namespace mmp;

namespace {

/// Side of the images drawn by the tests, in pixels.
const int SIZE = 64;

/// Largest difference allowed on a colour channel, out of 255.
const int TOLERANCE = 2;

/// Time the colour wheel plays between two drawings, in milliseconds.
const int PLAY_MS = 200;

/// Largest saturation, out of 255, of the white centre of a colour wheel.
const int WHITE_SATURATION_MAX = 16;

/// Appends the paths of the settings under node, in declaration order.
void collectLeafPaths(SettingNode* node, QStringList* paths)
{
  if (node->kind() == SettingNode::SettingKind)
    paths->append(node->path());
  else if (node->kind() == SettingNode::GroupKind)
    for (SettingNode* child : static_cast<SettingGroup*>(node)->children())
      collectLeafPaths(child, paths);
}

/// Returns the paths of the settings of an object, in declaration order.
QStringList leafPaths(const Serializable& object)
{
  QStringList paths;
  collectLeafPaths(object.settings(), &paths);
  return paths;
}

/// Returns the paths of the settings of an object that have no description.
QStringList undescribedPaths(const Serializable& object)
{
  QStringList undescribed;
  for (const QString& path : leafPaths(object))
    if (object.settings()->find(path)->info().description().isEmpty())
      undescribed.append(path);
  return undescribed;
}

/// An OpenGL context made current on an offscreen surface for the life of the object.
struct OffscreenContext
{
  QOffscreenSurface surface;
  QOpenGLContext context;
  bool ok = false;

  explicit OffscreenContext(QOpenGLContext* shareContext = nullptr)
  {
    surface.create();
    context.setShareContext(shareContext);
    ok = context.create() && makeCurrent() && !context.isOpenGLES();
  }
  ~OffscreenContext() { context.doneCurrent(); }

  bool makeCurrent() { return context.makeCurrent(&surface); }
};

/// Draws the generator if due, and returns its image with the top row first.
QImage drawnImage(Generator& generator)
{
  generator.update();
  generator.bind();
  QImage bottomUp(generator.getWidth(), generator.getHeight(), QImage::Format_RGBA8888);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bottomUp.bits());
  generator.release();

  // OpenGL gives the bottom row first.
  QImage image(bottomUp.size(), bottomUp.format());
  for (int y = 0; y < image.height(); y++)
    memcpy(image.scanLine(y), bottomUp.constScanLine(image.height() - 1 - y), size_t(image.bytesPerLine()));
  return image;
}

bool closeColors(const QColor& a, const QColor& b)
{
  return qAbs(a.red() - b.red()) <= TOLERANCE && qAbs(a.green() - b.green()) <= TOLERANCE &&
         qAbs(a.blue() - b.blue()) <= TOLERANCE && qAbs(a.alpha() - b.alpha()) <= TOLERANCE;
}

QString colorText(const QColor& c)
{
  return QString("(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

/// Checks the colour of a pixel of an image.
#define CHECK_PIXEL(image, x, y, expected) \
  QVERIFY2(closeColors((image).pixelColor((x), (y)), (expected)), \
           qPrintable(QString("pixel (%1, %2) is %3, expected %4").arg(x).arg(y) \
                      .arg(colorText((image).pixelColor((x), (y))), colorText(expected))))

} // namespace

void TestGenerators::generatorsDeclareTheirSettings()
{
  const QStringList generator = { "id", "name", "locked", "opacity", "x", "y", "width", "height" };

  CheckerGenerator checker;
  const QStringList checkerPaths = generator + QStringList { "count", "back_color", "front_color" };
  QCOMPARE(leafPaths(checker), checkerPaths);
  QCOMPARE(undescribedPaths(checker), QStringList());

  GridGenerator grid;
  const QStringList gridPaths = generator + QStringList { "columns", "rows", "thickness", "back_color", "line_color" };
  QCOMPARE(leafPaths(grid), gridPaths);
  QCOMPARE(undescribedPaths(grid), QStringList());

  ColorWheelGenerator wheel;
  const QStringList wheelPaths = generator + QStringList { "speed", "reverse" };
  QCOMPARE(leafPaths(wheel), wheelPaths);
  QCOMPARE(undescribedPaths(wheel), QStringList());

  ColorGenerator color;
  const QStringList colorPaths = generator + QStringList { "color" };
  QCOMPARE(leafPaths(color), colorPaths);
  QCOMPARE(undescribedPaths(color), QStringList());

  // Only the colour wheel changes with time.
  QVERIFY(wheel.isAnimated());
  QVERIFY(!checker.isAnimated() && !grid.isAnimated() && !color.isAnimated());
}

void TestGenerators::frameCounterGrows()
{
  const quint64 before = FrameCounter::current();
  FrameCounter::advance();
  QCOMPARE(FrameCounter::current(), before + 1);
}

void TestGenerators::sizeStaysWithinBounds()
{
  CheckerGenerator checker;
  QCOMPARE(checker.getWidth(), int(Generator::DEFAULT_SIZE));
  QCOMPARE(checker.getHeight(), int(Generator::DEFAULT_SIZE));

  checker.setSize(0, Generator::MAX_SIZE + 1);
  QCOMPARE(checker.getWidth(), 1);
  QCOMPARE(checker.getHeight(), int(Generator::MAX_SIZE));

  // Through the settings, as the inspector, OSC and project files write them.
  QVERIFY(checker.settings()->find("width")->setValue(640));
  QCOMPARE(checker.getWidth(), 640);
  QCOMPARE(checker.getRect(), QRectF(0, 0, 640, Generator::MAX_SIZE));
}

void TestGenerators::sizeChangeIsSignalled()
{
  GridGenerator grid;
  QSignalSpy spy(&grid, &Generator::sizeChanged);
  grid.setSize(800, 600);
  QCOMPARE(spy.count(), 1);
  const QList<QVariant> arguments = spy.takeFirst();
  QCOMPARE(arguments.at(1).toInt(), int(Generator::DEFAULT_SIZE));
  QCOMPARE(arguments.at(2).toInt(), int(Generator::DEFAULT_SIZE));
  QCOMPARE(arguments.at(3).toInt(), 800);
  QCOMPARE(arguments.at(4).toInt(), 600);

  // The same size again changes nothing.
  grid.setSize(800, 600);
  QCOMPARE(spy.count(), 0);
}

void TestGenerators::colorFillsTheImage()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  const QColor color(10, 200, 30, 128);
  ColorGenerator generator(color);
  generator.setSize(SIZE, SIZE);
  const QImage image = drawnImage(generator);
  QCOMPARE(generator.renderCount(), 1);
  CHECK_PIXEL(image, 0, 0, color);
  CHECK_PIXEL(image, SIZE - 1, SIZE - 1, color);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
}

void TestGenerators::checkerStartsWithFrontColorAtTopLeft()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  CheckerGenerator checker;
  checker.setSize(SIZE, SIZE);
  const QImage image = drawnImage(checker);
  const int square = SIZE / CheckerGenerator::DEFAULT_COUNT;
  const QColor front(Qt::white), back(Qt::black);
  CHECK_PIXEL(image, 0, 0, front);
  CHECK_PIXEL(image, square, 0, back);
  CHECK_PIXEL(image, 0, square, back);
  CHECK_PIXEL(image, square, square, front);
  CHECK_PIXEL(image, SIZE - 1, SIZE - 1, front);

  // Colours and count follow the settings.
  QVERIFY(checker.settings()->find("count")->setValue(2));
  QVERIFY(checker.settings()->find("front_color")->setValue(QColor(Qt::red)));
  const QImage changed = drawnImage(checker);
  CHECK_PIXEL(changed, square, square, QColor(Qt::red));
  CHECK_PIXEL(changed, SIZE / 2, 0, back);
}

void TestGenerators::gridLinesArePixelExact()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  GridGenerator grid;
  grid.setSize(SIZE, SIZE);
  const QImage image = drawnImage(grid);
  const int cell = SIZE / GridGenerator::DEFAULT_COUNT;
  const QColor line(Qt::white), back(Qt::black);
  const int middle = cell / 2;

  // A line of one pixel along each border and between the cells.
  CHECK_PIXEL(image, 0, middle, line);
  CHECK_PIXEL(image, 1, middle, back);
  CHECK_PIXEL(image, cell - 1, middle, back);
  CHECK_PIXEL(image, cell, middle, line);
  CHECK_PIXEL(image, cell + 1, middle, back);
  CHECK_PIXEL(image, SIZE - 1, middle, line);
  CHECK_PIXEL(image, SIZE - 2, middle, back);
  CHECK_PIXEL(image, middle, 0, line);
  CHECK_PIXEL(image, middle, SIZE - 1, line);
  CHECK_PIXEL(image, middle, middle, back);

  // Thicker lines grow on both sides of an inner line.
  QVERIFY(grid.settings()->find("thickness")->setValue(3.0));
  const QImage thick = drawnImage(grid);
  CHECK_PIXEL(thick, cell - 2, middle, back);
  CHECK_PIXEL(thick, cell - 1, middle, line);
  CHECK_PIXEL(thick, cell + 1, middle, line);
  CHECK_PIXEL(thick, cell + 2, middle, back);
  CHECK_PIXEL(thick, 2, middle, line);
  CHECK_PIXEL(thick, 3, middle, back);
}

void TestGenerators::stillGeneratorDrawsOnlyWhenChanged()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  CheckerGenerator checker;
  checker.setSize(SIZE, SIZE);
  checker.play();
  checker.update();
  QCOMPARE(checker.renderCount(), 1);
  QVERIFY(!checker.needsRedraw());

  // A new frame does not change a still image.
  FrameCounter::advance();
  checker.update();
  QCOMPARE(checker.renderCount(), 1);

  // A setting does.
  QVERIFY(checker.settings()->find("count")->setValue(8));
  QVERIFY(checker.needsRedraw());
  checker.update();
  QCOMPARE(checker.renderCount(), 2);
  QVERIFY(!checker.needsRedraw());
}

void TestGenerators::animatedGeneratorDrawsOncePerFrameAcrossContexts()
{
  // Two contexts sharing their textures, like the views of MapMap.
  OffscreenContext first;
  if (!first.ok)
    QSKIP("No desktop OpenGL context available");
  OffscreenContext second(&first.context);
  if (!second.ok)
    QSKIP("No second shared OpenGL context available");

  ColorWheelGenerator wheel;
  wheel.setSize(SIZE, SIZE);
  wheel.play();
  QVERIFY(wheel.needsRedraw());

  // The first view of a frame draws it, the other only reads it.
  FrameCounter::advance();
  QVERIFY(first.makeCurrent());
  wheel.update();
  QCOMPARE(wheel.renderCount(), 1);
  QVERIFY(second.makeCurrent());
  wheel.update();
  QCOMPARE(wheel.renderCount(), 1);
  QVERIFY(first.makeCurrent());
  wheel.update();
  QCOMPARE(wheel.renderCount(), 1);

  // The next frame draws it again, whichever view paints first.
  FrameCounter::advance();
  QVERIFY(second.makeCurrent());
  wheel.update();
  QCOMPARE(wheel.renderCount(), 2);

  // The other context reads what the first one drew.
  QVERIFY(first.makeCurrent());
  const QImage image = drawnImage(wheel);
  QCOMPARE(wheel.renderCount(), 2);
  QVERIFY(image.pixelColor(SIZE / 2, 0).saturation() > 0);

  // Paused, it is not drawn again.
  wheel.pause();
  QVERIFY(!wheel.needsRedraw());
  FrameCounter::advance();
  wheel.update();
  QCOMPARE(wheel.renderCount(), 2);
}

void TestGenerators::colorWheelTurnsWithTime()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  ColorWheelGenerator wheel;
  wheel.setSize(SIZE, SIZE);
  QVERIFY(wheel.settings()->find("speed")->setValue(1.0));
  const QImage before = drawnImage(wheel);

  wheel.play();
  QTest::qWait(PLAY_MS);
  FrameCounter::advance();
  const QImage after = drawnImage(wheel);

  // The top of the wheel shows another hue; the centre stays white.
  const int topHueBefore = before.pixelColor(SIZE / 2, 0).hue();
  const int topHueAfter = after.pixelColor(SIZE / 2, 0).hue();
  QVERIFY2(topHueBefore != topHueAfter,
           qPrintable(QString("hue %1 did not change").arg(topHueBefore)));
  QVERIFY(after.pixelColor(SIZE / 2, SIZE / 2).saturation() < WHITE_SATURATION_MAX);

  // Rewinding goes back to the first image.
  wheel.pause();
  wheel.rewind();
  const QImage rewound = drawnImage(wheel);
  CHECK_PIXEL(rewound, SIZE / 2, 0, before.pixelColor(SIZE / 2, 0));
}

void TestGenerators::intermediateImageGivesBackTheFramebuffer()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");
  QOpenGLFunctions* f = gl.context.functions();

  // The caller draws into its own framebuffer, with blending and a viewport of its own.
  GLuint callerFramebuffer = 0, callerTexture = 0;
  glGenTextures(1, &callerTexture);
  glBindTexture(GL_TEXTURE_2D, callerTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  f->glGenFramebuffers(1, &callerFramebuffer);
  f->glBindFramebuffer(GL_FRAMEBUFFER, callerFramebuffer);
  f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, callerTexture, 0);
  glViewport(1, 2, 3, 4);
  glEnable(GL_BLEND);

  IntermediateImage image;
  bool blendWhileDrawing = true;
  QVERIFY(image.render(QSize(SIZE, SIZE), [&]() { blendWhileDrawing = glIsEnabled(GL_BLEND); }));
  QVERIFY(!blendWhileDrawing);
  QCOMPARE(image.size(), QSize(SIZE, SIZE));

  GLint framebuffer = 0, viewport[4] = { 0, 0, 0, 0 };
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
  glGetIntegerv(GL_VIEWPORT, viewport);
  QCOMPARE(GLuint(framebuffer), callerFramebuffer);
  QCOMPARE(viewport[0], 1);
  QCOMPARE(viewport[1], 2);
  QCOMPARE(viewport[2], 3);
  QCOMPARE(viewport[3], 4);
  QVERIFY(glIsEnabled(GL_BLEND));

  // Nothing to draw into.
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("cannot be drawn without"));
  QVERIFY(!image.render(QSize(), []() {}));

  f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
  f->glDeleteFramebuffers(1, &callerFramebuffer);
  glDeleteTextures(1, &callerTexture);
  for (GLuint id : image.textureIds())
    glDeleteTextures(1, &id);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
}

void TestGenerators::intermediateImagesTakeTurns()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  IntermediateImage image;
  QCOMPARE(image.texture(), GLuint(0));

  // Each drawing goes to the texture the views are not reading.
  QVERIFY(image.render(QSize(SIZE, SIZE), []() {}));
  const GLuint first = image.texture();
  QVERIFY(image.render(QSize(SIZE, SIZE), []() {}));
  const GLuint second = image.texture();
  QVERIFY(first != 0 && second != 0 && first != second);
  QVERIFY(image.render(QSize(SIZE, SIZE), []() {}));
  QCOMPARE(image.texture(), first);
  QCOMPARE(image.textureIds().size(), 2);

  // The image has mipmaps, for when it is shown smaller.
  image.bind();
  GLint halfWidth = 0, minFilter = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &halfWidth);
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
  QCOMPARE(halfWidth, SIZE / 2);
  QCOMPARE(minFilter, GLint(GL_LINEAR_MIPMAP_LINEAR));

  for (GLuint id : image.textureIds())
    glDeleteTextures(1, &id);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
}

void TestGenerators::drawingIsTimedWhenEnabled()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  FrameStats::setEnabled(true);
  CheckerGenerator checker;
  checker.setName("timed checker");
  checker.setSize(SIZE, SIZE);
  checker.update();
  const FrameStats::Summary summary = checker.frameStats()->summary();
  FrameStats::setEnabled(false);

  QCOMPARE(summary.stages[FrameStats::RENDER].count, 1);
  QCOMPARE(summary.name, QString("timed checker"));
  QVERIFY(summary.path.contains("Checker"));
  QVERIFY(FrameStats::summaryLine(summary).contains("render"));
}
