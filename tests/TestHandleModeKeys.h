/*
 * TestHandleModeKeys.h
 *
 * Unit tests for the keys that choose the mode of the shape handle
 * (src/gui/HandleModeKeys), with key events simulated offscreen.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_HANDLE_MODE_KEYS_H_
#define TEST_HANDLE_MODE_KEYS_H_

#include <QtTest/QtTest>

class QAction;
class QLineEdit;
class QListWidget;
class QWidget;

namespace mmp { class HandleModeKeys; }

class TestHandleModeKeys: public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();

  void keyChoosesMode();
  void textFieldKeepsLetters();
  void modifiedKeyOutsideDragKeepsShortcut();
  void modifiedKeyDuringDragChangesMode();
  void repeatedKeyNeverTriggersShortcut();
  void itemViewKeyChangesModeNotSelection();
  void repeatedKeyPressIsConsumed();
  void dialogKeepsItsKeys();

private:
  mmp::HandleModeKeys* _keys = nullptr;
  QWidget* _window = nullptr;
  QWidget* _canvas = nullptr;      // Stands for a view: takes the focus, ignores keys.
  QLineEdit* _lineEdit = nullptr;
  QListWidget* _list = nullptr;
  QAction* _rotateAction = nullptr; // Shift + R, like "Rotate 90° CW".
  int _rotateTriggered = 0;
};

#endif /* TEST_HANDLE_MODE_KEYS_H_ */
