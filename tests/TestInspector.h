/*
 * TestInspector.h
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

#ifndef TEST_INSPECTOR_H_
#define TEST_INSPECTOR_H_

#include <QLocale>
#include <QObject>

/// Tests the inspector: its rows, its controls and their gestures.
class TestInspector : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();

  void controlForEachType();
  void colorSourceRows();
  void arrowsMoveByStepsShiftByTen();
  void openBoundHasSliderRange();
  void controlsUseMapMapColors();
  void typedCommaIsAccepted();
  void refusedTextShowsValueAgain();
  void labelDragAdjustsShiftIsFine();
  void doubleClickResets();
  void readOnlyRowIsDisabled();
  void switchToggles();
  void shortChoiceShowsButtons();
  void choiceKeepsMissingId();
  void referenceMenuFollowsSources();
  void rangeBoundsDoNotCross();
  void pointFieldsWriteCoordinates();
  void externalValueDoesNotRebuild();
  void structureChangeRebuildsLater();
  void foldedGroupStaysFolded();
  void layerGroupsAndSharedRow();
  void liveValueShowsMark();
  void colorCodeIsWrittenAndRemembered();
  void pickerKeepsHueOfGrey();
  void noHorizontalScrollAtReferenceWidth();
  void rowShowsItsKey();

private:
  QLocale _previousLocale;
};

#endif /* TEST_INSPECTOR_H_ */
