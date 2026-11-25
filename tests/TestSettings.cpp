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
#include <QPointF>
#include <QVector>

#include "Setting.h"
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
