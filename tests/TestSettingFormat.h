/*
 * TestSettingFormat.h
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

#ifndef TEST_SETTING_FORMAT_H_
#define TEST_SETTING_FORMAT_H_

#include <QObject>

/// Tests how the inspector writes and reads numbers.
class TestSettingFormat : public QObject
{
  Q_OBJECT

private slots:
  void decimalsFollowStep();
  void unitsInFrenchAndEnglish();
  void noThousandsSeparator();
  void durationsInMinutesAndSeconds();
  void noNegativeZero();
  void parsesCommaAndPoint();
  void parsesWithOrWithoutUnit();
  void parsesDurations();
  void refusesText();
};

#endif /* TEST_SETTING_FORMAT_H_ */
