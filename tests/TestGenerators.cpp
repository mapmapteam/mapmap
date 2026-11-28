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
#include "Palette.h"

#include <QJsonObject>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSignalSpy>
#include <cmath>
#include <memory>

using namespace mmp;

namespace {

/// Side of the images drawn by the tests, in pixels.
const int SIZE = 64;

/// Largest difference allowed on a colour channel, out of 255.
const int TOLERANCE = 2;

/// Time the colour wheel plays between two drawings, in milliseconds.
const int PLAY_MS = 200;

/// Speed of a polar pattern in the tests: one turn per second.
const double ONE_TURN_PER_SECOND_RPM = 60.0;

/// Largest difference allowed on the angle turned, in turns, for the timers of the tests.
const double PHASE_TOLERANCE = 0.05;

/// Largest difference allowed on a colour channel at the centre pixel, which is half a
/// pixel from the centre of the disk.
const int CENTRE_TOLERANCE = 10;

/// Largest saturation, out of 255, of a colour that counts as nearly grey.
const int NEARLY_GREY_SATURATION = 32;

/// Positions in a SIZE × SIZE image: the centre, a point in the upper right, one in the
/// lower right, one in the middle of the top half, and a point at three tenths of the radius.
const int CENTRE = SIZE / 2;
const int QUARTER = SIZE / 4;
const int THREE_QUARTERS = 3 * SIZE / 4;
const int THREE_TENTHS_RADIUS = CENTRE + 3 * CENTRE / 10;

/// Added to palette positions by the colour wheel shader, against rounding.
const double PALETTE_INDEX_EPSILON = 0.0001;

/// Appends the paths of the settings under node, in declaration order.
void collectLeafPaths(SettingNode* node, QStringList* paths)
{
  if (node->kind() == SettingNode::SettingKind)
    paths->append(node->path());
  else if (node->kind() == SettingNode::GroupKind)
    for (SettingNode* child : static_cast<SettingGroup*>(node)->children())
      collectLeafPaths(child, paths);
  else if (node->kind() == SettingNode::ListKind)
  {
    SettingList* list = static_cast<SettingList*>(node);
    for (int index = 0; index < list->count(); index++)
      collectLeafPaths(list->at(index), paths);
  }
}

/// A colour wheel of size SIZE with the black and white palette.
void makeBlackAndWhite(ColorWheelGenerator& pattern)
{
  pattern.setSize(SIZE, SIZE);
  QVERIFY(pattern.settings()->find("palette")->setValue("black_and_white"));
}

/// Sets a setting of a generator.
void set(Generator& generator, const char* id, const QVariant& value)
{
  QVERIFY2(generator.settings()->find(id)->setValue(value), id);
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

bool closeColors(const QColor& a, const QColor& b, int tolerance = TOLERANCE)
{
  return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance &&
         qAbs(a.blue() - b.blue()) <= tolerance && qAbs(a.alpha() - b.alpha()) <= tolerance;
}

/// Returns the distance between two angles, in turns, the short way round.
double turnDistance(double a, double b)
{
  const double difference = std::fabs(a - b);
  return std::fmin(difference - std::floor(difference), 1.0 - (difference - std::floor(difference)));
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
  const QStringList wheelPaths = generator
      + QStringList { "geometry", "angular_divisions", "radial_divisions", "grid_blend", "smoothness",
                      "palette", "hue_shift", "radial_saturation", "radial_lightness",
                      "radial_falloff", "radial_reverse", "speed", "reverse" };
  QCOMPARE(leafPaths(wheel), wheelPaths);
  QCOMPARE(undescribedPaths(wheel), QStringList());

  ColorGenerator color;
  const QStringList colorPaths = generator + QStringList { "color" };
  QCOMPARE(leafPaths(color), colorPaths);
  QCOMPARE(undescribedPaths(color), QStringList());

  // Only the polar pattern changes with time.
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

  // A plain colour has a smaller image, by default and in its settings.
  ColorGenerator color;
  QCOMPARE(color.getWidth(), int(ColorGenerator::DEFAULT_COLOR_SIZE));
  QCOMPARE(color.settings()->find("height")->value().toInt(), int(ColorGenerator::DEFAULT_COLOR_SIZE));
  QCOMPARE(color.settings()->find("width")->info().defaultValue().toInt(), int(ColorGenerator::DEFAULT_COLOR_SIZE));

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
  set(wheel, "speed", ONE_TURN_PER_SECOND_RPM);
  const QImage before = drawnImage(wheel);

  wheel.play();
  QTest::qWait(PLAY_MS);
  FrameCounter::advance();
  const QImage after = drawnImage(wheel);

  // The segment at the top shows another colour.
  const int topHueBefore = before.pixelColor(CENTRE, QUARTER).hue();
  const int topHueAfter = after.pixelColor(CENTRE, QUARTER).hue();
  QVERIFY2(topHueBefore != topHueAfter, qPrintable(QString("hue %1 did not change").arg(topHueBefore)));

  // Rewinding goes back to the first image.
  wheel.pause();
  wheel.rewind();
  const QImage rewound = drawnImage(wheel);
  CHECK_PIXEL(rewound, CENTRE, QUARTER, before.pixelColor(CENTRE, QUARTER));
}

void TestGenerators::colorWheelSpeedChangesDoNotJump()
{
  ColorWheelGenerator wheel;
  set(wheel, "speed", ONE_TURN_PER_SECOND_RPM);
  wheel.play();
  QTest::qWait(PLAY_MS);

  // Stopping keeps the angle turned so far, and the pattern is then still.
  set(wheel, "speed", 0.0);
  const double stopped = wheel.phase();
  QVERIFY2(qAbs(stopped - PLAY_MS / 1000.0) < PHASE_TOLERANCE, qPrintable(QString::number(stopped)));
  QVERIFY(!wheel.isAnimated());
  QTest::qWait(PLAY_MS);

  // Starting again, in reverse, goes on from there.
  set(wheel, "reverse", true);
  set(wheel, "speed", ONE_TURN_PER_SECOND_RPM);
  QVERIFY(turnDistance(wheel.phase(), stopped) < PHASE_TOLERANCE);
  QVERIFY(wheel.isAnimated());
  QTest::qWait(PLAY_MS);
  set(wheel, "speed", 0.0);
  QVERIFY2(turnDistance(wheel.phase(), stopped - PLAY_MS / 1000.0) < PHASE_TOLERANCE,
           qPrintable(QString::number(wheel.phase())));
}

void TestGenerators::colorWheelGeometryPresets()
{
  ColorWheelGenerator pattern;
  QCOMPARE(pattern.geometryPreset(), QString("segments"));
  QCOMPARE(pattern.settings()->find("geometry")->value().toString(), QString("segments"));

  // A preset sets the four geometry settings, and nothing else.
  set(pattern, "hue_shift", 30.0);
  set(pattern, "geometry", "spiral");
  QCOMPARE(pattern.settings()->find("angular_divisions")->value().toInt(), 6);
  QCOMPARE(pattern.settings()->find("radial_divisions")->value().toDouble(), 4.0);
  QCOMPARE(pattern.settings()->find("hue_shift")->value().toDouble(), 30.0);

  // The preset shown follows the settings.
  QSignalSpy spy(&pattern, &Element::settingChanged);
  set(pattern, "radial_divisions", 3.5);
  QCOMPARE(pattern.geometryPreset(), ColorWheelGenerator::CUSTOM_GEOMETRY);
  bool geometryNotified = false;
  for (const QList<QVariant>& arguments : spy)
    geometryNotified = geometryNotified || arguments.at(1).toString() == "geometry";
  QVERIFY(geometryNotified);
  set(pattern, "radial_divisions", -4.0);
  QCOMPARE(pattern.geometryPreset(), QString("reverse_spiral"));

  set(pattern, "geometry", "checkerboard");
  QCOMPARE(pattern.settings()->find("grid_blend")->value().toDouble(), 1.0);
  QCOMPARE(pattern.settings()->find("angular_divisions")->value().toInt(), 12);
  QCOMPARE(pattern.settings()->find("radial_divisions")->value().toDouble(), 6.0);

  // Custom brings back the last geometry of the user's own.
  set(pattern, "geometry", ColorWheelGenerator::CUSTOM_GEOMETRY);
  QCOMPARE(pattern.settings()->find("geometry")->value().toString(), ColorWheelGenerator::CUSTOM_GEOMETRY);
  QCOMPARE(pattern.settings()->find("angular_divisions")->value().toInt(), 6);
  QCOMPARE(pattern.settings()->find("radial_divisions")->value().toDouble(), 3.5);
  QCOMPARE(pattern.settings()->find("grid_blend")->value().toDouble(), 0.0);

  // Without one, Custom keeps the geometry, to change.
  ColorWheelGenerator fresh;
  set(fresh, "geometry", ColorWheelGenerator::CUSTOM_GEOMETRY);
  QCOMPARE(fresh.settings()->find("geometry")->value().toString(), ColorWheelGenerator::CUSTOM_GEOMETRY);
  QCOMPARE(fresh.settings()->find("angular_divisions")->value().toInt(), ColorWheelGenerator::DEFAULT_ANGULAR_DIVISIONS);

  // A custom geometry read from a project is the one Custom brings back.
  QJsonObject obj;
  pattern.write(obj);
  ColorWheelGenerator reread;
  reread.read(obj);
  set(reread, "geometry", "segments");
  set(reread, "geometry", ColorWheelGenerator::CUSTOM_GEOMETRY);
  QCOMPARE(reread.settings()->find("radial_divisions")->value().toDouble(), 3.5);
}

void TestGenerators::colorWheelPalettes()
{
  ColorWheelGenerator pattern;
  QCOMPARE(pattern.paletteColors().size(), 12);
  QCOMPARE(pattern.paletteColors().first(), QColor(Qt::red));

  // A preset replaces the colours.
  set(pattern, "palette", "triadic");
  QCOMPARE(pattern.paletteColors().size(), 3);
  QCOMPARE(pattern.paletteColors(), Palette::presetColors("triadic"));

  // Every preset has colours, within the size a shader receives.
  for (const SettingOption& option : Palette::presetOptions())
  {
    const QVector<QColor> colors = Palette::presetColors(option.id);
    QVERIFY2(!colors.isEmpty(), qPrintable(option.id));
    QVERIFY(colors.size() <= Palette::MAX_SIZE);
  }
}

void TestGenerators::colorWheelPaletteClosesTheCircle()
{
  // One colour per segment, two alternating colours, every other colour, none at all.
  QCOMPARE(ColorWheelGenerator::paletteScale(12, 12), 1.0);
  QCOMPARE(ColorWheelGenerator::paletteScale(12, 2), 1.0);
  QCOMPARE(ColorWheelGenerator::paletteScale(6, 12), 2.0);
  QCOMPARE(ColorWheelGenerator::paletteScale(0, 5), 1.0);

  // Three colours on 25 divisions: a whole number of cycles, skipping two entries.
  QCOMPARE(ColorWheelGenerator::paletteScale(25, 3), 27.0 / 25.0);

  for (int divisions = 1; divisions <= 64; divisions++)
    for (int size = 1; size <= Palette::MAX_SIZE; size++)
    {
      // Around the circle, the palette always goes round a whole number of times.
      const double scale = ColorWheelGenerator::paletteScale(divisions, size);
      const double turns = scale * divisions / size;
      QVERIFY(qAbs(turns - qRound(turns)) < 1e-9 && qRound(turns) >= 1);

      // Neighbours never have the same colour, but with two colours on an odd number of
      // divisions, or with one colour.
      if (size == 1 || (size == 2 && divisions % 2 == 1) || divisions == 1)
        continue;
      for (int k = 0; k < divisions; k++)
      {
        const int entry = int(std::floor(k * scale + PALETTE_INDEX_EPSILON)) % size;
        const int next = int(std::floor((k + 1) * scale + PALETTE_INDEX_EPSILON)) % size;
        QVERIFY2(entry != next, qPrintable(QString("%1 divisions, %2 colours, division %3")
                                           .arg(divisions).arg(size).arg(k)));
      }
    }
}

void TestGenerators::colorWheelSavesItsPalette()
{
  ColorWheelGenerator pattern;
  set(pattern, "palette", "complementary");
  QJsonObject obj;
  pattern.write(obj);
  QCOMPARE(obj.value("palette").toString(), QString("complementary"));
  QVERIFY(!obj.contains("colors"));
  QVERIFY(!obj.contains("geometry"));

  // The colours come from the preset.
  ColorWheelGenerator reread;
  reread.read(obj);
  QCOMPARE(reread.paletteColors(), Palette::presetColors("complementary"));
}

void TestGenerators::colorWheelGeometries()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");
  const QColor white(Qt::white), black(Qt::black), grey(128, 128, 128);

  // Two segments: white above, black below. Transparent outside the disk.
  ColorWheelGenerator pattern;
  makeBlackAndWhite(pattern);
  set(pattern, "angular_divisions", 2);
  QImage image = drawnImage(pattern);
  CHECK_PIXEL(image, THREE_QUARTERS, QUARTER, white);
  CHECK_PIXEL(image, THREE_QUARTERS, THREE_QUARTERS, black);
  QCOMPARE(image.pixelColor(0, 0).alpha(), 0);

  // Full smoothness: the middle of a segment is half way to the next colour.
  set(pattern, "smoothness", 1.0);
  image = drawnImage(pattern);
  CHECK_PIXEL(image, CENTRE, QUARTER, grey);
  set(pattern, "smoothness", 0.0);

  // Concentric rings: white at the centre, black from the second ring.
  set(pattern, "angular_divisions", 0);
  set(pattern, "radial_divisions", 4.0);
  image = drawnImage(pattern);
  CHECK_PIXEL(image, CENTRE, CENTRE, white);
  CHECK_PIXEL(image, THREE_TENTHS_RADIUS, CENTRE, black);

  // A polar checkerboard: the colours alternate along both the angle and the radius.
  set(pattern, "angular_divisions", 2);
  set(pattern, "grid_blend", 1.0);
  image = drawnImage(pattern);
  const int upper = CENTRE - 2;
  const int lower = CENTRE + 2;
  CHECK_PIXEL(image, CENTRE + 1, upper, white);
  CHECK_PIXEL(image, CENTRE + 1, lower, black);
  CHECK_PIXEL(image, THREE_TENTHS_RADIUS, upper, black);
  CHECK_PIXEL(image, THREE_TENTHS_RADIUS, lower, white);

  // The disk stays round in a wide image.
  pattern.setSize(2 * SIZE, SIZE);
  image = drawnImage(pattern);
  QCOMPARE(image.pixelColor(SIZE / 4, CENTRE).alpha(), 0);
  QVERIFY(image.pixelColor(SIZE, CENTRE).alpha() > 0);
}

void TestGenerators::colorWheelColors()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  // A monochromatic pattern turned to green; greys keep their colour.
  ColorWheelGenerator pattern;
  pattern.setSize(SIZE, SIZE);
  set(pattern, "angular_divisions", 0);
  set(pattern, "palette", "monochromatic");
  set(pattern, "hue_shift", 120.0);
  QImage image = drawnImage(pattern);
  const QColor centre = image.pixelColor(CENTRE, CENTRE);
  QVERIFY2(centre.green() > centre.red() && centre.green() > centre.blue(), qPrintable(colorText(centre)));
  set(pattern, "palette", "grayscale");
  image = drawnImage(pattern);
  const QColor grey = image.pixelColor(CENTRE, CENTRE);
  QVERIFY(qAbs(grey.red() - grey.green()) <= TOLERANCE && qAbs(grey.green() - grey.blue()) <= TOLERANCE);

  // Radial lightness: a white or a black centre.
  set(pattern, "palette", "spectrum");
  set(pattern, "hue_shift", 0.0);
  set(pattern, "radial_lightness", 1.0);
  QVERIFY(closeColors(drawnImage(pattern).pixelColor(CENTRE, CENTRE), QColor(Qt::white), CENTRE_TOLERANCE));
  set(pattern, "radial_lightness", -1.0);
  QVERIFY(closeColors(drawnImage(pattern).pixelColor(CENTRE, CENTRE), QColor(Qt::black), CENTRE_TOLERANCE));
  set(pattern, "radial_lightness", 0.0);

  // Radial saturation: a grey centre, of the lightness of the colour (red, half light).
  set(pattern, "radial_saturation", 1.0);
  QVERIFY(closeColors(drawnImage(pattern).pixelColor(CENTRE, CENTRE), QColor(128, 128, 128), CENTRE_TOLERANCE));

  // Reversed, the circumference is grey and the centre keeps its colour.
  set(pattern, "radial_reverse", true);
  image = drawnImage(pattern);
  QVERIFY(closeColors(image.pixelColor(CENTRE, CENTRE), QColor(Qt::red), CENTRE_TOLERANCE));
  const QColor edge = image.pixelColor(CENTRE, 1);
  QVERIFY2(edge.saturation() < NEARLY_GREY_SATURATION, qPrintable(colorText(edge)));
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
