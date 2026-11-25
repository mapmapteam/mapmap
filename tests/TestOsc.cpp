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
#include "RemoteSettingWrite.h"
#include "Source.h"

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

void TestOsc::parsesLayerFlip()
{
  OscAction h = parseOscAction("/mapmap/layer/flip/horizontal", { 3 });
  QCOMPARE(typeOf(h), int(OscAction::LayerFlip));
  QCOMPARE(h.id, 3);
  QCOMPARE(int(h.flipDirection), int(OscAction::FlipHorizontal));
  // Without source/destination, the output shape is flipped.
  QCOMPARE(int(h.shapeRole), int(OscAction::OutputShape));

  OscAction v = parseOscAction("/mapmap/layer/flip/destination/vertical", { 3 });
  QCOMPARE(typeOf(v), int(OscAction::LayerFlip));
  QCOMPARE(int(v.flipDirection), int(OscAction::FlipVertical));
  QCOMPARE(int(v.shapeRole), int(OscAction::OutputShape));

  // "source" flips the image inside the layer.
  OscAction img = parseOscAction("/mapmap/mapping/flip/source/horizontal", { QString("toy-*") });
  QCOMPARE(typeOf(img), int(OscAction::LayerFlip));
  QVERIFY(img.selectByName);
  QCOMPARE(img.name, QString("toy-*"));
  QCOMPARE(int(img.shapeRole), int(OscAction::InputShape));
}

void TestOsc::parsesLayerRotate()
{
  OscAction cw = parseOscAction("/mapmap/layer/rotate/cw", { 2 });
  QCOMPARE(typeOf(cw), int(OscAction::LayerRotate));
  QCOMPARE(cw.id, 2);
  QCOMPARE(int(cw.turn), int(OscAction::Turn90CW));
  QCOMPARE(int(cw.shapeRole), int(OscAction::OutputShape));

  OscAction ccw = parseOscAction("/mapmap/layer/rotate/source/ccw", { 2 });
  QCOMPARE(typeOf(ccw), int(OscAction::LayerRotate));
  QCOMPARE(int(ccw.turn), int(OscAction::Turn90CCW));
  QCOMPARE(int(ccw.shapeRole), int(OscAction::InputShape));

  OscAction half = parseOscAction("/mapmap/layer/rotate/180", { 2 });
  QCOMPARE(typeOf(half), int(OscAction::LayerRotate));
  QCOMPARE(int(half.turn), int(OscAction::Turn180));
}

void TestOsc::rejectsMalformedFlipAndRotate()
{
  // Missing target.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/flip/horizontal", {})), int(OscAction::Invalid));
  // Unknown direction or amount.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/flip/diagonal", { 1 })), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/rotate/45", { 1 })), int(OscAction::Invalid));
  // Unknown shape role.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/flip/middle/horizontal", { 1 })), int(OscAction::Invalid));
  // No direction at all.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/rotate", { 1 })), int(OscAction::Invalid));
}

void TestOsc::parsesLayerSettingPath()
{
  OscAction a = parseOscAction("/mapmap/layer/3/opacity", { 0.5 });
  QCOMPARE(typeOf(a), int(OscAction::LayerSetting));
  QVERIFY(!a.selectByName);
  QCOMPARE(a.id, 3);
  QCOMPARE(a.path, QString("opacity"));
  QCOMPARE(a.values, QVariantList({ 0.5 }));

  // "mapping" is an alias of "layer" here too.
  OscAction alias = parseOscAction("/mapmap/mapping/3/opacity", { 0.5 });
  QCOMPARE(typeOf(alias), int(OscAction::LayerSetting));
  QCOMPARE(alias.id, 3);
}

void TestOsc::parsesNestedSettingPath()
{
  OscAction a = parseOscAction("/mapmap/layer/3/output/vertices/5", { 10.0, 20.0 });
  QCOMPARE(typeOf(a), int(OscAction::LayerSetting));
  QCOMPARE(a.id, 3);
  QCOMPARE(a.path, QString("output/vertices/5"));
  QCOMPARE(a.values, QVariantList({ 10.0, 20.0 }));
}

void TestOsc::parsesSourceSettingPath()
{
  OscAction a = parseOscAction("/mapmap/source/2/color", { QString("#ff0000") });
  QCOMPARE(typeOf(a), int(OscAction::SourceSetting));
  QVERIFY(!a.selectByName);
  QCOMPARE(a.id, 2);
  QCOMPARE(a.path, QString("color"));
  QCOMPARE(a.values, QVariantList({ QString("#ff0000") }));
}

void TestOsc::rejectsSettingPathWithoutValue()
{
  // A path with no value to write.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/3/opacity", {})), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/2/color", {})), int(OscAction::Invalid));
  // An id with no path; with two arguments, the old form would read "3" as a property.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/3", { 0.5 })), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/3", { 3, 0.5 })), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/2", { 2, 0.5 })), int(OscAction::Invalid));
  QCOMPARE(typeOf(parseOscAction("/mapmap/source/2", { 0.5 })), int(OscAction::Invalid));
  // An id too large for an int.
  QCOMPARE(typeOf(parseOscAction("/mapmap/layer/99999999999/opacity", { 0.5 })), int(OscAction::Invalid));
}

void TestOsc::legacyPropertyFormStillParses()
{
  OscAction a = parseOscAction("/mapmap/layer/opacity", { 3, 0.5 });
  QCOMPARE(typeOf(a), int(OscAction::LayerProperty));
  QVERIFY(!a.selectByName);
  QCOMPARE(a.id, 3);
  QCOMPARE(a.property, QString("opacity"));
  QCOMPARE(a.value.toDouble(), 0.5);

  // A segment that is not only digits is still a property name.
  OscAction signedId = parseOscAction("/mapmap/layer/-3", { 3, 0.5 });
  QCOMPARE(typeOf(signedId), int(OscAction::LayerProperty));
  QCOMPARE(signedId.property, QString("-3"));
}

void TestOsc::unchangedVideoFileIsAccepted()
{
  Video video;
  Setting* uri = video.settings()->find("uri");
  QVERIFY(uri);

  // Without the preference "Allow message with existing media source", the file the video
  // already plays is an unchanged value: accepted, as for any setting.
  QVERIFY(writeRemoteSetting(video, uri, { video.getUri() }, false));
  QCOMPARE(video.getUri(), QString());

  // Any other setting goes through its arguments.
  Setting* rate = video.settings()->find("rate");
  QVERIFY(writeRemoteSetting(video, rate, { 0.5 }, false));
  QCOMPARE(video.getRate(), 0.5);
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("refused the arguments"));
  QVERIFY(!writeRemoteSetting(video, rate, { QString("fast") }, false));
  QCOMPARE(video.getRate(), 0.5);
}
