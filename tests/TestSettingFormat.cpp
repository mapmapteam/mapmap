/*
 * TestSettingFormat.cpp
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

#include "TestSettingFormat.h"

#include <QtTest>

#include "SettingFormat.h"

using namespace mmp;

namespace {

/// The no-break space written before most units.
const QString NBSP = QString(QChar(0x00A0));

const QLocale FRENCH(QLocale::French, QLocale::Canada);
const QLocale ENGLISH(QLocale::English, QLocale::UnitedStates);

/// An opacity as declared by Element: 0..1 shown in %, no step.
SettingInfo opacity()
{
  return SettingInfo::number("opacity", "Opacity").bounds(0.0, 1.0)
      .unit(SettingInfo::Percent).displayScale(100.0);
}

} // namespace

void TestSettingFormat::decimalsFollowStep()
{
  QCOMPARE(SettingFormat::decimals(opacity()), 0);
  QCOMPARE(SettingFormat::decimals(SettingInfo::number("f", "F").step(0.1)), 1);
  QCOMPARE(SettingFormat::decimals(SettingInfo::number("f", "F").step(0.01)), 2);
  QCOMPARE(SettingFormat::decimals(SettingInfo::number("f", "F").step(5)), 0);
  QCOMPARE(SettingFormat::decimals(SettingInfo::integer("n", "N").step(0.5)), 0);
  // A 0..1 value shown in % with a step of 0.001 shows one decimal: 0.1 %.
  QCOMPARE(SettingFormat::decimals(opacity().step(0.001)), 1);
  // An open number without a unit shows tenths.
  QCOMPARE(SettingFormat::decimals(SettingInfo::number("x", "X")), 1);
}

void TestSettingFormat::unitsInFrenchAndEnglish()
{
  QCOMPARE(SettingFormat::formatNumber(70, opacity(), FRENCH), QString("70") + NBSP + "%");
  QCOMPARE(SettingFormat::formatNumber(70, opacity(), ENGLISH), QString("70%"));

  const SettingInfo angle = SettingInfo::number("a", "A").step(1).unit(SettingInfo::Degrees);
  QCOMPARE(SettingFormat::formatNumber(12, angle, FRENCH), QString("12") + QChar(0x00B0));
  QCOMPARE(SettingFormat::formatNumber(12, angle, ENGLISH), QString("12") + QChar(0x00B0));

  const SettingInfo frequency = SettingInfo::number("f", "F").step(0.1).unit(SettingInfo::Hertz);
  QCOMPARE(SettingFormat::formatNumber(2.4, frequency, FRENCH), QString("2,4") + NBSP + "Hz");
  QCOMPARE(SettingFormat::formatNumber(2.4, frequency, ENGLISH), QString("2.4") + NBSP + "Hz");

  const SettingInfo rotation = SettingInfo::number("r", "R").step(0.1).unit(SettingInfo::RevolutionsPerMinute);
  QCOMPARE(SettingFormat::formatNumber(5, rotation, ENGLISH), QString("5.0") + NBSP + "rpm");
  double parsed = 0;
  QVERIFY(SettingFormat::parseNumber("12 rpm", rotation, &parsed, ENGLISH));
  QCOMPARE(parsed, 12.0);

  // Pixels are shown without their unit.
  const SettingInfo pixels = SettingInfo::point("p", "P").unit(SettingInfo::Pixels);
  QCOMPARE(SettingFormat::formatNumber(120, pixels, ENGLISH), QString("120"));
}

void TestSettingFormat::noThousandsSeparator()
{
  const SettingInfo size = SettingInfo::integer("w", "W").unit(SettingInfo::Pixels);
  QCOMPARE(SettingFormat::formatNumber(1024, size, ENGLISH), QString("1024"));
  QCOMPARE(SettingFormat::formatNumber(1024, size, FRENCH), QString("1024"));
  const SettingInfo plain = SettingInfo::number("n", "N").step(0.1);
  QCOMPARE(SettingFormat::formatNumber(1000000.5, plain, ENGLISH), QString("1000000.5"));
  QCOMPARE(SettingFormat::formatNumber(1000000.5, plain, FRENCH), QString("1000000,5"));

  // What is shown is read back.
  double value = 0;
  QVERIFY(SettingFormat::parseNumber(SettingFormat::formatNumber(1024, size, ENGLISH), size, &value, ENGLISH));
  QCOMPARE(value, 1024.0);
}

void TestSettingFormat::durationsInMinutesAndSeconds()
{
  const SettingInfo whole = SettingInfo::number("t", "T").step(1).unit(SettingInfo::Seconds);
  QCOMPARE(SettingFormat::formatNumber(65, whole, FRENCH), QString("01:05"));
  QCOMPARE(SettingFormat::formatNumber(4, whole, FRENCH), QString("00:04"));

  const SettingInfo tenths = SettingInfo::number("t", "T").step(0.1).unit(SettingInfo::Seconds);
  QCOMPARE(SettingFormat::formatNumber(3.2, tenths, FRENCH), QString("00:03,2"));
  QCOMPARE(SettingFormat::formatNumber(3.2, tenths, ENGLISH), QString("00:03.2"));
}

void TestSettingFormat::noNegativeZero()
{
  const SettingInfo angle = SettingInfo::number("a", "A").step(1).unit(SettingInfo::Degrees);
  QCOMPARE(SettingFormat::formatNumber(-0.2, angle, ENGLISH), QString("0") + QChar(0x00B0));
}

void TestSettingFormat::parsesCommaAndPoint()
{
  double value = 0;
  QVERIFY(SettingFormat::parseNumber("33,5", opacity(), &value, FRENCH));
  QCOMPARE(value, 33.5);
  QVERIFY(SettingFormat::parseNumber("33.5", opacity(), &value, FRENCH));
  QCOMPARE(value, 33.5);
  QVERIFY(SettingFormat::parseNumber("33,5", opacity(), &value, ENGLISH));
  QCOMPARE(value, 33.5);
  QVERIFY(SettingFormat::parseNumber("-12", opacity(), &value, ENGLISH));
  QCOMPARE(value, -12.0);
}

void TestSettingFormat::parsesWithOrWithoutUnit()
{
  double value = 0;
  QVERIFY(SettingFormat::parseNumber("70 %", opacity(), &value, FRENCH));
  QCOMPARE(value, 70.0);
  QVERIFY(SettingFormat::parseNumber(QString("70") + NBSP + "%", opacity(), &value, FRENCH));
  QCOMPARE(value, 70.0);
  QVERIFY(SettingFormat::parseNumber("70%", opacity(), &value, ENGLISH));
  QCOMPARE(value, 70.0);

  const SettingInfo frequency = SettingInfo::number("f", "F").step(0.1).unit(SettingInfo::Hertz);
  QVERIFY(SettingFormat::parseNumber("2,4 hz", frequency, &value, FRENCH));
  QCOMPARE(value, 2.4);
}

void TestSettingFormat::parsesDurations()
{
  const SettingInfo time = SettingInfo::number("t", "T").step(1).unit(SettingInfo::Seconds);
  double value = 0;
  QVERIFY(SettingFormat::parseNumber("1:05", time, &value, FRENCH));
  QCOMPARE(value, 65.0);
  QVERIFY(SettingFormat::parseNumber("90", time, &value, FRENCH));
  QCOMPARE(value, 90.0);
  QVERIFY(SettingFormat::parseNumber("00:03,5", time, &value, FRENCH));
  QCOMPARE(value, 3.5);
  QVERIFY(!SettingFormat::parseNumber("1:2:3", time, &value, FRENCH));
}

void TestSettingFormat::refusesText()
{
  double value = 42;
  QVERIFY(!SettingFormat::parseNumber("", opacity(), &value, FRENCH));
  QVERIFY(!SettingFormat::parseNumber("abc", opacity(), &value, FRENCH));
  QVERIFY(!SettingFormat::parseNumber("12abc", opacity(), &value, FRENCH));
  QCOMPARE(value, 42.0);
}
