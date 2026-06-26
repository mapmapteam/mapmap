/*
 * TestOsc.h
 *
 * Unit tests for src/control/OscAction (the pure OSC command parser).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_OSC_H_
#define TEST_OSC_H_

#include <QtTest/QtTest>

class TestOsc: public QObject
{
  Q_OBJECT

private slots:
  void rejectsNonMapMapAddress();
  void parsesGlobalTransport();
  void parsesSourceTransport();
  void parsesSourceProperty();
  void parsesSourceByNamePattern();
  void parsesLayerProperty();
  void acceptsMappingAlias();
  void parsesLayerMoveAndTranslate();
  void parsesLayerVertex();
  void rejectsMissingTarget();
  void rejectsMissingValue();
  void rejectsMalformedVertex();
  void ignoresLeadingAndDoubleSlashes();
};

#endif /* TEST_OSC_H_ */
