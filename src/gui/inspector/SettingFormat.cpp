/*
 * SettingFormat.cpp
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

#include "SettingFormat.h"

#include <cmath>

#include <QRegularExpression>
#include <QStringList>

namespace mmp {

namespace {

/// A step of this size or more shows no decimal.
const double WHOLE_STEP = 1.0;

/// A step of this size or more, and below WHOLE_STEP, shows one decimal.
const double TENTH_STEP = 0.1;

/// Decimals shown for a step below TENTH_STEP.
const int FINE_DECIMALS = 2;

/// Number of steps between the bounds when a bounded number gives no step.
const double STEPS_BETWEEN_BOUNDS = 100.0;

/// Seconds in a minute.
const int SECONDS_PER_MINUTE = 60;

/// Width of the minutes and of the seconds in "mm:ss".
const int TIME_FIELD_WIDTH = 2;

/// No-break space, written between a number and most units.
const QChar NO_BREAK_SPACE(0x00A0);

/// Returns the locale without its thousands separators: "1024", not "1,024".
QLocale withoutGroupSeparator(const QLocale& locale)
{
  QLocale plain(locale);
  plain.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
  return plain;
}

/// Returns true for a Number or an Integer.
bool isNumber(const SettingInfo& info)
{
  return info.type() == SettingInfo::Number || info.type() == SettingInfo::Integer;
}

/// Returns the symbol of a unit, without any space.
QString unitSymbol(SettingInfo::Unit unit)
{
  switch (unit) {
  case SettingInfo::Percent: return QStringLiteral("%");
  case SettingInfo::Degrees: return QString(QChar(0x00B0));
  case SettingInfo::Hertz:   return QStringLiteral("Hz");
  case SettingInfo::Seconds: return QStringLiteral("s");
  case SettingInfo::Pixels:  return QStringLiteral("px");
  case SettingInfo::RevolutionsPerMinute: return QStringLiteral("rpm");
  case SettingInfo::NoUnit:  break;
  }
  return QString();
}

} // namespace

double SettingFormat::displayStep(const SettingInfo& info)
{
  const double scale = info.displayScale();
  if (info.step() > 0)
    return info.step() * scale;

  if (info.type() == SettingInfo::Integer)
    return WHOLE_STEP;

  bool hasMinimum = false;
  bool hasMaximum = false;
  const double minimum = info.minimum().toDouble(&hasMinimum);
  const double maximum = info.maximum().toDouble(&hasMaximum);
  if (hasMinimum && hasMaximum && info.minimum().isValid() && info.maximum().isValid()
      && maximum > minimum) {
    // Round the step down to a power of ten, so that it gives a whole number of decimals.
    const double raw = (maximum - minimum) * scale / STEPS_BETWEEN_BOUNDS;
    return std::pow(10.0, std::floor(std::log10(raw)));
  }

  // An open number: whole units for units that are usually whole, tenths otherwise.
  switch (info.unit()) {
  case SettingInfo::Percent:
  case SettingInfo::Degrees:
  case SettingInfo::Pixels:
  case SettingInfo::Seconds:
    return WHOLE_STEP;
  default:
    return TENTH_STEP;
  }
}

int SettingFormat::decimals(const SettingInfo& info)
{
  if (info.type() == SettingInfo::Integer)
    return 0;
  const double step = displayStep(info);
  if (step >= WHOLE_STEP)
    return 0;
  if (step >= TENTH_STEP)
    return 1;
  return FINE_DECIMALS;
}

QString SettingFormat::unitSuffix(SettingInfo::Unit unit, const QLocale& locale)
{
  // Positions and sizes are in pixels all over MapMap: no need to say it.
  if (unit == SettingInfo::Pixels)
    return QString();
  const QString symbol = unitSymbol(unit);
  if (symbol.isEmpty())
    return symbol;
  // Degrees stick to the number in every language; a percent sign too, except in French.
  if (unit == SettingInfo::Degrees)
    return symbol;
  if (unit == SettingInfo::Percent && locale.language() != QLocale::French)
    return symbol;
  return NO_BREAK_SPACE + symbol;
}

QString SettingFormat::formatNumber(double displayValue, const SettingInfo& info,
                                    const QLocale& anyLocale)
{
  const QLocale locale = withoutGroupSeparator(anyLocale);
  const int places = decimals(info);
  if (info.unit() == SettingInfo::Seconds)
    return formatTime(displayValue, places, locale);

  QString text = locale.toString(displayValue, 'f', places);
  // Never show "-0".
  if (text.startsWith(locale.negativeSign())) {
    const QString zero = locale.toString(0.0, 'f', places);
    if (text.mid(locale.negativeSign().size()) == zero)
      text = zero;
  }
  return text + unitSuffix(info.unit(), locale);
}

QString SettingFormat::formatTime(double seconds, int decimals, const QLocale& anyLocale)
{
  const QLocale locale = withoutGroupSeparator(anyLocale);
  const bool negative = seconds < 0;
  const double magnitude = std::fabs(seconds);
  const int minutes = int(magnitude) / SECONDS_PER_MINUTE;
  const double rest = magnitude - minutes * SECONDS_PER_MINUTE;

  const QString minuteText = QStringLiteral("%1").arg(minutes, TIME_FIELD_WIDTH, 10, QChar('0'));
  QString secondText = locale.toString(rest, 'f', decimals);
  const int wholeWidth = decimals > 0 ? TIME_FIELD_WIDTH + 1 + decimals : TIME_FIELD_WIDTH;
  while (secondText.size() < wholeWidth)
    secondText.prepend(QChar('0'));
  return (negative ? locale.negativeSign() : QString()) + minuteText + QChar(':') + secondText;
}

bool SettingFormat::parseNumber(const QString& text, const SettingInfo& info, double* displayValue,
                                const QLocale& locale)
{
  QString clean = text.trimmed();
  if (clean.isEmpty())
    return false;

  // Take off the unit and the spaces, whichever way they were typed.
  const QString symbol = unitSymbol(info.unit());
  if (!symbol.isEmpty() && clean.endsWith(symbol, Qt::CaseInsensitive))
    clean.chop(symbol.size());
  clean.remove(QRegularExpression(QStringLiteral("\\s")));
  clean.remove(NO_BREAK_SPACE);
  // A comma or a point always separates the decimals, even in a language that groups
  // thousands with one of them: nobody types a thousands separator in a small field.
  const QString group = locale.groupSeparator();
  if (group != QStringLiteral(",") && group != QStringLiteral("."))
    clean.remove(group);
  // Accept the decimal separator of the locale, a comma and a point.
  clean.replace(locale.decimalPoint(), QStringLiteral("."));
  clean.replace(QChar(','), QChar('.'));
  clean.replace(locale.negativeSign(), QStringLiteral("-"));

  bool ok = false;
  double value = 0.0;
  if (clean.contains(QChar(':'))) {
    // Minutes and seconds.
    const QStringList parts = clean.split(QChar(':'));
    if (parts.size() != 2)
      return false;
    bool minutesOk = false;
    bool secondsOk = false;
    const double minutes = parts.at(0).toDouble(&minutesOk);
    const double seconds = parts.at(1).toDouble(&secondsOk);
    if (!minutesOk || !secondsOk || seconds < 0)
      return false;
    const double sign = parts.at(0).startsWith(QChar('-')) ? -1.0 : 1.0;
    value = minutes * SECONDS_PER_MINUTE + sign * seconds;
    ok = true;
  } else {
    value = clean.toDouble(&ok);
  }
  if (!ok || !std::isfinite(value))
    return false;

  if (!isNumber(info) && info.type() != SettingInfo::Point && info.type() != SettingInfo::Range)
    return false;
  *displayValue = value;
  return true;
}

} // namespace mmp
