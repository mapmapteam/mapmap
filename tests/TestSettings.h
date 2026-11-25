/*
 * TestSettings.h
 *
 * Unit tests for the settings of the model: their bounds, and the registry
 * that lists them.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_SETTINGS_H_
#define TEST_SETTINGS_H_

#include <QtTest/QtTest>

class TestSettings: public QObject
{
  Q_OBJECT

private slots:
  void opacityIsClamped();
  void settingClampsToBounds();
  void rejectsNonFinite();
  void stepIsNotStrictByDefault();
  void integerIsRounded();
  void rangeNeverCrosses();
  void choiceKeepsMissingId();
  void readOnlyRefusesWrites();
  void resetRestoresDefault();
  void boundSettingUsesOwnerStorage();
  void pathFollowsStructure();
  void notifiesTopmostListener();
  void listFollowsCount();
  void findRejectsBadListIndex();
  void mountedGroupIsNotOwned();
};

#endif /* TEST_SETTINGS_H_ */
