/*
 * TestSettings.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestSettings.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QSignalSpy>
#include <QVector>

#include "Layer.h"
#include "Mesh.h"
#include "MetaObjectRegistry.h"
#include "Setting.h"
#include "SettingJson.h"
#include "Source.h"

using namespace mmp;

void TestSettings::opacityIsClamped()
{
  Color source(QColor(Qt::red));
  source.setOpacity(5.0f);
  QCOMPARE(source.getOpacity(), 1.0f);
  source.setOpacity(-2.0f);
  QCOMPARE(source.getOpacity(), 0.0f);
}

namespace {

/// Records the notifications a settings tree sends to its listener.
class RecordingListener : public SettingListener
{
public:
  void settingValueChanged(const QString& path, const QVariant& value) override
  {
    paths.append(path);
    values.append(value);
  }

  void settingStructureChanged() override
  {
    structureChanges++;
  }

  QStringList paths;
  QVariantList values;
  int structureChanges = 0;
};

/// Adds a list of points stored in a vector to a group.
SettingList* addPointList(SettingGroup& root, QVector<QPointF>& points)
{
  return root.addList("vertices", SettingInfo::point("vertex", "Vertex"),
                      [&points]() { return int(points.size()); },
                      [&points](int index) { return QVariant(points.at(index)); },
                      [&points](int index, const QVariant& value) {
                        points[index] = value.toPointF();
                        return true;
                      });
}

} // namespace

void TestSettings::settingClampsToBounds()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("opacity", "Opacity").bounds(0.0, 1.0));
  QVERIFY(setting);
  QVERIFY(setting->setValue(5.0));
  QCOMPARE(setting->value().toDouble(), 1.0);
  QVERIFY(setting->setValue(-1));
  QCOMPARE(setting->value().toDouble(), 0.0);
  QCOMPARE(setting->value().typeId(), int(QMetaType::Double));
}

void TestSettings::rejectsNonFinite()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("rate", "Rate"));
  QVERIFY(setting->setValue(0.5));
  QVERIFY(!setting->setValue(qQNaN()));
  QCOMPARE(setting->value().toDouble(), 0.5);
  QVERIFY(!setting->setValue(qInf()));
  QCOMPARE(setting->value().toDouble(), 0.5);
  QVERIFY(!setting->setValue(QString("abc")));
  QCOMPARE(setting->value().toDouble(), 0.5);

  bool ok = true;
  setting->sanitized(qQNaN(), &ok);
  QVERIFY(!ok);
}

void TestSettings::stepIsNotStrictByDefault()
{
  SettingGroup root;
  Setting* loose = root.addSetting(SettingInfo::number("loose", "Loose").bounds(0.0, 1.0).step(0.1));
  QVERIFY(loose->setValue(0.333));
  QCOMPARE(loose->value().toDouble(), 0.333);
  QVERIFY(!loose->info().isStrictStep());

  Setting* strict = root.addSetting(SettingInfo::number("strict", "Strict").bounds(0.0, 1.0).step(0.1, true));
  QVERIFY(strict->setValue(0.333));
  QCOMPARE(strict->value().toDouble(), 0.3);
  QVERIFY(strict->info().isStrictStep());
}

void TestSettings::integerIsRounded()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::integer("nColumns", "Columns"));
  QVERIFY(setting->setValue(2.6));
  QCOMPARE(setting->value().toInt(), 3);
  QCOMPARE(setting->value().typeId(), int(QMetaType::Int));
}

void TestSettings::rangeNeverCrosses()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::range("window", "Window").bounds(0.0, 10.0));
  QVERIFY(setting->setValue(QPointF(8, 3)));
  QCOMPARE(setting->value().toPointF(), QPointF(3, 8));
  QVERIFY(setting->setValue(QPointF(20, -5)));
  QCOMPARE(setting->value().toPointF(), QPointF(0, 10));
}

void TestSettings::choiceKeepsMissingId()
{
  SettingGroup root;
  QList<SettingOption> options = { { "a", "A" }, { "b", "B" } };
  Setting* setting = root.addSetting(SettingInfo::choice("mode", "Mode", options));
  QCOMPARE(setting->info().options().size(), 2);
  QVERIFY(setting->setValue(QString("gone")));
  QCOMPARE(setting->value().toString(), QString("gone"));
}

void TestSettings::readOnlyRefusesWrites()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("id", "ID").defaultValue(0.5).readOnly());
  QCOMPARE(setting->value().toDouble(), 0.5);
  QVERIFY(!setting->setValue(0.7));
  QCOMPARE(setting->value().toDouble(), 0.5);
}

void TestSettings::resetRestoresDefault()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("gain", "Gain").bounds(0.0, 1.0).defaultValue(0.25));
  QCOMPARE(setting->value().toDouble(), 0.25);
  QVERIFY(setting->setValue(0.8));
  QCOMPARE(setting->value().toDouble(), 0.8);
  setting->reset();
  QCOMPARE(setting->value().toDouble(), 0.25);
}

void TestSettings::boundSettingUsesOwnerStorage()
{
  double storage = 0.4;
  bool accepts = true;
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("volume", "Volume").bounds(0.0, 1.0));
  setting->bind([&storage]() { return QVariant(storage); },
                [&storage, &accepts](const QVariant& value) {
                  if (!accepts)
                    return false;
                  storage = value.toDouble();
                  return true;
                });
  QCOMPARE(setting->value().toDouble(), 0.4);

  QVERIFY(setting->setValue(0.6));
  QCOMPARE(storage, 0.6);

  storage = 0.2;
  QCOMPARE(setting->value().toDouble(), 0.2);

  accepts = false;
  QVERIFY(!setting->setValue(0.9));
  QCOMPARE(storage, 0.2);
  QCOMPARE(setting->value().toDouble(), 0.2);

  // An owner whose write function notifies by itself is not notified twice.
  RecordingListener listener;
  root.setListener(&listener);
  setting->bind([&storage]() { return QVariant(storage); },
                [&storage, setting](const QVariant& value) {
                  storage = value.toDouble();
                  setting->notifyChanged();
                  return true;
                });
  QVERIFY(setting->setValue(0.7));
  QCOMPARE(listener.paths, QStringList() << "volume");
  QCOMPARE(listener.values, QVariantList() << 0.7);
}

void TestSettings::pathFollowsStructure()
{
  SettingGroup root;
  SettingGroup* output = root.addGroup("output");
  Setting* locked = output->addSetting(SettingInfo::boolean("locked", "Locked"));
  QCOMPARE(root.path(), QString());
  QCOMPARE(output->path(), QString("output"));
  QCOMPARE(locked->path(), QString("output/locked"));
  QCOMPARE(locked->id(), QString("locked"));
  QCOMPARE(locked->parent(), output);
  QCOMPARE(output->parent(), &root);
  QCOMPARE(root.find("output/locked"), locked);
  QCOMPARE(root.findNode("output"), static_cast<SettingNode*>(output));
  QCOMPARE(root.children().size(), 1);
}

void TestSettings::notifiesTopmostListener()
{
  SettingGroup root;
  RecordingListener listener;
  root.setListener(&listener);
  SettingGroup* output = root.addGroup("output");
  Setting* locked = output->addSetting(SettingInfo::boolean("locked", "Locked"));
  QCOMPARE(listener.structureChanges, 2);

  QVERIFY(locked->setValue(true));
  QCOMPARE(listener.paths, QStringList() << "output/locked");
  QCOMPARE(listener.values, QVariantList() << true);

  QVERIFY(locked->setValue(true));
  QCOMPARE(listener.paths.size(), 1);

  locked->setAvailable(false);
  QVERIFY(!locked->isAvailable());
  QCOMPARE(listener.structureChanges, 3);
  locked->setAvailable(false);
  QCOMPARE(listener.structureChanges, 3);
}

void TestSettings::listFollowsCount()
{
  QVector<QPointF> points = { QPointF(0, 0), QPointF(1, 1) };
  SettingGroup root;
  SettingList* list = addPointList(root, points);
  RecordingListener listener;
  root.setListener(&listener);

  QCOMPARE(list->count(), 2);
  QVERIFY(list->at(1)->setValue(QPointF(5, 6)));
  QCOMPARE(points.at(1), QPointF(5, 6));
  QCOMPARE(list->at(1)->path(), QString("vertices/1"));
  QCOMPARE(list->at(1)->parent(), &root);
  QCOMPARE(listener.paths, QStringList() << "vertices/1");

  points.append(QPointF(7, 8));
  list->sync();
  QCOMPARE(list->count(), 3);
  QCOMPARE(listener.structureChanges, 1);
  QVERIFY(list->at(2));
  QVERIFY(!list->at(3));
  QCOMPARE(root.find("vertices/2")->value().toPointF(), QPointF(7, 8));

  list->notifyChanged();
  QCOMPARE(listener.paths.last(), QString("vertices"));
  QCOMPARE(listener.values.last().toList().size(), 3);
}

void TestSettings::findRejectsBadListIndex()
{
  QVector<QPointF> points = { QPointF(0, 0), QPointF(1, 1) };
  SettingGroup root;
  SettingList* list = addPointList(root, points);
  QVERIFY(root.find("vertices/1"));
  QVERIFY(!root.find("vertices/99"));
  QVERIFY(!root.find("vertices/-1"));
  QVERIFY(!root.find("vertices/x"));
  QVERIFY(!root.find("vertices/1/x"));
  QVERIFY(!root.find("nope"));
  QVERIFY(!root.find("vertices"));
  QCOMPARE(root.findNode("vertices"), static_cast<SettingNode*>(list));
}

void TestSettings::mountedGroupIsNotOwned()
{
  SettingGroup shape;
  Setting* locked = shape.addSetting(SettingInfo::boolean("locked", "Locked"));
  {
    SettingGroup layer;
    RecordingListener listener;
    layer.setListener(&listener);
    layer.mount("output", &shape);
    QCOMPARE(locked->path(), QString("output/locked"));

    QVERIFY(layer.find("output/locked"));
    QVERIFY(layer.find("output/locked")->setValue(true));
    QCOMPARE(locked->value().toBool(), true);
    QCOMPARE(listener.paths, QStringList() << "output/locked");

    layer.unmount("output");
    QVERIFY(!layer.find("output/locked"));
    QVERIFY(!shape.parent());
    QCOMPARE(locked->path(), QString("locked"));

    // Destroying the group it is mounted on leaves it alive.
    layer.mount("output", &shape);
  }
  QVERIFY(!shape.parent());
  QCOMPARE(shape.find("locked"), locked);
  QCOMPARE(locked->value().toBool(), true);

  // Destroying a mounted group detaches it from the group it is mounted on.
  SettingGroup layer;
  SettingGroup* input = new SettingGroup;
  layer.mount("input", input);
  QVERIFY(layer.findNode("input"));
  delete input;
  QVERIFY(!layer.findNode("input"));
  QVERIFY(layer.children().isEmpty());
}

void TestSettings::destroyedMountedGroupDetachesSilently()
{
  SettingGroup layer;
  RecordingListener listener;
  layer.setListener(&listener);
  SettingGroup* input = new SettingGroup;
  layer.mount("input", input);
  const int structureChanges = listener.structureChanges;

  // The owner should unmount first; a group destroyed while mounted leaves without a word.
  delete input;
  QVERIFY(!layer.findNode("input"));
  QCOMPARE(listener.structureChanges, structureChanges);
}

void TestSettings::booleanRefusesOtherText()
{
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::boolean("locked", "Locked"));
  QVERIFY(!setting->setValue(QString("abc")));
  QCOMPARE(setting->value().toBool(), false);
  QVERIFY(!setting->setValue(QString()));
  QVERIFY(!setting->setValue(2));
  QCOMPARE(setting->value().toBool(), false);

  QVERIFY(setting->setValue(QString("TRUE")));
  QCOMPARE(setting->value().toBool(), true);
  QVERIFY(setting->setValue(QString("0")));
  QCOMPARE(setting->value().toBool(), false);
  QVERIFY(setting->setValue(QString("1")));
  QCOMPARE(setting->value().toBool(), true);
  QVERIFY(setting->setValue(QString("False")));
  QCOMPARE(setting->value().toBool(), false);
  QVERIFY(setting->setValue(1.0));
  QCOMPARE(setting->value().toBool(), true);
  QVERIFY(setting->setValue(false));
  QCOMPARE(setting->value().typeId(), int(QMetaType::Bool));

  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("refused the arguments"));
  QVERIFY(!setting->setFromArguments({ QString("abc") }));
  QCOMPARE(setting->value().toBool(), false);
}

void TestSettings::bindRefusesHalfPair()
{
  double storage = 0.4;
  SettingGroup root;
  Setting* setting = root.addSetting(SettingInfo::number("volume", "Volume").bounds(0.0, 1.0));

  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot bind setting"));
  setting->bind([&storage]() { return QVariant(storage); }, nullptr);
  QCOMPARE(setting->value().toDouble(), 0.0);

  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot bind setting"));
  setting->bind(nullptr, [&storage](const QVariant& value) {
    storage = value.toDouble();
    return true;
  });
  QVERIFY(setting->setValue(0.6));
  QCOMPARE(storage, 0.4);
  QCOMPARE(setting->value().toDouble(), 0.6);

  QVector<QPointF> points = { QPointF(0, 0) };
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot add settings list"));
  QVERIFY(!root.addList("vertices", SettingInfo::point("vertex", "Vertex"),
                        [&points]() { return int(points.size()); },
                        [&points](int index) { return QVariant(points.at(index)); },
                        nullptr));
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot add settings list"));
  QVERIFY(!root.addList("vertices", SettingInfo::point("vertex", "Vertex"),
                        nullptr,
                        [&points](int index) { return QVariant(points.at(index)); },
                        [](int, const QVariant&) { return true; }));
  QVERIFY(!root.findNode("vertices"));
}

void TestSettings::warnsOnBadDefault()
{
  SettingGroup root;
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Default value .* cannot be sanitized"));
  QVERIFY(root.addSetting(SettingInfo::number("rate", "Rate").defaultValue(QString("abc"))));
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Default value .* cannot be sanitized"));
  QVERIFY(root.addSetting(SettingInfo::boolean("locked", "Locked").defaultValue(QString("yes"))));
}

void TestSettings::normalizedMapsBounds()
{
  SettingGroup root;
  Setting* angle = root.addSetting(SettingInfo::number("angle", "Angle").bounds(-180.0, 180.0));
  QVERIFY(angle->hasNormalized());
  QVERIFY(angle->setValue(90));
  QCOMPARE(angle->normalized(), 0.75);
  QVERIFY(angle->setNormalized(0.25));
  QCOMPARE(angle->value().toDouble(), -90.0);
  QVERIFY(angle->setNormalized(2.0));
  QCOMPARE(angle->value().toDouble(), 180.0);
  QVERIFY(!angle->setNormalized(qQNaN()));
  QCOMPARE(angle->value().toDouble(), 180.0);

  Setting* count = root.addSetting(SettingInfo::integer("count", "Count").bounds(0, 10));
  QVERIFY(count->hasNormalized());
  QVERIFY(count->setNormalized(0.26));
  QCOMPARE(count->value().toInt(), 3);
  QCOMPARE(count->normalized(), 0.3);
}

void TestSettings::normalizedBooleanAndChoice()
{
  SettingGroup root;
  Setting* locked = root.addSetting(SettingInfo::boolean("locked", "Locked"));
  QVERIFY(locked->hasNormalized());
  QVERIFY(locked->setNormalized(0.49));
  QCOMPARE(locked->value().toBool(), false);
  QCOMPARE(locked->normalized(), 0.0);
  QVERIFY(locked->setNormalized(0.5));
  QCOMPARE(locked->value().toBool(), true);
  QCOMPARE(locked->normalized(), 1.0);

  QList<SettingOption> options = { { "a", "A" }, { "b", "B" }, { "c", "C" }, { "d", "D" } };
  Setting* mode = root.addSetting(SettingInfo::choice("mode", "Mode", options));
  QVERIFY(mode->hasNormalized());
  QVERIFY(mode->setNormalized(0.6));
  QCOMPARE(mode->value().toString(), QString("c"));
  QVERIFY(mode->setNormalized(mode->normalized()));
  QCOMPARE(mode->value().toString(), QString("c"));
  QVERIFY(mode->setNormalized(0.0));
  QCOMPARE(mode->value().toString(), QString("a"));
  QCOMPARE(mode->normalized(), 0.0);
  QVERIFY(mode->setNormalized(1.0));
  QCOMPARE(mode->value().toString(), QString("d"));
  QCOMPARE(mode->normalized(), 1.0);
}

void TestSettings::unboundedHasNoNormalized()
{
  SettingGroup root;
  Setting* name = root.addSetting(SettingInfo::text("name", "Name").defaultValue(QString("layer")));
  QVERIFY(!name->hasNormalized());
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("has no normalized position"));
  QVERIFY(!name->setNormalized(0.5));
  QCOMPARE(name->value().toString(), QString("layer"));

  Setting* rate = root.addSetting(SettingInfo::number("rate", "Rate").defaultValue(1.0));
  QVERIFY(!rate->hasNormalized());
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("has no normalized position"));
  QVERIFY(!rate->setNormalized(0.5));
  QCOMPARE(rate->value().toDouble(), 1.0);

  Setting* gain = root.addSetting(SettingInfo::number("gain", "Gain").bounds(0.0, QVariant()));
  QVERIFY(!gain->hasNormalized());
  Setting* vertex = root.addSetting(SettingInfo::point("vertex", "Vertex").bounds(0.0, 1.0));
  QVERIFY(!vertex->hasNormalized());
  Setting* empty = root.addSetting(SettingInfo::choice("empty", "Empty", QList<SettingOption>()));
  QVERIFY(!empty->hasNormalized());
}

void TestSettings::displayScaleConverts()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(SettingInfo::number("opacity", "Opacity")
                                     .bounds(0.0, 1.0).displayScale(100.0).valueType(QMetaType::Float));
  QVERIFY(opacity->setValue(0.75));
  QCOMPARE(opacity->displayValue().toDouble(), 75.0);
  QVERIFY(opacity->setDisplayValue(500));
  QCOMPARE(opacity->value().toDouble(), 1.0);
  QVERIFY(opacity->setDisplayValue(25));
  QCOMPARE(opacity->value().toDouble(), 0.25);
  QVERIFY(!opacity->setDisplayValue(QString("abc")));
  QCOMPARE(opacity->value().toDouble(), 0.25);

  Setting* name = root.addSetting(SettingInfo::text("name", "Name"));
  QVERIFY(name->setDisplayValue(QString("front")));
  QCOMPARE(name->displayValue(), QVariant(QString("front")));
}

void TestSettings::textMatchesProjectFormat()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(SettingInfo::number("opacity", "Opacity")
                                     .bounds(0.0, 1.0).valueType(QMetaType::Float));
  QVERIFY(opacity->setValue(0.75f));
  QCOMPARE(opacity->toText(), QString("0.75"));

  // A float is written as the project writer always did: converted to double, with all its digits.
  QVERIFY(opacity->setValue(0.3f));
  QCOMPARE(opacity->toText(), QString("0.30000001192092896"));

  Setting* copy = root.addSetting(SettingInfo::number("copy", "Copy").valueType(QMetaType::Float));
  QVERIFY(copy->setFromText(opacity->toText()));
  QCOMPARE(copy->value(), opacity->value());
  QVERIFY(!copy->setFromText("abc"));
  QCOMPARE(copy->value(), opacity->value());

  // A value read from the owner is written as its value type, here a float property.
  double storage = 0.1;
  Setting* bound = root.addSetting(SettingInfo::number("bound", "Bound").valueType(QMetaType::Float));
  bound->bind([&storage]() { return QVariant(storage); },
              [&storage](const QVariant& value) {
                storage = value.toDouble();
                return true;
              });
  QCOMPARE(bound->toText(), QVariant(0.1f).toString());
  QVERIFY(bound->toText() != QVariant(0.1).toString());

  Setting* locked = root.addSetting(SettingInfo::boolean("locked", "Locked"));
  QCOMPARE(locked->toText(), QString("false"));
  QVERIFY(locked->setValue(true));
  QCOMPARE(locked->toText(), QString("true"));
  QVERIFY(locked->setFromText("false"));
  QCOMPARE(locked->value().toBool(), false);
  QVERIFY(!locked->setFromText("yes"));

  Setting* color = root.addSetting(SettingInfo::color("color", "Color"));
  QVERIFY(color->setValue(QColor("#ffaa00")));
  QCOMPARE(color->toText(), QString("#ffaa00"));

  // A colour with alpha keeps it through the text.
  const QColor translucent(255, 170, 0, 128);
  QVERIFY(color->setValue(translucent));
  QCOMPARE(color->toText(), QString("#80ffaa00"));
  const QString text = color->toText();
  QVERIFY(color->setValue(QColor(Qt::black)));
  QVERIFY(color->setFromText(text));
  QCOMPARE(color->value().value<QColor>(), translucent);

  Setting* count = root.addSetting(SettingInfo::integer("count", "Count"));
  QVERIFY(count->setFromText("3"));
  QCOMPARE(count->toText(), QString("3"));
  QCOMPARE(count->value().typeId(), int(QMetaType::Int));
}

void TestSettings::argumentsByType()
{
  SettingGroup root;
  Setting* rate = root.addSetting(SettingInfo::number("rate", "Rate"));
  QVERIFY(rate->setFromArguments({ 0.5 }));
  QCOMPARE(rate->value().toDouble(), 0.5);

  Setting* color = root.addSetting(SettingInfo::color("color", "Color"));
  QVERIFY(color->setFromArguments({ QString("#ff0000") }));
  QCOMPARE(color->value().value<QColor>(), QColor(Qt::red));
  QVERIFY(color->setFromArguments({ QString("blue") }));
  QCOMPARE(color->value().value<QColor>(), QColor(Qt::blue));

  Setting* vertex = root.addSetting(SettingInfo::point("vertex", "Vertex"));
  QVERIFY(vertex->setFromArguments({ 10.0, 20.0 }));
  QCOMPARE(vertex->value().toPointF(), QPointF(10, 20));

  Setting* window = root.addSetting(SettingInfo::range("window", "Window"));
  QVERIFY(window->setFromArguments({ 8.0, 3.0 }));
  QCOMPARE(window->value().toPointF(), QPointF(3, 8));

  Setting* locked = root.addSetting(SettingInfo::boolean("locked", "Locked"));
  QVERIFY(locked->setFromArguments({ 1 }));
  QCOMPARE(locked->value().toBool(), true);
  QVERIFY(locked->setFromArguments({ 0 }));
  QCOMPARE(locked->value().toBool(), false);

  Setting* name = root.addSetting(SettingInfo::text("name", "Name"));
  QVERIFY(name->setFromArguments({ QString("front") }));
  QCOMPARE(name->value().toString(), QString("front"));
}

void TestSettings::rejectsWrongType()
{
  SettingGroup root;
  Setting* rate = root.addSetting(SettingInfo::number("rate", "Rate").defaultValue(0.5));
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("refused the arguments"));
  QVERIFY(!rate->setFromArguments({ QString("abc") }));
  QCOMPARE(rate->value().toDouble(), 0.5);
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("expects 1 argument"));
  QVERIFY(!rate->setFromArguments({}));
  QCOMPARE(rate->value().toDouble(), 0.5);
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("expects 1 argument"));
  QVERIFY(!rate->setFromArguments({ 0.1, 0.2 }));
  QCOMPARE(rate->value().toDouble(), 0.5);

  Setting* vertex = root.addSetting(SettingInfo::point("vertex", "Vertex").defaultValue(QPointF(1, 2)));
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("expects 2 arguments"));
  QVERIFY(!vertex->setFromArguments({ 1.0, 2.0, 3.0 }));
  QCOMPARE(vertex->value().toPointF(), QPointF(1, 2));
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("refused the arguments"));
  QVERIFY(!vertex->setFromArguments({ QString("x"), 2.0 }));
  QCOMPARE(vertex->value().toPointF(), QPointF(1, 2));
}

namespace {

/// Number of columns and rows of the mesh used by the tests of the element settings.
const int MESH_SIZE = 4;

/// Distance between two neighbouring vertices of that mesh, in pixels.
const qreal MESH_SPACING = 100;

/// Returns a mesh of MESH_SIZE x MESH_SIZE vertices, MESH_SPACING apart, starting at origin.
MShape::ptr makeMesh(const QPointF& origin = QPointF())
{
  QVector<QPointF> points;
  for (int row = 0; row < MESH_SIZE; row++)
    for (int column = 0; column < MESH_SIZE; column++)
      points.append(origin + QPointF(column * MESH_SPACING, row * MESH_SPACING));
  return MShape::ptr(new Mesh(points, MESH_SIZE, MESH_SIZE));
}

/// Appends the paths of the settings under node, in declaration order.
void collectLeafPaths(SettingNode* node, QStringList* paths)
{
  switch (node->kind()) {
  case SettingNode::SettingKind:
    paths->append(node->path());
    break;
  case SettingNode::GroupKind:
    for (SettingNode* child : static_cast<SettingGroup*>(node)->children())
      collectLeafPaths(child, paths);
    break;
  case SettingNode::ListKind: {
    SettingList* list = static_cast<SettingList*>(node);
    for (int index = 0; index < list->count(); index++)
      collectLeafPaths(list->at(index), paths);
    break;
  }
  }
}

/// Returns the paths of the settings of an object, in declaration order.
QStringList leafPaths(const Serializable& object)
{
  QStringList paths;
  collectLeafPaths(object.settings(), &paths);
  return paths;
}

/// Returns the paths of the settings of an object that have no description.
QStringList undescribedPaths(const Serializable& object)
{
  QStringList undescribed;
  for (const QString& path : leafPaths(object)) {
    if (object.settings()->find(path)->info().description().isEmpty())
      undescribed.append(path);
  }
  return undescribed;
}

/// Returns the paths "prefix/0" to "prefix/(count - 1)".
QStringList numberedPaths(const QString& prefix, int count)
{
  QStringList paths;
  for (int index = 0; index < count; index++)
    paths.append(prefix + "/" + QString::number(index));
  return paths;
}

/// Installs the functions layers use to find their sources, and removes them when destroyed.
class SourceLookup
{
public:
  explicit SourceLookup(const QList<Source::ptr>& sources)
  {
    Layer::setSourceResolver([sources](uid id) {
      for (const Source::ptr& source : sources) {
        if (source->getId() == id)
          return source;
      }
      return Source::ptr();
    });
    Layer::setSourceLister([sources]() { return QVector<Source::ptr>(sources.begin(), sources.end()); });
  }

  ~SourceLookup()
  {
    Layer::setSourceResolver(Layer::SourceResolver());
    Layer::setSourceLister(Layer::SourceLister());
  }
};

} // namespace

void TestSettings::elementsDeclareTheirSettings()
{
  const QStringList element = { "id", "name", "locked", "opacity" };
  const QStringList texture = element + QStringList { "x", "y" };

  Color color;
  const QStringList colorPaths = element + QStringList { "color" };
  QCOMPARE(leafPaths(color), colorPaths);
  QCOMPARE(undescribedPaths(color), QStringList());

  Image image;
  const QStringList imagePaths = texture + QStringList { "uri", "rate" };
  QCOMPARE(leafPaths(image), imagePaths);
  QCOMPARE(undescribedPaths(image), QStringList());

  Video video;
  const QStringList videoPaths = texture + QStringList { "uri", "rate", "volume" };
  QCOMPARE(leafPaths(video), videoPaths);
  QCOMPARE(undescribedPaths(video), QStringList());

  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  const QStringList layerPaths = element
      + QStringList { "solo", "visible", "depth", "hasInputShape", "sourceId", "output/locked" }
      + numberedPaths("output/vertices", MESH_SIZE * MESH_SIZE)
      + QStringList { "output/nColumns", "output/nRows" };
  QCOMPARE(leafPaths(layer), layerPaths);
  QCOMPARE(undescribedPaths(layer), QStringList());
}

void TestSettings::treeWritesReachTheModel()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());

  QVERIFY(layer.settings()->find("opacity")->setValue(0.5f));
  QCOMPARE(layer.getOpacity(), 0.5f);

  // Within the quad of the corner: the mesh keeps it as it is.
  const QPointF corner(10, 5);
  QVERIFY(layer.settings()->find("output/vertices/0")->setValue(corner));
  QCOMPARE(layer.getShape()->getVertex(0), corner);
}

void TestSettings::modelWritesNotifyTheTree()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());

  QSignalSpy layerSpy(&layer, &Element::settingChanged);
  layer.setOpacity(0.25f);
  QCOMPARE(layerSpy.count(), 1);
  QCOMPARE(layerSpy.at(0).at(0).toInt(), layer.getId());
  QCOMPARE(layerSpy.at(0).at(1).toString(), QString("opacity"));
  QCOMPARE(layerSpy.at(0).at(2).toFloat(), 0.25f);

  Color* color = static_cast<Color*>(red.data());
  QSignalSpy colorSpy(color, &Element::settingChanged);
  color->setColor(QColor(Qt::blue));
  QCOMPARE(colorSpy.count(), 1);
  QCOMPARE(colorSpy.at(0).at(0).toInt(), color->getId());
  QCOMPARE(colorSpy.at(0).at(1).toString(), QString("color"));
  QCOMPARE(colorSpy.at(0).at(2).value<QColor>(), QColor(Qt::blue));

  // Writing the same colour again says nothing.
  color->setColor(QColor(Qt::blue));
  QCOMPARE(colorSpy.count(), 1);

  // One vertex moved: one notification, for that vertex only.
  layerSpy.clear();
  const QPointF moved(MESH_SPACING + 10, 5);
  layer.getShape()->setVertex(1, moved);
  QCOMPARE(layerSpy.count(), 1);
  QCOMPARE(layerSpy.at(0).at(1).toString(), QString("output/vertices/1"));
  QCOMPARE(layerSpy.at(0).at(2).toPointF(), moved);

  // The same through the tree.
  layerSpy.clear();
  QVERIFY(layer.settings()->find("output/vertices/1")->setValue(QPointF(MESH_SPACING, 10)));
  QCOMPARE(layerSpy.count(), 1);
  QCOMPARE(layerSpy.at(0).at(1).toString(), QString("output/vertices/1"));
}

void TestSettings::opacityOverOscRangeIsClamped()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());

  QVERIFY(layer.settings()->find("opacity")->setFromArguments({ 5.0 }));
  QCOMPARE(layer.getOpacity(), 1.0f);
}

void TestSettings::listShrinksWithMesh()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  QVERIFY(layer.settings()->find("output/vertices/15"));

  QSignalSpy structureSpy(&layer, &Element::settingsStructureChanged);
  Mesh* mesh = static_cast<Mesh*>(layer.getShape().data());
  mesh->setNColumns(2);

  QCOMPARE(mesh->nVertices(), 2 * MESH_SIZE);
  QVERIFY(!layer.settings()->find("output/vertices/15"));
  QVERIFY(layer.settings()->find("output/vertices/7"));
  QCOMPARE(structureSpy.count(), 1);
  QCOMPARE(structureSpy.at(0).at(0).toInt(), layer.getId());

  // The fewest divisions of a mesh is kept in one place: the setting.
  mesh->setNColumns(1);
  QCOMPARE(mesh->nColumns(), 2);
  QVERIFY(layer.settings()->find("output/nRows")->setValue(0));
  QCOMPARE(mesh->nRows(), 2);
}

void TestSettings::meshGridsOfTextureLayerFollowEachOther()
{
  Source::ptr image(new Image);
  TextureLayer layer(image, makeMesh(), makeMesh());
  Mesh* output = static_cast<Mesh*>(layer.getShape().data());
  Mesh* input  = static_cast<Mesh*>(layer.getInputShape().data());
  QSignalSpy structureSpy(&layer, &Element::settingsStructureChanged);

  // Five columns: one structure change per mesh, for its longer list of points.
  const int columns = MESH_SIZE + 1;
  QVERIFY(layer.settings()->find("output/nColumns")->setValue(columns));
  QCOMPARE(output->nColumns(), columns);
  QCOMPARE(input->nColumns(), columns);
  QCOMPARE(input->nVertices(), columns * MESH_SIZE);
  QVERIFY(layer.settings()->find("input/vertices/19"));
  QCOMPARE(structureSpy.count(), 2);

  // The other way: rows written on the input mesh.
  const int rows = MESH_SIZE - 1;
  QVERIFY(layer.settings()->find("input/nRows")->setValue(rows));
  QCOMPARE(input->nRows(), rows);
  QCOMPARE(output->nRows(), rows);
  QCOMPARE(output->nColumns(), columns);
  QCOMPARE(structureSpy.count(), 4);
}

void TestSettings::meshGridOfColorLayerIsAlone()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  MShape::ptr loose = makeMesh();
  QSignalSpy structureSpy(&layer, &Element::settingsStructureChanged);

  const int columns = MESH_SIZE + 1;
  QVERIFY(layer.settings()->find("output/nColumns")->setValue(columns));
  QCOMPARE(static_cast<Mesh*>(layer.getShape().data())->nColumns(), columns);
  QCOMPARE(static_cast<Mesh*>(loose.data())->nColumns(), MESH_SIZE);
  QCOMPARE(structureSpy.count(), 1);
}

void TestSettings::readingKeepsDifferentMeshGrids()
{
  MetaObjectRegistry::instance().addBuiltinTypes();
  Source::ptr image(new Image);
  const QVector<QPointF> corners = { QPointF(0, 0), QPointF(1, 0), QPointF(0, 1), QPointF(1, 1) };
  const int fewest = 2;
  TextureLayer first(image, makeMesh(), MShape::ptr(new Mesh(corners, fewest, fewest)));
  QJsonObject obj;
  first.write(obj);

  // A file whose meshes differ is read as it is: nothing is resized, so nothing is rewritten.
  TextureLayer second;
  second.read(obj);
  QCOMPARE(static_cast<Mesh*>(second.getShape().data())->nColumns(), MESH_SIZE);
  QCOMPARE(static_cast<Mesh*>(second.getInputShape().data())->nColumns(), fewest);
}

void TestSettings::rejectsUnknownSource()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  red->setName("red");
  Source::ptr blue(new Color(QColor(Qt::blue)));
  blue->setName("blue");
  Source::ptr picture(new Image());
  picture->setName("picture");
  SourceLookup lookup({ red, blue, picture });

  ColorLayer layer(red, makeMesh());
  Setting* sourceId = layer.settings()->find("sourceId");
  QVERIFY(sourceId);
  QCOMPARE(sourceId->value().toInt(), red->getId());

  const int unknownId = 9999;
  QVERIFY(!sourceId->setValue(unknownId));
  QCOMPARE(layer.getSourceId(), red->getId());

  // An image cannot fill a colour layer.
  QVERIFY(!sourceId->setValue(picture->getId()));
  QCOMPARE(layer.getSourceId(), red->getId());

  QVERIFY(sourceId->setValue(blue->getId()));
  QCOMPARE(layer.getSource(), blue);

  // The options are the compatible sources: their id as text, their name.
  const QList<SettingOption> options = sourceId->info().options();
  QCOMPARE(options.size(), 2);
  QCOMPARE(options.at(0).id, QString::number(red->getId()));
  QCOMPARE(options.at(0).label, QString("red"));
  QCOMPARE(options.at(1).id, QString::number(blue->getId()));
  QCOMPARE(options.at(1).label, QString("blue"));
}

void TestSettings::replacingShapeRemountsTree()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  MShape::ptr first = makeMesh();
  QScopedPointer<ColorLayer> layer(new ColorLayer(red, first));

  const QPointF origin(1000, 2000);
  MShape::ptr second = makeMesh(origin);
  layer->setShape(second);

  QCOMPARE(layer->settings()->find("output/vertices/0")->value().toPointF(), origin);
  QVERIFY(!first->settings()->parent());

  // The first shape can be given to another layer.
  ColorLayer other(red, first);
  QCOMPARE(other.settings()->find("output/vertices/0")->value().toPointF(), QPointF());

  // A destroyed layer lets go of its shape.
  layer.reset();
  QVERIFY(!second->settings()->parent());
}

namespace {

/// Returns the compact JSON text of an object, for readable comparisons.
QByteArray jsonText(const QJsonObject& obj)
{
  return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

} // namespace

void TestSettings::writeMatchesLegacyFormat()
{
  Color source(QColor(Qt::red));
  source.setOpacity(0.75f);
  source.setName("a");

  QJsonObject written;
  source.write(written);

  const QJsonObject expected {
    { "className", "Color" },
    { "color", "#ff0000" },
    { "id", source.getId() },
    { "locked", "false" },
    { "name", "a" },
    { "opacity", "0.75" }
  };
  QCOMPARE(jsonText(written), jsonText(expected));
}

void TestSettings::readToleratesNonTextValues()
{
  Color source(QColor(Qt::red));
  const QJsonObject base { { "className", "Color" }, { "id", source.getId() } };

  // A JSON number or boolean is taken as an argument.
  QJsonObject obj = base;
  obj["opacity"] = 0.5;
  obj["locked"] = true;
  source.read(obj);
  QCOMPARE(source.getOpacity(), 0.5f);
  QVERIFY(source.isLocked());

  // Any other JSON value is skipped, with a warning.
  obj = base;
  obj["opacity"] = QJsonObject();
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("opacity.*cannot be read"));
  source.read(obj);
  QCOMPARE(source.getOpacity(), 0.5f);

  // So is a text that is not a value of the setting.
  obj["opacity"] = "abc";
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("opacity.*cannot be read"));
  source.read(obj);
  QCOMPARE(source.getOpacity(), 0.5f);

  // A number the setting refuses is reported by the setting.
  obj = base;
  obj["locked"] = 2;
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("locked.*refused the arguments"));
  source.read(obj);
  QVERIFY(source.isLocked());
}

void TestSettings::unknownKeysSurviveRoundTrip()
{
  Color source(QColor(Qt::red));
  const QJsonObject nested { { "a", 1 } };
  const QJsonObject obj {
    { "className", "Color" },
    { "id", source.getId() },
    { "opacity", "0.5" },
    { "futureKey", "7" },
    { "nested", nested }
  };
  source.read(obj);

  QJsonObject written;
  source.write(written);
  QCOMPARE(written.value("futureKey"), QJsonValue("7"));
  QCOMPARE(written.value("nested"), QJsonValue(nested));
  QCOMPARE(written.value("opacity"), QJsonValue("0.5"));
  QCOMPARE(written.value("className"), QJsonValue("Color"));
}

void TestSettings::runtimeSettingIsSavedAndRestored()
{
  QJsonObject saved;
  {
    Color first(QColor(Qt::red));
    Setting* gain = first.settings()->addSetting(SettingInfo::number("gain", "Gain"));
    QVERIFY(gain->setValue(0.3));
    first.write(saved);
  }
  QCOMPARE(saved.value("gain"), QJsonValue("0.3"));

  Color second(QColor(Qt::red));
  saved["id"] = second.getId();
  second.read(saved);
  QVERIFY(!second.settings()->find("gain"));

  // The setting added later takes the value kept for it.
  Setting* gain = second.settings()->addSetting(SettingInfo::number("gain", "Gain"));
  QVERIFY(gain);
  QCOMPARE(gain->value().toDouble(), 0.3);

  // The kept value is gone: what is written now comes from the setting.
  QVERIFY(gain->setValue(0.6));
  QVERIFY(second.settings()->remove("gain"));
  QJsonObject written;
  second.write(written);
  QVERIFY(!written.contains("gain"));
}

void TestSettings::reservedKeysAreNotKept()
{
  MetaObjectRegistry::instance().addBuiltinTypes();
  Source::ptr red(new Color(QColor(Qt::red)));

  ColorLayer first(red, makeMesh());
  QJsonObject obj;
  first.write(obj);
  QVERIFY(obj.contains("destination"));
  QVERIFY(!obj.contains("source"));

  // A colour layer has no input shape: a "source" shape in its file is not kept.
  QJsonObject shape = obj.value("destination").toObject();
  shape["futureShapeKey"] = "1";
  obj["destination"] = shape;
  obj["source"] = shape;

  ColorLayer second(red, makeMesh());
  obj["id"] = second.getId();
  second.read(obj);

  QJsonObject written;
  second.write(written);
  QVERIFY(!written.contains("source"));
  QCOMPARE(written.value("id"), QJsonValue(second.getId()));
  QCOMPARE(written.value("sourceId"), QJsonValue(red->getId()));

  // The shape keeps the keys it does not know.
  const QJsonObject writtenShape = written.value("destination").toObject();
  QCOMPARE(writtenShape.value("futureShapeKey"), QJsonValue("1"));
  QCOMPARE(writtenShape.value("vertices"), shape.value("vertices"));
}

void TestSettings::pointSettingIsNotWritten()
{
  Color source(QColor(Qt::red));
  source.settings()->addSetting(SettingInfo::point("spot", "Spot"));

  QJsonObject written;
  QTest::ignoreMessage(QtWarningMsg, QRegularExpression("spot.*no text form"));
  source.write(written);
  QVERIFY(!written.contains("spot"));
  QVERIFY(written.contains("opacity"));
}

namespace {

/// Returns the description of the setting at path in a list made by describeSettings(), or an empty object.
QJsonObject describedAt(const QJsonArray& settings, const QString& path)
{
  for (const QJsonValue& entry : settings) {
    if (entry.toObject().value("path").toString() == path)
      return entry.toObject();
  }
  return QJsonObject();
}

} // namespace

void TestSettings::describesBoundsAndUnits()
{
  Color source(QColor(Qt::red));
  const QJsonArray settings = describeSettings(source.settings());

  const QJsonObject opacity = describedAt(settings, "opacity");
  QCOMPARE(opacity.value("type").toString(), QString("number"));
  QCOMPARE(opacity.value("minimum").toDouble(), 0.0);
  QCOMPARE(opacity.value("maximum").toDouble(), 1.0);
  QCOMPARE(opacity.value("default").toDouble(), 1.0);
  QCOMPARE(opacity.value("unit").toString(), QString("percent"));
  QCOMPARE(opacity.value("value").toDouble(), 1.0);
  QVERIFY(!opacity.value("description").toString().isEmpty());
  QVERIFY(!opacity.value("label").toString().isEmpty());

  // A setting without a bound or a unit leaves the keys out.
  const QJsonObject name = describedAt(settings, "name");
  QCOMPARE(name.value("type").toString(), QString("text"));
  QVERIFY(!name.contains("minimum"));
  QVERIFY(!name.contains("unit"));

  // Declaration order: the id comes first.
  QCOMPARE(settings.first().toObject().value("path").toString(), QString("id"));
}

void TestSettings::describesNestedPaths()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  SourceLookup lookup({ red });
  ColorLayer layer(red, makeMesh());
  const QJsonArray settings = describeSettings(layer.settings());

  const QJsonObject vertex = describedAt(settings, "output/vertices/0");
  QCOMPARE(vertex.value("type").toString(), QString("point"));
  QCOMPARE(vertex.value("value").toArray().size(), 2);
  QVERIFY(!describedAt(settings, "output/vertices/" + QString::number(MESH_SIZE * MESH_SIZE - 1)).isEmpty());
  QVERIFY(describedAt(settings, "output/vertices/" + QString::number(MESH_SIZE * MESH_SIZE)).isEmpty());

  const QJsonObject sourceId = describedAt(settings, "sourceId");
  QVERIFY(!sourceId.value("options").toArray().isEmpty());
  const QJsonObject option = sourceId.value("options").toArray().first().toObject();
  QVERIFY(option.contains("id"));
  QVERIFY(option.contains("label"));

  // Every leaf of the tree is described, once.
  QCOMPARE(settings.size(), leafPaths(layer).size());
}

void TestSettings::valuesKeepLegacyKeys()
{
  Color source(QColor(Qt::red));
  const QJsonObject values = settingValues(source.settings());
  QStringList keys = values.keys();
  keys.sort();
  QCOMPARE(keys, QStringList({ "color", "id", "locked", "name", "opacity" }));
  QCOMPARE(values.value("color").toString(), QString("#ff0000"));
  QCOMPARE(values.value("locked").toBool(true), false);
  QCOMPARE(values.value("id").toInt(), static_cast<int>(source.getId()));

  // A colour with an alpha keeps it.
  source.setColor(QColor(255, 0, 0, 128));
  QCOMPARE(settingValues(source.settings()).value("color").toString(), QString("#80ff0000"));

  // Only the settings directly under the group: not those of a nested group or a list.
  Source::ptr red(new Color(QColor(Qt::red)));
  SourceLookup lookup({ red });
  ColorLayer layer(red, makeMesh());
  const QJsonObject layerValues = settingValues(layer.settings());
  QVERIFY(layerValues.contains("sourceId"));
  QVERIFY(layerValues.contains("visible"));
  QVERIFY(!layerValues.contains("output"));
  QVERIFY(!layerValues.contains("vertices"));
}

void TestSettings::describesFlagsAndForms()
{
  SettingGroup group;
  group.addSetting(SettingInfo::boolean("flag", "Flag").defaultValue(true));
  group.addSetting(SettingInfo::range("span", "Span").bounds(0, 10).defaultValue(QPointF(1, 2)));
  group.addSetting(SettingInfo::point("spot", "Spot").bounds(-5, 20));
  group.addSetting(SettingInfo::number("fixed", "Fixed").readOnly().unit(SettingInfo::Degrees).step(0.5));
  group.addSetting(SettingInfo::choice("mode", "Mode", { { "a", "A" }, { "b", "B" } }));
  Setting* hidden = group.addSetting(SettingInfo::color("tint", "Tint").hidden().defaultValue(QColor(1, 2, 3, 4)));
  hidden->setAvailable(false);

  const QJsonArray settings = describeSettings(&group);
  QCOMPARE(describedAt(settings, "flag").value("value"), QJsonValue(true));
  QCOMPARE(describedAt(settings, "span").value("default").toArray(), QJsonArray({ 1.0, 2.0 }));
  // The bounds of a range or a point are plain numbers, applying to each coordinate.
  const QJsonObject span = describedAt(settings, "span");
  QVERIFY(span.value("minimum").isDouble());
  QVERIFY(span.value("maximum").isDouble());
  QCOMPARE(span.value("minimum").toDouble(), 0.0);
  QCOMPARE(span.value("maximum").toDouble(), 10.0);
  const QJsonObject spot = describedAt(settings, "spot");
  QVERIFY(spot.value("minimum").isDouble());
  QVERIFY(spot.value("maximum").isDouble());
  QCOMPARE(spot.value("minimum").toDouble(), -5.0);
  QCOMPARE(spot.value("maximum").toDouble(), 20.0);
  QCOMPARE(describedAt(settings, "fixed").value("readOnly"), QJsonValue(true));
  QCOMPARE(describedAt(settings, "fixed").value("unit").toString(), QString("degrees"));
  QCOMPARE(describedAt(settings, "fixed").value("step").toDouble(), 0.5);
  QCOMPARE(describedAt(settings, "mode").value("options").toArray().size(), 2);
  QVERIFY(!describedAt(settings, "flag").contains("readOnly"));
  QVERIFY(!describedAt(settings, "flag").contains("available"));

  // Hidden only concerns the panel: the setting is listed, with its alpha.
  QCOMPARE(describedAt(settings, "tint").value("available"), QJsonValue(false));
  QCOMPARE(describedAt(settings, "tint").value("default").toString(), QString("#04010203"));
}
