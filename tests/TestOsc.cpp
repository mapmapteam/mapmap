/*
 * TestOsc.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestOsc.h"

#include "OscAction.h"

using namespace mmp;

namespace {
// Shorthand: the type of a parsed action, as an int (for nice QCOMPARE output).
int typeOf(const OscAction& a) { return static_cast<int>(a.type); }
}

void TestOsc::rejectsNonMapMapAddress()
{
  QCOMPARE(typeOf(parseOscAction("/foo/play", {})), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("", {})),          int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap", {})),   int(OscAction::Invalid));
}

void TestOsc::parsesGlobalTransport()
{
  QCOMPARE(typeOf(parseOscAction("/mapmap/play",   {})), int(OscAction::PlayAll));
  QCOMPARE(typeOf(parseOscAction("/mapmap/pause",  {})), int(OscAction::PauseAll));
  QCOMPARE(typeOf(parseOscAction("/mapmap/rewind", {})), int(OscAction::RewindAll));
  QCOMPARE(typeOf(parseOscAction("/mapmap/quit",   {})), int(OscAction::Quit));
}

void TestOsc::parsesSourceTransport()
{
  OscAction play = parseOscAction("/mapmap/source/play", { 3 });
  QCOMPARE(typeOf(play), int(OscAction::SourcePlay));
  QVERIFY(!play.selectByName);
  QCOMPARE(play.id, 3);

  QCOMPARE(typeOf(parseOscAction("/mapmap/source/pause",  { 3 })), int(OscAction::SourcePause));
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/rewind", { 3 })), int(OscAction::SourceRewind));
}

void TestOsc::parsesSourceProperty()
{
  OscAction a = parseOscAction("/mapmap/source/opacity", { 2, 0.5 });
  QCOMPARE(typeOf(a), int(OscAction::SourceProperty));
  QVERIFY(!a.selectByName);
  QCOMPARE(a.id, 2);
  QCOMPARE(a.property, QString("opacity"));
  QCOMPARE(a.value.toDouble(), 0.5);
}

void TestOsc::parsesSourceByNamePattern()
{
  OscAction a = parseOscAction("/mapmap/source/opacity", { QString("clip*"), 0.25 });
  QCOMPARE(typeOf(a), int(OscAction::SourceProperty));
  QVERIFY(a.selectByName);
  QCOMPARE(a.name, QString("clip*"));
  QCOMPARE(a.value.toDouble(), 0.25);
}

void TestOsc::parsesLayerProperty()
{
  OscAction a = parseOscAction("/mapmap/layer/visible", { 7, true });
  QCOMPARE(typeOf(a), int(OscAction::LayerProperty));
  QCOMPARE(a.id, 7);
  QCOMPARE(a.property, QString("visible"));
  QCOMPARE(a.value.toBool(), true);
}

void TestOsc::acceptsMappingAlias()
{
  // "mapping" is an accepted alias for "layer".
  OscAction a = parseOscAction("/mapmap/mapping/solo", { 1, false });
  QCOMPARE(typeOf(a), int(OscAction::LayerProperty));
  QCOMPARE(a.property, QString("solo"));
}

void TestOsc::parsesLayerMoveAndTranslate()
{
  OscAction move = parseOscAction("/mapmap/layer/move/xy", { 4, 0.5, 0.25 });
  QCOMPARE(typeOf(move), int(OscAction::LayerMove));
  QCOMPARE(move.id, 4);
  QCOMPARE(move.x, 0.5);
  QCOMPARE(move.y, 0.25);

  OscAction tr = parseOscAction("/mapmap/layer/translate/xy", { 4, -0.1, 0.2 });
  QCOMPARE(typeOf(tr), int(OscAction::LayerTranslate));
  QCOMPARE(tr.x, -0.1);
  QCOMPARE(tr.y, 0.2);
}

void TestOsc::parsesLayerVertex()
{
  OscAction src = parseOscAction("/mapmap/layer/vertex/source/xy", { 5, 2, 0.1, 0.9 });
  QCOMPARE(typeOf(src), int(OscAction::LayerVertex));
  QCOMPARE(int(src.shapeRole), int(OscAction::InputShape));
  QCOMPARE(src.vertexIndex, 2);
  QCOMPARE(src.x, 0.1);
  QCOMPARE(src.y, 0.9);

  OscAction dst = parseOscAction("/mapmap/layer/vertex/destination/xy", { 5, 0, 1.0, 1.0 });
  QCOMPARE(typeOf(dst), int(OscAction::LayerVertex));
  QCOMPARE(int(dst.shapeRole), int(OscAction::OutputShape));
  QCOMPARE(dst.vertexIndex, 0);

  // Without source/destination, the output shape is the default.
  OscAction bare = parseOscAction("/mapmap/layer/vertex/xy", { 5, 1, 0.3, 0.3 });
  QCOMPARE(typeOf(bare), int(OscAction::LayerVertex));
  QCOMPARE(int(bare.shapeRole), int(OscAction::OutputShape));
  QCOMPARE(bare.vertexIndex, 1);
}

void TestOsc::rejectsMissingTarget()
{
  // No selector argument at all.
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/play",    {})), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/opacity", {})), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/visible",  {})), int(OscAction::Invalid));
}

void TestOsc::rejectsMissingValue()
{
  // Selector present but no value to set.
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/opacity", { 1 })), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/visible",  { 1 })), int(OscAction::Invalid));
}

void TestOsc::rejectsMalformedVertex()
{
  // move/xy needs x and y; here only x is provided.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/move/xy", { 1, 0.5 })), int(OscAction::Invalid));
  // vertex needs index + x + y.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/vertex/source/xy", { 1, 2 })), int(OscAction::Invalid));
  // Unknown vertex role.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/vertex/middle/xy", { 1, 2, 0.0, 0.0 })), int(OscAction::Invalid));
}

void TestOsc::ignoresLeadingAndDoubleSlashes()
{
  // Leading slash, no slash, and accidental double slashes all tokenize the same.
  QCOMPARE(typeOf(parseOscAction("mapmap/play", {})),    int(OscAction::PlayAll));
  QCOMPARE(typeOf(parseOscAction("//mapmap//play", {})), int(OscAction::PlayAll));
}
