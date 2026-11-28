/*
 * Palette.cpp
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


#include "Palette.h"

#include <cmath>

namespace mmp {

const QString Palette::SPECTRUM = QStringLiteral("spectrum");

namespace {

/// Number of hues of the spectrum preset.
const int SPECTRUM_SIZE = 12;

/// Number of shades of the monochromatic and grayscale presets.
const int SHADES_SIZE = 6;

/// Saturation and lightness of the colours of the spectrum.
const double FULL_SATURATION = 1.0;
const double SPECTRUM_LIGHTNESS = 0.5;

/// Darkest and lightest shades of the monochromatic preset.
const double DARKEST_SHADE = 0.2;
const double LIGHTEST_SHADE = 0.8;

/// Hues of the presets, in degrees. Hue shift turns them: red is only where they start.
const double RED = 0.0;
const double DEGREES_PER_TURN = 360.0;
const double COMPLEMENT = 180.0;
const double ANALOGOUS_STEP = 30.0;
const double TRIADIC_STEP = 120.0;

/// Returns the colour of a hue in degrees, with a saturation and a lightness between 0 and 1.
QColor hsl(double hueDegrees, double saturation, double lightness)
{
  const double turns = hueDegrees / DEGREES_PER_TURN;
  // In RGB, as the colours read from project files.
  return QColor::fromHslF(float(turns - std::floor(turns)), float(saturation), float(lightness)).toRgb();
}

/// Returns count lightnesses that go from dark to light and back, between low and high,
/// so that the cycle of the palette closes without a jump.
QVector<double> shades(int count, double low, double high)
{
  QVector<double> values;
  for (int i = 0; i < count; i++)
  {
    const double position = 1.0 - std::fabs(2.0 * i / count - 1.0);
    values.append(low + (high - low) * position);
  }
  return values;
}

} // namespace

QList<SettingOption> Palette::presetOptions()
{
  return {
    { SPECTRUM,                       tr("Spectrum") },
    { QStringLiteral("monochromatic"),   tr("Monochromatic") },
    { QStringLiteral("grayscale"),       tr("Grayscale") },
    { QStringLiteral("black_and_white"), tr("Black and White") },
    { QStringLiteral("complementary"),   tr("Complementary") },
    { QStringLiteral("analogous"),       tr("Analogous") },
    { QStringLiteral("triadic"),         tr("Triadic") },
  };
}

QVector<QColor> Palette::presetColors(const QString& id)
{
  QVector<QColor> colors;
  if (id == SPECTRUM)
  {
    for (int i = 0; i < SPECTRUM_SIZE; i++)
      colors.append(hsl(DEGREES_PER_TURN * i / SPECTRUM_SIZE, FULL_SATURATION, SPECTRUM_LIGHTNESS));
  }
  else if (id == QLatin1String("monochromatic"))
  {
    for (double lightness : shades(SHADES_SIZE, DARKEST_SHADE, LIGHTEST_SHADE))
      colors.append(hsl(RED, FULL_SATURATION, lightness));
  }
  else if (id == QLatin1String("grayscale"))
  {
    for (double lightness : shades(SHADES_SIZE, 0.0, 1.0))
      colors.append(QColor::fromRgbF(float(lightness), float(lightness), float(lightness)));
  }
  else if (id == QLatin1String("black_and_white"))
    colors = { QColor(Qt::white), QColor(Qt::black) };
  else if (id == QLatin1String("complementary"))
    colors = { hsl(RED, FULL_SATURATION, SPECTRUM_LIGHTNESS), hsl(RED + COMPLEMENT, FULL_SATURATION, SPECTRUM_LIGHTNESS) };
  else if (id == QLatin1String("analogous"))
    colors = { hsl(RED - ANALOGOUS_STEP, FULL_SATURATION, SPECTRUM_LIGHTNESS),
               hsl(RED, FULL_SATURATION, SPECTRUM_LIGHTNESS),
               hsl(RED + ANALOGOUS_STEP, FULL_SATURATION, SPECTRUM_LIGHTNESS) };
  else if (id == QLatin1String("triadic"))
    colors = { hsl(RED, FULL_SATURATION, SPECTRUM_LIGHTNESS),
               hsl(RED + TRIADIC_STEP, FULL_SATURATION, SPECTRUM_LIGHTNESS),
               hsl(RED + 2 * TRIADIC_STEP, FULL_SATURATION, SPECTRUM_LIGHTNESS) };
  return colors;
}

} // namespace mmp
