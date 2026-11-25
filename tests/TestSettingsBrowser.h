/*
 * TestSettingsBrowser.h
 *
 * Unit tests for the settings panel made from a settings tree.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_SETTINGS_BROWSER_H_
#define TEST_SETTINGS_BROWSER_H_

#include <QtTest/QtTest>

class TestSettingsBrowser: public QObject
{
  Q_OBJECT

private slots:
  void colorSourceRows();
  void editWritesTreeOnce();
  void refreshDoesNotRewriteRow();
  void unavailableRowIsDisabled();
  void choiceKeepsMissingId();
  void layerRowsAndFolds();
  void rebuildFollowsShrinkingList();
  void editThatShrinksListRebuildsLater();
  void openFoldStaysOpen();
  void rebuildFromEditedWaitsForEditToEnd();
  void mediaSourceRows();
  void filePathRowHasFilter();
  void integerRowHasBounds();
  void booleanRowWritesTree();
  void pointRowWritesTree();
  void referenceRowWritesSourceId();
  void sourceMenuFollowsSourceList();
  void rangeRowScalesLikeNumber();
  void textureMeshPanelKeepsGridsInStep();
};

#endif /* TEST_SETTINGS_BROWSER_H_ */
