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

#include <QConicalGradient>
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

/// Default speed of a colour wheel, and the end of its slider, in turns per second.
const double DEFAULT_TURNS_PER_SECOND = 0.1;
const double TURNS_SLIDER_MAX = 2.0;

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

ColorWheelGenerator::ColorWheelGenerator(int id)
  : ShaderGenerator(id),
    _turns(0),
    _turnsTime(0)
{
  SettingGroup* root = settings();
  root->addSetting(SettingInfo::number("speed", tr("Speed"))
                   .bounds(0, QVariant())
                   .sliderRange(0, TURNS_SLIDER_MAX)
                   .defaultValue(DEFAULT_TURNS_PER_SECOND)
                   .unit(SettingInfo::Hertz)
                   .description(tr("The number of turns the wheel makes each second.")));
  root->addSetting(SettingInfo::boolean("reverse", tr("Reverse"))
                   .defaultValue(false)
                   .description(tr("Turns the wheel counterclockwise instead of clockwise.")));
}

QString ColorWheelGenerator::_extraUniforms() const
{
  return QStringLiteral("uniform float turns;\n");
}

void ColorWheelGenerator::_setExtraUniforms(QOpenGLShaderProgram* program)
{
  // The turns are counted here rather than computed from the time, so that a change
  // of speed does not make the wheel jump.
  const double now = time();
  const Setting* speed = settings()->find("speed");
  const Setting* reverse = settings()->find("reverse");
  const double direction = reverse->effectiveValue().toBool() ? -1.0 : 1.0;
  _turns += (now - _turnsTime) * speed->effectiveValue().toDouble() * direction;
  _turns -= std::floor(_turns);
  _turnsTime = now;
  program->setUniformValue("turns", GLfloat(_turns));
}

void ColorWheelGenerator::rewind()
{
  ShaderGenerator::rewind();
  _turns = 0;
  _turnsTime = 0;
}

QString ColorWheelGenerator::_fragmentShader() const
{
  // The hue grows with the angle; adding the turns moves each hue to a smaller
  // angle, so that the wheel turns clockwise.
  return QStringLiteral(
    "vec3 hsvToRgb(vec3 c) {\n"
    "  vec3 p = abs(fract(c.xxx + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);\n"
    "  return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);\n"
    "}\n"
    "void main() {\n"
    "  vec2 offset = (isf_FragNormCoord - 0.5) * vec2(RENDERSIZE.x / RENDERSIZE.y, 1.0);\n"
    "  float hue = fract(atan(offset.y, offset.x) / 6.28318531 + turns);\n"
    "  float saturation = clamp(length(offset) * 2.0, 0.0, 1.0);\n"
    "  gl_FragColor = vec4(hsvToRgb(vec3(hue, saturation, 1.0)), 1.0);\n"
    "}\n");
}

QIcon ColorWheelGenerator::getIcon() const
{
  QPixmap pixmap = iconPixmap(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  QConicalGradient gradient(pixmap.rect().center(), 0);
  const QColor hues[] = { Qt::red, Qt::magenta, Qt::blue, Qt::cyan, Qt::green, Qt::yellow, Qt::red };
  const int nHues = int(sizeof(hues) / sizeof(hues[0]));
  for (int i = 0; i < nHues; i++)
    gradient.setColorAt(qreal(i) / (nHues - 1), hues[i]);
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
