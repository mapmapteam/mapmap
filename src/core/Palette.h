/*
 * Palette.h
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


#ifndef PALETTE_H_
#define PALETTE_H_

#include <QColor>
#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QVector>

#include "Setting.h"

namespace mmp {

/**
 * An ordered, cyclic collection of colours, as generators sample them: after the
 * last colour comes the first. Presets make the usual palettes.
 */
class Palette
{
  Q_DECLARE_TR_FUNCTIONS(Palette)

public:
  /// Most colours in a palette: the number a shader receives.
  static const int MAX_SIZE = 16;

  /// Id of the default palette: the hues of the spectrum.
  static const QString SPECTRUM;

  /// Returns the presets as the options of a Choice setting.
  static QList<SettingOption> presetOptions();

  /// Returns the colours of a preset; an empty list for an unknown id.
  static QVector<QColor> presetColors(const QString& id);
};

} // namespace mmp

#endif /* PALETTE_H_ */
