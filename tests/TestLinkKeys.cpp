/*
 * TestLinkKeys.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestLinkKeys.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <QVBoxLayout>

#include "Link.h"
#include "LinkKeys.h"

using namespace mmp;

void TestLinkKeys::initTestCase()
{
  _table = new LinkTable(this);
  Link link;
  link.trigger = LinkTrigger::forKey("M");
  link.target = LinkTarget::of(LinkTarget::LayerElement, 1, "visible");
  _table->add(link);

  _filter = new KeyLinkFilter(_table, this);
  qApp->installEventFilter(_filter);
  connect(_filter, &KeyLinkFilter::linkTriggered, this, [this](int index) { _triggered << index; });
  connect(_filter, &KeyLinkFilter::keyOffered, this, [this](const QString& key) { _offered << key; });

  _window = new QWidget;
  auto* layout = new QVBoxLayout(_window);
  _canvas = new QWidget(_window);
  _canvas->setFocusPolicy(Qt::StrongFocus);
  _lineEdit = new QLineEdit(_window);
  _list = new QListWidget(_window);
  _list->addItems({ "Alpha", "Mur" });
  layout->addWidget(_canvas);
  layout->addWidget(_lineEdit);
  layout->addWidget(_list);

  _flipAction = new QAction(QStringLiteral("Flip &Horizontal"), _window);
  _flipAction->setShortcut(Qt::Key_H);
  _window->addAction(_flipAction);
  connect(_flipAction, &QAction::triggered, this, [this]() { _flipTriggered++; });

  _window->show();
  _window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(_window));
}

void TestLinkKeys::init()
{
  _filter->setLearning(false);
  _filter->setLinksActive(true);
  _flipTriggered = 0;
  _triggered.clear();
  _offered.clear();
  _canvas->setFocus();
}

void TestLinkKeys::cleanupTestCase()
{
  qApp->removeEventFilter(_filter);
  delete _window;
}

void TestLinkKeys::keyTextIsPortable()
{
  QKeyEvent m(QEvent::KeyPress, Qt::Key_M, Qt::ShiftModifier, "M");
  QCOMPARE(keyLinkText(&m), QString("Shift+M"));
  QKeyEvent keypad(QEvent::KeyPress, Qt::Key_1, Qt::KeypadModifier, "1");
  QCOMPARE(keyLinkText(&keypad), QString("1"));
  QKeyEvent shift(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
  QVERIFY(keyLinkText(&shift).isEmpty());
}

void TestLinkKeys::refusesKeysOfCommands()
{
  // The name of the command comes without its mnemonic.
  QCOMPARE(commandOfKey("H", { _flipAction }), QString("Flip Horizontal"));
  QVERIFY(commandOfKey("Shift+H", { _flipAction }).isEmpty());
}

void TestLinkKeys::refusesKeysOfEditors()
{
  for (const QString& key : { "R", "S", "T", "Left", "Shift+Up", "Space", "Shift+Space", "Esc", "Del", "Backspace" })
    QVERIFY2(!commandOfKey(key, {}).isEmpty(), qPrintable(key));
  // With Shift, R is free unless a command takes it (Shift + R turns the layer, in MapMap).
  QVERIFY(commandOfKey("Shift+R", {}).isEmpty());
}

void TestLinkKeys::acceptsFreeKeys()
{
  for (const QString& key : { "M", "1", "Shift+M", "F5" })
    QVERIFY2(commandOfKey(key, { _flipAction }).isEmpty(), qPrintable(key));
}

void TestLinkKeys::learnedKeyActsOncePerPress()
{
  QTest::keyClick(_canvas, Qt::Key_M);
  QCOMPARE(_triggered, QList<int>{ 0 });
  // A held key repeats: it does not act again.
  QKeyEvent repeat(QEvent::KeyPress, Qt::Key_M, Qt::NoModifier, "m", true);
  QApplication::sendEvent(_canvas, &repeat);
  QCOMPARE(_triggered.size(), 1);
  // Another key does nothing.
  QTest::keyClick(_canvas, Qt::Key_N);
  QCOMPARE(_triggered.size(), 1);
}

void TestLinkKeys::textFieldKeepsLearnedKey()
{
  _lineEdit->clear();
  _lineEdit->setFocus();
  QTest::keyClick(_lineEdit, Qt::Key_M);
  QVERIFY(_triggered.isEmpty());
  QCOMPARE(_lineEdit->text(), QString("m"));
}

void TestLinkKeys::listDoesNotSearchLearnedKey()
{
  _list->setCurrentRow(0);
  _list->setFocus();
  QTest::keyClick(_list, Qt::Key_M);
  QCOMPARE(_triggered.size(), 1);
  QCOMPARE(_list->currentRow(), 0);
}

void TestLinkKeys::inactiveLinksDoNothing()
{
  _filter->setLinksActive(false);
  QTest::keyClick(_canvas, Qt::Key_M);
  QVERIFY(_triggered.isEmpty());
}

void TestLinkKeys::dialogKeepsItsKeysUnlessAllowed()
{
  QDialog dialog(_window);
  QWidget* inside = new QWidget(&dialog);
  inside->setFocusPolicy(Qt::StrongFocus);
  dialog.show();
  dialog.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&dialog));
  inside->setFocus();
  QTest::keyClick(inside, Qt::Key_M);
  QVERIFY(_triggered.isEmpty());

  // The output window, a dialog, receives them.
  dialog.setProperty(ACCEPTS_LEARNED_KEYS_PROPERTY, true);
  QTest::keyClick(inside, Qt::Key_M);
  QCOMPARE(_triggered.size(), 1);
  dialog.close();
  _window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(_window));
}

void TestLinkKeys::learningTakesEveryKey()
{
  _filter->setLearning(true);
  // H is offered for learning: it does not flip the layer.
  QTest::keyClick(_canvas, Qt::Key_H);
  QCOMPARE(_flipTriggered, 0);
  QCOMPARE(_offered, QStringList{ "H" });
  // A learned key does not act while learning.
  QTest::keyClick(_canvas, Qt::Key_M);
  QVERIFY(_triggered.isEmpty());
  QCOMPARE(_offered, (QStringList{ "H", "M" }));

  _filter->setLearning(false);
  QTest::keyClick(_canvas, Qt::Key_H);
  QCOMPARE(_flipTriggered, 1);
}

void TestLinkKeys::learningEscapeAndDelete()
{
  _filter->setLearning(true);
  QSignalSpy cancelled(_filter, &KeyLinkFilter::learningCancelled);
  QSignalSpy removed(_filter, &KeyLinkFilter::removeRequested);
  QTest::keyClick(_canvas, Qt::Key_Delete);
  QTest::keyClick(_canvas, Qt::Key_Backspace);
  QTest::keyClick(_canvas, Qt::Key_Escape);
  QCOMPARE(removed.count(), 2);
  QCOMPARE(cancelled.count(), 1);
  QVERIFY(_offered.isEmpty());
}
