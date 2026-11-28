/*
 * Generators.cpp
 *
 * (c) 2026 MapMap contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "Generators.h"

#include "Palette.h"

#include <QConicalGradient>
#include <QVector4D>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QPixmap>
#include <cmath>

namespace mmp {

namespace {

/// End of the slider of a number of squares, columns or rows; larger numbers can be typed.
const int COUNT_SLIDER_MAX = 32;

/// Default and largest suggested thickness of the lines of a grid, in pixels.
const double DEFAULT_THICKNESS = 1.0;
const double THICKNESS_SLIDER_MAX = 20.0;

/// Default colour of a colour generator: a mid grey.
const QColor DEFAULT_GENERATOR_COLOR(128, 128, 128);

/// Squares, cells or lines on each side of an icon.
const int ICON_DIVISIONS = 4;

/// Returns an empty icon-sized pixmap and a painter on it.
QPixmap iconPixmap(const QColor& fill)
{
  QPixmap pixmap(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
  pixmap.fill(fill);
  return pixmap;
}

/// Returns the colour value of a setting of a source.
QColor colorSetting(const Source* source, const QString& id)
{
  const Setting* setting = source->settings()->find(id);
  return setting ? setting->value().value<QColor>() : QColor();
}

/// Returns the integer value of a setting of a source.
int intSetting(const Source* source, const QString& id)
{
  const Setting* setting = source->settings()->find(id);
  return setting ? setting->value().toInt() : 0;
}

} // namespace

CheckerGenerator::CheckerGenerator(int id)
  : ShaderGenerator(id)
{
  SettingGroup* root = settings();
  root->addSetting(SettingInfo::integer("count", tr("Count"))
                   .bounds(1, QVariant())
                   .sliderRange(1, COUNT_SLIDER_MAX)
                   .defaultValue(DEFAULT_COUNT)
                   .description(tr("The number of squares on each side.")));
  root->addSetting(SettingInfo::color("back_color", tr("Back Color"))
                   .defaultValue(QColor(Qt::black))
                   .description(tr("The colour of every other square.")));
  root->addSetting(SettingInfo::color("front_color", tr("Front Color"))
                   .defaultValue(QColor(Qt::white))
                   .description(tr("The colour of the square at the top left, and of every other square.")));
  _addShaderInput("count");
  _addShaderInput("back_color");
  _addShaderInput("front_color");
}

QString CheckerGenerator::_fragmentShader() const
{
  return QStringLiteral(
    "void main() {\n"
    "  float column = floor(isf_FragNormCoord.x * float(count));\n"
    "  float row = floor((1.0 - isf_FragNormCoord.y) * float(count));\n"
    "  gl_FragColor = (mod(column + row, 2.0) < 1.0) ? front_color : back_color;\n"
    "}\n");
}

QIcon CheckerGenerator::getIcon() const
{
  QPixmap pixmap = iconPixmap(colorSetting(this, "back_color"));
  QPainter painter(&pixmap);
  const qreal cell = qreal(pixmap.width()) / ICON_DIVISIONS;
  for (int row = 0; row < ICON_DIVISIONS; row++)
    for (int column = 0; column < ICON_DIVISIONS; column++)
      if ((row + column) % 2 == 0)
        painter.fillRect(QRectF(column * cell, row * cell, cell, cell), colorSetting(this, "front_color"));
  return QIcon(pixmap);
}

GridGenerator::GridGenerator(int id)
  : ShaderGenerator(id)
{
  SettingGroup* root = settings();
  root->addSetting(SettingInfo::integer("columns", tr("Columns"))
                   .bounds(1, QVariant())
                   .sliderRange(1, COUNT_SLIDER_MAX)
                   .defaultValue(DEFAULT_COUNT)
                   .description(tr("The number of columns between the lines.")));
  root->addSetting(SettingInfo::integer("rows", tr("Rows"))
                   .bounds(1, QVariant())
                   .sliderRange(1, COUNT_SLIDER_MAX)
                   .defaultValue(DEFAULT_COUNT)
                   .description(tr("The number of rows between the lines.")));
  root->addSetting(SettingInfo::number("thickness", tr("Thickness"))
                   .bounds(0, QVariant())
                   .sliderRange(0, THICKNESS_SLIDER_MAX)
                   .defaultValue(DEFAULT_THICKNESS)
                   .unit(SettingInfo::Pixels)
                   .description(tr("The thickness of the lines.")));
  root->addSetting(SettingInfo::color("back_color", tr("Back Color"))
                   .defaultValue(QColor(Qt::black))
                   .description(tr("The colour between the lines.")));
  root->addSetting(SettingInfo::color("line_color", tr("Line Color"))
                   .defaultValue(QColor(Qt::white))
                   .description(tr("The colour of the lines.")));
  _addShaderInput("columns");
  _addShaderInput("rows");
  _addShaderInput("thickness");
  _addShaderInput("back_color");
  _addShaderInput("line_color");
}

QString GridGenerator::_fragmentShader() const
{
  // Positions are counted from the top left. A line covers the pixels whose centre
  // is within half a thickness of it, the lower edge excluded, so that a line one
  // pixel thick covers exactly one pixel, the one after it. The lines of the
  // borders are moved inside, to be seen whole.
  return QStringLiteral(
    "bool onLine(float position, float cell, float size, float halfThickness, float count) {\n"
    "  float nearest = clamp(floor(position / cell + 0.5), 0.0, count);\n"
    "  float line = clamp(nearest * cell, halfThickness, size - halfThickness);\n"
    "  float offset = position - line;\n"
    "  return offset > -halfThickness && offset <= halfThickness;\n"
    "}\n"
    "void main() {\n"
    "  vec2 position = vec2(isf_FragNormCoord.x, 1.0 - isf_FragNormCoord.y) * RENDERSIZE;\n"
    "  vec2 cell = RENDERSIZE / vec2(float(columns), float(rows));\n"
    "  float halfThickness = thickness * 0.5;\n"
    "  bool line = onLine(position.x, cell.x, RENDERSIZE.x, halfThickness, float(columns)) ||\n"
    "              onLine(position.y, cell.y, RENDERSIZE.y, halfThickness, float(rows));\n"
    "  gl_FragColor = line ? line_color : back_color;\n"
    "}\n");
}

QIcon GridGenerator::getIcon() const
{
  QPixmap pixmap = iconPixmap(colorSetting(this, "back_color"));
  QPainter painter(&pixmap);
  painter.setPen(colorSetting(this, "line_color"));
  const int last = pixmap.width() - 1;
  const int columns = qMax(1, qMin(intSetting(this, "columns"), ICON_DIVISIONS));
  const int rows = qMax(1, qMin(intSetting(this, "rows"), ICON_DIVISIONS));
  for (int i = 0; i <= columns; i++)
    painter.drawLine(i * last / columns, 0, i * last / columns, last);
  for (int i = 0; i <= rows; i++)
    painter.drawLine(0, i * last / rows, last, i * last / rows);
  return QIcon(pixmap);
}

const QString ColorWheelGenerator::CUSTOM_GEOMETRY = QStringLiteral("custom");

namespace {

/// Bounds and defaults of the settings of a colour wheel.
const int MAX_ANGULAR_DIVISIONS = 64;
const double MAX_RADIAL_DIVISIONS = 32.0;
const double RADIAL_DIVISIONS_STEP = 0.1;
const double FRACTION_STEP = 0.01;
const double PERCENT_SCALE = 100.0;
const double MAX_HUE_SHIFT = 180.0;
const double HUE_SHIFT_STEP = 1.0;
const double MIN_RADIAL_FALLOFF = 0.1;
const double MAX_RADIAL_FALLOFF = 5.0;
const double DEFAULT_RADIAL_FALLOFF = 1.0;
const double MAX_SPEED_RPM = 120.0;
const double SPEED_STEP = 0.1;
const double DEFAULT_SPEED_RPM = 5.0;

/// Seconds in a minute, to turn revolutions per minute into turns per second.
const double SECONDS_PER_MINUTE = 60.0;

/// Two geometry settings closer than this are equal, for matching a preset.
const double PRESET_TOLERANCE = 1e-6;

/// The settings that make the geometry, and the display groups of the panel.
const char* const GEOMETRY_SETTINGS[] = { "angular_divisions", "radial_divisions", "grid_blend", "smoothness" };
const char* const GEOMETRY_GROUP = "geometry";
const char* const COLOR_GROUP = "color";
const char* const RADIAL_COLOR_GROUP = "radial_color";
const char* const ANIMATION_GROUP = "animation";

/// Order of the display groups in the panel.
enum DisplayGroupOrder { GEOMETRY_ORDER, COLOR_ORDER, RADIAL_COLOR_ORDER, ANIMATION_ORDER };

/// A shortcut that sets the four geometry settings.
struct GeometryPreset
{
  const char* id;
  int angularDivisions;
  double radialDivisions;
  double gridBlend;
  double smoothness;
};

const GeometryPreset GEOMETRY_PRESETS[] = {
  { "segments",         12,  0.0, 0.0, 0.0 },
  { "continuous_wheel", 12,  0.0, 0.0, 1.0 },
  { "spiral",            6,  4.0, 0.0, 0.0 },
  { "reverse_spiral",    6, -4.0, 0.0, 0.0 },
  { "concentric_rings",  0,  8.0, 0.0, 0.0 },
  { "checkerboard",     12,  6.0, 1.0, 0.0 },
};

/// Returns the geometry presets, custom included, as the options of a Choice setting.
QList<SettingOption> geometryOptions()
{
  return {
    { QStringLiteral("segments"),         ColorWheelGenerator::tr("Segments") },
    { QStringLiteral("continuous_wheel"), ColorWheelGenerator::tr("Continuous Wheel") },
    { QStringLiteral("spiral"),           ColorWheelGenerator::tr("Spiral") },
    { QStringLiteral("reverse_spiral"),   ColorWheelGenerator::tr("Reverse Spiral") },
    { QStringLiteral("concentric_rings"), ColorWheelGenerator::tr("Concentric Rings") },
    { QStringLiteral("checkerboard"),     ColorWheelGenerator::tr("Checkerboard") },
    { ColorWheelGenerator::CUSTOM_GEOMETRY, ColorWheelGenerator::tr("Custom") },
  };
}

/// The fragment shader of a colour wheel; its inputs and extra uniforms are declared before it.
const char* COLOR_WHEEL_SHADER = R"glsl(
// Color Wheel: one fragment shader for colour wheels, spirals, rings and polar checkerboards.
// Inputs (declared by ShaderGenerator from the settings): angular_divisions (int),
// radial_divisions, grid_blend, smoothness, hue_shift (degrees),
// radial_saturation, radial_lightness, radial_falloff (float), radial_reverse (bool).
// Extra uniforms: phase, palette[] (with alpha), palette_size, palette_scale.
// The output is not premultiplied: MapMap blends with the alpha.

const float TAU = 6.28318530718;

// Below this distance to the centre, the angle is undefined: it is taken as 0.
const float CENTER_EPSILON = 0.000001;

// Smallest antialiasing width, so that no division is by zero.
const float MIN_AA_WIDTH = 0.000001;

// Added to palette positions, so that a position that should be a whole index is not
// rounded down to the previous one.
const float INDEX_EPSILON = 0.0001;

// ---- Palette ---------------------------------------------------------------

// The colour at a whole palette index, cycling through the palette, negative indices included.
vec4 paletteColor(float index)
{
  float size = float(palette_size);
  float wrapped = index - size * floor(index / size);
  return palette[int(clamp(wrapped, 0.0, size - 1.0))];
}

// Weight of the next colour at the fractional position f within a band or cell: 0 until
// the last `width` of it, then rising to 1 at its end. A width of 0 keeps hard edges.
float transition(float f, float width)
{
  if (width <= 0.0)
    return 0.0;
  return clamp((f - 1.0 + width) / width, 0.0, 1.0);
}

// The palette sampled at a position in palette entries: interpolated with the smoothness.
vec4 samplePalette(float position)
{
  float index = floor(position);
  return mix(paletteColor(index), paletteColor(index + 1.0), transition(position - index, smoothness));
}

// The colour of band k: the palette scaled so that it closes around the circle (palette_scale).
vec4 bandColor(float k)
{
  return samplePalette(k * palette_scale + INDEX_EPSILON);
}

// The colour of the cell at angular index i and radial index j.
vec4 cellColor(float i, float j)
{
  return samplePalette(i * palette_scale + j + INDEX_EPSILON);
}

// ---- Colour spaces ---------------------------------------------------------

vec3 rgbToHsl(vec3 c)
{
  float maxC = max(max(c.r, c.g), c.b);
  float minC = min(min(c.r, c.g), c.b);
  float lightness = (maxC + minC) * 0.5;
  float delta = maxC - minC;
  if (delta <= 0.0)
    return vec3(0.0, 0.0, lightness);
  float saturation = delta / (1.0 - abs(2.0 * lightness - 1.0));
  float hue;
  if (maxC == c.r)
    hue = mod((c.g - c.b) / delta, 6.0);
  else if (maxC == c.g)
    hue = (c.b - c.r) / delta + 2.0;
  else
    hue = (c.r - c.g) / delta + 4.0;
  return vec3(hue / 6.0, clamp(saturation, 0.0, 1.0), lightness);
}

vec3 hslToRgb(vec3 hsl)
{
  vec3 rgb = clamp(abs(mod(hsl.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
  return hsl.z + hsl.y * (rgb - 0.5) * (1.0 - abs(2.0 * hsl.z - 1.0));
}

// ---- Main ------------------------------------------------------------------

void main()
{
  // 1. Centred coordinates, the disk inscribed in the image and kept round.
  vec2 p = (isf_FragNormCoord - 0.5) * 2.0;
  p *= RENDERSIZE / min(RENDERSIZE.x, RENDERSIZE.y);

  // 2-3. Polar coordinates, turned by the phase. The angle is undefined at the centre.
  float r = length(p);
  float a = 0.0;
  if (r > CENTER_EPSILON)
    a = fract(atan(p.y, p.x) / TAU - phase);

  float angularDivisions = float(angular_divisions);
  float u = a * angularDivisions;
  float v = r * radial_divisions;

  // Antialiasing widths, in bands, from the size of a pixel. The angle jumps from 1 to 0
  // at the seam: its derivative is computed from the position instead.
  float pixel = length(fwidth(p));
  float du = min(angularDivisions * pixel / (TAU * max(r, CENTER_EPSILON)), 1.0);
  float dv = min(abs(radial_divisions) * fwidth(r), 1.0);

  // 4. Bands: segments, spirals or rings, from the sum of both coordinates.
  float band = u + v;
  float k = floor(band);
  vec4 bands = mix(bandColor(k), bandColor(k + 1.0),
                   transition(band - k, max(smoothness, min(du + dv, 1.0))));

  // 5. Cells: both coordinates divided on their own, interpolated along each axis.
  float i = floor(u);
  float j = floor(v);
  float tu = transition(u - i, max(smoothness, du));
  float tv = transition(v - j, max(smoothness, dv));
  vec4 cells = mix(mix(cellColor(i, j), cellColor(i + 1.0, j), tu),
                   mix(cellColor(i, j + 1.0), cellColor(i + 1.0, j + 1.0), tu), tv);

  // 7. Blend of the two organisations.
  vec4 mixed = mix(bands, cells, grid_blend);

  // 8. Hue rotation; greys have no hue and stay as they are.
  vec3 hsl = rgbToHsl(mixed.rgb);
  hsl.x = fract(hsl.x + hue_shift / 360.0);

  // 9. Radial saturation and lightness, by the distance to the centre.
  float radius = clamp(r, 0.0, 1.0);
  float w = radial_reverse ? radius : 1.0 - radius;
  float lightnessWeight = pow(w, radial_falloff);
  float saturationWeight = pow(radial_saturation < 0.0 ? 1.0 - w : w, radial_falloff);
  hsl.y *= 1.0 - abs(radial_saturation) * saturationWeight;
  hsl.z = mix(hsl.z, step(0.0, radial_lightness), abs(radial_lightness) * lightnessWeight);
  vec3 color = hslToRgb(hsl);

  // 10-11. Transparent outside the disk, with an antialiased edge.
  float edge = max(fwidth(r), MIN_AA_WIDTH);
  float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, r);
  gl_FragColor = vec4(color, mixed.a * alpha);
}
)glsl";

} // namespace

ColorWheelGenerator::ColorWheelGenerator(int id)
  : ShaderGenerator(id),
    _applyingGeometry(false),
    _paletteId(Palette::SPECTRUM),
    _colors(Palette::presetColors(Palette::SPECTRUM)),
    _phase(0),
    _phaseTime(0),
    _phaseRate(0)
{
  SettingGroup* root = settings();
  root->describeDisplayGroup(GEOMETRY_GROUP, tr("Geometry"), GEOMETRY_ORDER, false);
  root->describeDisplayGroup(COLOR_GROUP, tr("Color"), COLOR_ORDER, false);
  root->describeDisplayGroup(RADIAL_COLOR_GROUP, tr("Radial Color"), RADIAL_COLOR_ORDER, false);
  root->describeDisplayGroup(ANIMATION_GROUP, tr("Animation"), ANIMATION_ORDER, false);

  // Geometry.
  root->addSetting(SettingInfo::choice("geometry", tr("Preset"), geometryOptions())
                   .notStored()
                   .displayGroup(GEOMETRY_GROUP)
                   .description(tr("Sets the four geometry settings to a usual pattern; Custom brings back the last ones of your own.")))
      ->bind([this]() { return QVariant(_geometryId); },
             [this](const QVariant& value) { _selectGeometry(value.toString()); return true; });
  root->addSetting(SettingInfo::integer("angular_divisions", tr("Angular Divisions"))
                   .bounds(0, MAX_ANGULAR_DIVISIONS)
                   .defaultValue(DEFAULT_ANGULAR_DIVISIONS)
                   .displayGroup(GEOMETRY_GROUP)
                   .description(tr("Number of subdivisions around the circumference.")));
  root->addSetting(SettingInfo::number("radial_divisions", tr("Radial Divisions"))
                   .bounds(-MAX_RADIAL_DIVISIONS, MAX_RADIAL_DIVISIONS)
                   .step(RADIAL_DIVISIONS_STEP)
                   .defaultValue(0.0)
                   .displayGroup(GEOMETRY_GROUP)
                   .description(tr("Controls concentric subdivisions and spiral curvature. Negative values reverse the spiral orientation.")));
  root->addSetting(SettingInfo::number("grid_blend", tr("Grid Blend"))
                   .bounds(0.0, 1.0)
                   .step(FRACTION_STEP)
                   .defaultValue(0.0)
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_SCALE)
                   .displayGroup(GEOMETRY_GROUP)
                   .description(tr("Blends continuous bands with independently subdivided angular and radial cells.")));
  root->addSetting(SettingInfo::number("smoothness", tr("Smoothness"))
                   .bounds(0.0, 1.0)
                   .step(FRACTION_STEP)
                   .defaultValue(0.0)
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_SCALE)
                   .displayGroup(GEOMETRY_GROUP)
                   .description(tr("Interpolation between neighbouring colours: sharp at 0, continuous at 100%.")));

  // Colour.
  root->addSetting(SettingInfo::choice("palette", tr("Palette"), Palette::presetOptions())
                   .defaultValue(Palette::SPECTRUM)
                   .displayGroup(COLOR_GROUP)
                   .description(tr("The colours of the pattern, in the order of their cycle.")))
      ->bind([this]() { return QVariant(_paletteId); },
             [this](const QVariant& value) { _setPaletteId(value.toString()); return true; });
  root->addSetting(SettingInfo::number("hue_shift", tr("Hue Shift"))
                   .bounds(-MAX_HUE_SHIFT, MAX_HUE_SHIFT)
                   .step(HUE_SHIFT_STEP)
                   .defaultValue(0.0)
                   .unit(SettingInfo::Degrees)
                   .displayGroup(COLOR_GROUP)
                   .description(tr("Turns the hue of every colour; greys stay as they are.")));

  // Radial colour.
  root->addSetting(SettingInfo::number("radial_saturation", tr("Radial Saturation"))
                   .bounds(-1.0, 1.0)
                   .step(FRACTION_STEP)
                   .defaultValue(0.0)
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_SCALE)
                   .displayGroup(RADIAL_COLOR_GROUP)
                   .description(tr("Positive: grey centre; negative: grey circumference. The lightness is kept.")));
  root->addSetting(SettingInfo::number("radial_lightness", tr("Radial Lightness"))
                   .bounds(-1.0, 1.0)
                   .step(FRACTION_STEP)
                   .defaultValue(0.0)
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_SCALE)
                   .displayGroup(RADIAL_COLOR_GROUP)
                   .description(tr("Positive: white centre; negative: black centre.")));
  root->addSetting(SettingInfo::number("radial_falloff", tr("Radial Falloff"))
                   .bounds(MIN_RADIAL_FALLOFF, MAX_RADIAL_FALLOFF)
                   .step(FRACTION_STEP)
                   .defaultValue(DEFAULT_RADIAL_FALLOFF)
                   .displayGroup(RADIAL_COLOR_GROUP)
                   .description(tr("The curve of both radial gradients: broad below 1, linear at 1, concentrated above 1.")));
  root->addSetting(SettingInfo::boolean("radial_reverse", tr("Reverse Radial Gradient"))
                   .defaultValue(false)
                   .displayGroup(RADIAL_COLOR_GROUP)
                   .description(tr("Swaps the centre and the circumference in both radial gradients.")));

  // Animation.
  root->addSetting(SettingInfo::number("speed", tr("Speed"))
                   .bounds(0.0, MAX_SPEED_RPM)
                   .step(SPEED_STEP)
                   .defaultValue(DEFAULT_SPEED_RPM)
                   .unit(SettingInfo::RevolutionsPerMinute)
                   .displayGroup(ANIMATION_GROUP)
                   .description(tr("How fast the pattern turns; 0 keeps it still.")));
  root->addSetting(SettingInfo::boolean("reverse", tr("Reverse"))
                   .defaultValue(false)
                   .displayGroup(ANIMATION_GROUP)
                   .description(tr("Turns the pattern the other way.")));

  for (const char* id : { "angular_divisions", "radial_divisions", "grid_blend", "smoothness",
                          "hue_shift", "radial_saturation", "radial_lightness",
                          "radial_falloff", "radial_reverse" })
    _addShaderInput(QString::fromLatin1(id));

  _phaseRate = _turnsPerSecond();
  _followGeometry();

  connect(this, &Element::settingChanged, this, [this](uid, const QString& path) {
    // The preset shown follows the geometry, however it was changed, except while a
    // preset or the custom geometry is applied.
    for (const char* id : GEOMETRY_SETTINGS)
      if (path == QLatin1String(id) && !_applyingGeometry)
      {
        _followGeometry();
        settings()->find("geometry")->notifyChanged();
      }
    // The angle turned so far is kept when the speed or the direction changes.
    if (path == QLatin1String("speed") || path == QLatin1String("reverse"))
      _advancePhase();
  });
}

bool ColorWheelGenerator::isAnimated() const
{
  return settings()->find("speed")->effectiveValue().toDouble() > 0;
}

QString ColorWheelGenerator::geometryPreset() const
{
  const int angular = settings()->find("angular_divisions")->value().toInt();
  const double radial = settings()->find("radial_divisions")->value().toDouble();
  const double gridBlend = settings()->find("grid_blend")->value().toDouble();
  const double smoothness = settings()->find("smoothness")->value().toDouble();
  for (const GeometryPreset& preset : GEOMETRY_PRESETS)
    if (preset.angularDivisions == angular &&
        std::fabs(preset.radialDivisions - radial) < PRESET_TOLERANCE &&
        std::fabs(preset.gridBlend - gridBlend) < PRESET_TOLERANCE &&
        std::fabs(preset.smoothness - smoothness) < PRESET_TOLERANCE)
      return QString::fromLatin1(preset.id);
  return CUSTOM_GEOMETRY;
}

QVector<double> ColorWheelGenerator::_geometryValues() const
{
  QVector<double> values;
  for (const char* id : GEOMETRY_SETTINGS)
    values.append(settings()->find(id)->value().toDouble());
  return values;
}

void ColorWheelGenerator::_setGeometryValues(const QVector<double>& values)
{
  _applyingGeometry = true;
  int index = 0;
  for (const char* id : GEOMETRY_SETTINGS)
    settings()->find(id)->setValue(values.value(index++));
  _applyingGeometry = false;
}

void ColorWheelGenerator::_selectGeometry(const QString& id)
{
  if (id == CUSTOM_GEOMETRY)
  {
    // The geometry of the user's own, if any; otherwise the current one, to change.
    if (!_customGeometry.isEmpty())
      _setGeometryValues(_customGeometry);
    _geometryId = CUSTOM_GEOMETRY;
    return;
  }
  for (const GeometryPreset& preset : GEOMETRY_PRESETS)
  {
    if (id != QLatin1String(preset.id))
      continue;
    _setGeometryValues({ double(preset.angularDivisions), preset.radialDivisions,
                         preset.gridBlend, preset.smoothness });
    _geometryId = id;
    return;
  }
}

void ColorWheelGenerator::_followGeometry()
{
  _geometryId = geometryPreset();
  if (_geometryId == CUSTOM_GEOMETRY)
    _customGeometry = _geometryValues();
}

void ColorWheelGenerator::read(const QJsonObject& obj)
{
  ShaderGenerator::read(obj);
  // The geometry read is the user's own only when it matches no preset.
  _customGeometry.clear();
  _followGeometry();
}

void ColorWheelGenerator::_setPaletteId(const QString& id)
{
  _paletteId = id;
  const QVector<QColor> colors = Palette::presetColors(id);
  if (!colors.isEmpty())
    _colors = colors;
}

double ColorWheelGenerator::_turnsPerSecond() const
{
  const double speed = settings()->find("speed")->effectiveValue().toDouble();
  const bool reverse = settings()->find("reverse")->effectiveValue().toBool();
  return speed / SECONDS_PER_MINUTE * (reverse ? -1.0 : 1.0);
}

void ColorWheelGenerator::_advancePhase()
{
  const double now = time();
  _phase += (now - _phaseTime) * _phaseRate;
  _phase -= std::floor(_phase);
  _phaseTime = now;
  _phaseRate = _turnsPerSecond();
}

void ColorWheelGenerator::rewind()
{
  ShaderGenerator::rewind();
  _phase = 0;
  _phaseTime = 0;
  _phaseRate = _turnsPerSecond();
}

double ColorWheelGenerator::paletteScale(int angularDivisions, int paletteSize)
{
  if (angularDivisions <= 0 || paletteSize <= 0)
    return 1.0;
  // Rounded up, so that each division advances by one entry at least: neighbours never
  // repeat a colour (but with two colours on an odd number of divisions).
  const int cycles = qMax(1, int(std::ceil(double(angularDivisions) / paletteSize)));
  return double(paletteSize * cycles) / angularDivisions;
}

QString ColorWheelGenerator::_extraUniforms() const
{
  return QStringLiteral("uniform float phase;\n"
                        "uniform vec4 palette[%1];\n"
                        "uniform int palette_size;\n"
                        "uniform float palette_scale;\n").arg(Palette::MAX_SIZE);
}

void ColorWheelGenerator::_setExtraUniforms(QOpenGLShaderProgram* program)
{
  _advancePhase();
  program->setUniformValue("phase", GLfloat(_phase));

  QVector<QVector4D> colors;
  for (const QColor& color : std::as_const(_colors))
    colors.append(QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
  program->setUniformValueArray("palette", colors.constData(), int(colors.size()));
  program->setUniformValue("palette_size", GLint(colors.size()));
  program->setUniformValue("palette_scale", GLfloat(paletteScale(
    settings()->find("angular_divisions")->effectiveValue().toInt(), int(colors.size()))));
}

QString ColorWheelGenerator::_fragmentShader() const
{
  return QString::fromLatin1(COLOR_WHEEL_SHADER);
}

QIcon ColorWheelGenerator::getIcon() const
{
  QPixmap pixmap = iconPixmap(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  QConicalGradient gradient(pixmap.rect().center(), 0);
  for (int i = 0; i <= _colors.size(); i++)
    gradient.setColorAt(qreal(i) / qMax(1, int(_colors.size())), _colors.value(i % qMax(1, int(_colors.size()))));
  painter.setPen(Qt::NoPen);
  painter.setBrush(gradient);
  painter.drawEllipse(pixmap.rect());
  return QIcon(pixmap);
}

ColorGenerator::ColorGenerator(int id)
  : ShaderGenerator(id)
{
  settings()->addSetting(SettingInfo::color("color", tr("Color"))
                         .defaultValue(DEFAULT_GENERATOR_COLOR)
                         .description(tr("The colour of the whole image, transparency included.")));
  _addShaderInput("color");
}

ColorGenerator::ColorGenerator(const QColor& color, uid id)
  : ColorGenerator(int(id))
{
  settings()->find("color")->setValue(color);
}

QColor ColorGenerator::color() const
{
  return colorSetting(this, "color");
}

QString ColorGenerator::_fragmentShader() const
{
  return QStringLiteral(
    "void main() {\n"
    "  gl_FragColor = color;\n"
    "}\n");
}

QIcon ColorGenerator::getIcon() const
{
  return QIcon(iconPixmap(color()));
}

} // namespace mmp
