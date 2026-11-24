/*
 * TestHandleModeKeys.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestHandleModeKeys.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include "HandleModeKeys.h"

using namespace mmp;

void TestHandleModeKeys::initTestCase()
{
  _keys = new HandleModeKeys(this);
  qApp->installEventFilter(_keys);

  _window = new QWidget;
  auto* layout = new QVBoxLayout(_window);
  _canvas = new QWidget(_window);
  _canvas->setFocusPolicy(Qt::StrongFocus);
  _lineEdit = new QLineEdit(_window);
  _list = new QListWidget(_window);
  _list->addItems({ "Alpha", "Screen" });
  layout->addWidget(_canvas);
  layout->addWidget(_lineEdit);
  layout->addWidget(_list);

  _rotateAction = new QAction(_window);
  _rotateAction->setShortcut(Qt::SHIFT | Qt::Key_R);
  _window->addAction(_rotateAction);
  connect(_rotateAction, &QAction::triggered, this, [this]() { _rotateTriggered++; });

  _window->show();
  _window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(_window));
}

void TestHandleModeKeys::init()
{
  _keys->setMode(HandleMode::Free);
  _keys->setDragActive(false);
  _rotateTriggered = 0;
  _canvas->setFocus();
}

void TestHandleModeKeys::cleanupTestCase()
{
  qApp->removeEventFilter(_keys);
  delete _window;
}

void TestHandleModeKeys::keyChoosesMode()
{
  QTest::keyClick(_canvas, Qt::Key_R);
  QCOMPARE(_keys->mode(), HandleMode::Rotate);
  QTest::keyClick(_canvas, Qt::Key_S);
  QCOMPARE(_keys->mode(), HandleMode::Scale);
  QTest::keyClick(_canvas, Qt::Key_T);
  QCOMPARE(_keys->mode(), HandleMode::Free);
}

void TestHandleModeKeys::textFieldKeepsLetters()
{
  _lineEdit->clear();
  _lineEdit->setFocus();
  QTest::keyClick(_lineEdit, Qt::Key_S);
  QCOMPARE(_keys->mode(), HandleMode::Free);
  QCOMPARE(_lineEdit->text(), QString("s"));
}

void TestHandleModeKeys::modifiedKeyOutsideDragKeepsShortcut()
{
  QTest::keyClick(_canvas, Qt::Key_R, Qt::ShiftModifier);
  QCOMPARE(_rotateTriggered, 1);
  QCOMPARE(_keys->mode(), HandleMode::Free);
}

void TestHandleModeKeys::modifiedKeyDuringDragChangesMode()
{
  _keys->setDragActive(true);
  QTest::keyClick(_canvas, Qt::Key_R, Qt::ShiftModifier);
  QCOMPARE(_keys->mode(), HandleMode::Rotate);
  QCOMPARE(_rotateTriggered, 0);
}

void TestHandleModeKeys::repeatedKeyNeverTriggersShortcut()
{
  // A held Shift + R repeating after a drag: the filter takes the shortcut
  // override, so the shortcut does not fire.
  QKeyEvent repeat(QEvent::ShortcutOverride, Qt::Key_R, Qt::ShiftModifier, "R", true);
  repeat.ignore();
  QApplication::sendEvent(_canvas, &repeat);
  QVERIFY(repeat.isAccepted());
}

void TestHandleModeKeys::itemViewKeyChangesModeNotSelection()
{
  _list->setCurrentRow(0);
  _list->setFocus();
  QTest::keyClick(_list, Qt::Key_S);
  QCOMPARE(_keys->mode(), HandleMode::Scale);
  QCOMPARE(_list->currentRow(), 0);
}

void TestHandleModeKeys::repeatedKeyPressIsConsumed()
{
  // After its shortcut override was taken, a repeated Shift + R must not reach
  // the list either, where it would search for a layer starting with "R".
  _list->clear();
  _list->addItems({ "Alpha", "Rose" });
  _list->setCurrentRow(0);
  _list->setFocus();
  QKeyEvent repeat(QEvent::KeyPress, Qt::Key_R, Qt::ShiftModifier, "R", true);
  QApplication::sendEvent(_list, &repeat);
  QCOMPARE(_list->currentRow(), 0);
  _list->clear();
  _list->addItems({ "Alpha", "Screen" });
}

void TestHandleModeKeys::dialogKeepsItsKeys()
{
  // In a modal dialog (a file dialog, the preferences), R, S and T are the
  // dialog's: the mode does not change behind it.
  QDialog dialog(_window);
  QListWidget* files = new QListWidget(&dialog);
  files->addItems({ "a.mmp", "s.mmp" });
  dialog.setWindowModality(Qt::ApplicationModal);
  dialog.show();
  dialog.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&dialog));
  files->setCurrentRow(0);
  files->setFocus();
  QTest::keyClick(files, Qt::Key_S);
  QCOMPARE(_keys->mode(), HandleMode::Free);
  QCOMPARE(files->currentRow(), 1);
  dialog.close();
  _window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(_window));
}
