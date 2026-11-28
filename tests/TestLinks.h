/*
 * TestLinks.h
 *
 * Unit tests for the links between triggers and settings (src/core/Link).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_LINKS_H_
#define TEST_LINKS_H_

#include <QtTest/QtTest>

class TestLinks: public QObject
{
  Q_OBJECT

private slots:
  void keyTriggerIsPortable();
  void triggerDrivesOneSetting();
  void settingHasOneTriggerPerKind();
  void settingHasOneTriggerPerAction();
  void refusesInvalidLinks();
  void removesLinks();
  void writesAndReadsLinks();
  void skipsUnreadableLinks();
  void toggleSwitchesBoolean();
  void toggleGoesToNextOption();
  void toggleSendsNumberToOtherEnd();
  void switchOnAndOff();
  void onlyNormalizedSettingsCanBeLinked();
  void findsTargetsInProject();
  void windowListsLinks();
};

#endif /* TEST_LINKS_H_ */
