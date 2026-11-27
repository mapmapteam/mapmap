/*
 * SettingFormat.h
 *
 * (c) 2026 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef SETTING_FORMAT_H_
#define SETTING_FORMAT_H_

#include <QLocale>
#include <QString>

#include "Setting.h"

namespace mmp {

/**
 * Writes and reads the numbers of the inspector as the user sees them: "70 %", "12°",
 * "2,4 Hz", "01:05". Every function works on display values, the value of the setting
 * multiplied by its display scale (see Setting::displayValue()).
 *
 * The number of decimals follows the step of the setting: none for a step of 1 or more,
 * one for a step of 0.1 or more, two below.
 */
class SettingFormat {
public:
  /// Returns the step of the interface in display units: the step of the setting times its
  /// display scale, or a sensible step when the setting gives none.
  static double displayStep(const SettingInfo& info);

  /// Returns the number of decimals shown for the setting.
  static int decimals(const SettingInfo& info);

  /// Returns the text of the unit as written after a number in this locale, with its
  /// leading space when the locale wants one: " %" in French, "%" in English.
  static QString unitSuffix(SettingInfo::Unit unit, const QLocale& locale = QLocale());

  /// Returns a display value as text, with its unit: "70 %", "12°", "2,4 Hz", "120 px".
  /// A duration in seconds is written as minutes and seconds: "01:05".
  static QString formatNumber(double displayValue, const SettingInfo& info,
                              const QLocale& locale = QLocale());

  /// Returns a number of seconds as minutes and seconds, "mm:ss", followed by the decimals
  /// asked for: "00:03,2".
  static QString formatTime(double seconds, int decimals, const QLocale& locale = QLocale());

  /// Reads a display value typed by the user. The unit may be typed or not, a comma or a
  /// point may separate the decimals, and a duration may be "1:05" or a number of seconds.
  /// Returns false, leaving displayValue alone, when the text is not a number.
  static bool parseNumber(const QString& text, const SettingInfo& info, double* displayValue,
                          const QLocale& locale = QLocale());
};

} // namespace mmp

#endif /* SETTING_FORMAT_H_ */
