/*
 * TestLinkKeys.h
 *
 * Unit tests for the learned keys (src/gui/LinkKeys): the keys MapMap refuses to learn, and
 * the filter that applies the learned ones, with key events simulated offscreen.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_LINK_KEYS_H_
#define TEST_LINK_KEYS_H_

#include <QtTest/QtTest>

class QAction;
class QLineEdit;
class QListWidget;
class QWidget;

namespace mmp { class KeyLinkFilter; class LinkTable; }

class TestLinkKeys: public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();

  void keyTextIsPortable();
  void refusesKeysOfCommands();
  void refusesKeysOfEditors();
  void acceptsFreeKeys();
  void learnedKeyActsOncePerPress();
  void textFieldKeepsLearnedKey();
  void listDoesNotSearchLearnedKey();
  void inactiveLinksDoNothing();
  void dialogKeepsItsKeysUnlessAllowed();
  void learningTakesEveryKey();
  void learningEscapeAndDelete();
  void samePressReceivedTwiceActsOnce();

private:
  mmp::LinkTable* _table = nullptr;
  mmp::KeyLinkFilter* _filter = nullptr;
  QWidget* _window = nullptr;
  QWidget* _canvas = nullptr;      // Stands for a view: takes the focus, ignores keys.
  QLineEdit* _lineEdit = nullptr;
  QListWidget* _list = nullptr;
  QAction* _flipAction = nullptr;  // H, like "Flip Horizontal".
  int _flipTriggered = 0;
  QList<int> _triggered;
  QStringList _offered;
};

#endif /* TEST_LINK_KEYS_H_ */
