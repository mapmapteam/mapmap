/*
 * TestLinks.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestLinks.h"

#include <QSignalSpy>

#include <QAbstractButton>
#include <QTableWidget>

#include "Link.h"
#include "LinksWindow.h"
#include "MappingManager.h"
#include "Setting.h"
#include "Shapes.h"

using namespace mmp;

namespace {

Link keyLink(const QString& key, uid id, const QString& path)
{
  Link link;
  link.trigger = LinkTrigger::forKey(key);
  link.target = LinkTarget::of(LinkTarget::LayerElement, id, path);
  return link;
}

Layer::ptr makeColorLayer(Source::ptr source)
{
  MShape::ptr quad(new Quad(QPointF(0, 0), QPointF(1, 0), QPointF(1, 1), QPointF(0, 1)));
  return Layer::ptr(new ColorLayer(source, quad));
}

} // namespace

void TestLinks::keyTriggerIsPortable()
{
  const LinkTrigger trigger = LinkTrigger::forKey("shift+m");
  QVERIFY(trigger.isValid());
  QCOMPARE(trigger.key, QString("Shift+M"));
  QCOMPARE(trigger, LinkTrigger::forKey("Shift+M"));
  QVERIFY(trigger != LinkTrigger::forKey("M"));
  QVERIFY(!LinkTrigger::forKey("").isValid());
  // A sequence of keys is not a trigger.
  QVERIFY(!LinkTrigger::forKey("G, H").isValid());
}

void TestLinks::triggerDrivesOneSetting()
{
  LinkTable table;
  QVERIFY(table.add(keyLink("M", 1, "visible")));
  QVERIFY(table.add(keyLink("M", 2, "visible")));
  QCOMPARE(table.count(), 1);
  QCOMPARE(table.links().first().target.id, 2);
}

void TestLinks::settingHasOneTriggerPerKind()
{
  LinkTable table;
  QVERIFY(table.add(keyLink("M", 1, "visible")));
  QVERIFY(table.add(keyLink("N", 1, "visible")));
  QCOMPARE(table.count(), 1);
  QCOMPARE(table.links().first().trigger.key, QString("N"));
  QVERIFY(table.add(keyLink("M", 1, "solo")));
  QCOMPARE(table.count(), 2);
}

void TestLinks::refusesInvalidLinks()
{
  LinkTable table;
  QSignalSpy changed(&table, &LinkTable::changed);
  QVERIFY(!table.add(keyLink("", 1, "visible")));
  QVERIFY(!table.add(keyLink("M", NULL_UID, "visible")));
  QVERIFY(!table.add(keyLink("M", 1, "")));
  QVERIFY(table.isEmpty());
  QCOMPARE(changed.count(), 0);
}

void TestLinks::removesLinks()
{
  LinkTable table;
  table.add(keyLink("M", 1, "visible"));
  table.add(keyLink("N", 2, "visible"));
  QSignalSpy changed(&table, &LinkTable::changed);
  QVERIFY(table.remove(LinkTarget::of(LinkTarget::LayerElement, 1, "visible"), LinkTrigger::Key));
  QVERIFY(!table.remove(LinkTarget::of(LinkTarget::LayerElement, 1, "visible"), LinkTrigger::Key));
  QCOMPARE(table.count(), 1);
  QVERIFY(table.removeAt(0));
  QVERIFY(!table.removeAt(0));
  QVERIFY(table.isEmpty());
  QCOMPARE(changed.count(), 2);
}

void TestLinks::writesAndReadsLinks()
{
  LinkTable table;
  table.add(keyLink("M", 3, "visible"));
  Link off = keyLink("Shift+1", 4, "opacity");
  off.target.element = LinkTarget::SourceElement;
  off.action = LinkAction::SwitchOff;
  table.add(off);

  const QJsonArray json = table.toJson();
  QCOMPARE(json.size(), 2);
  const QJsonObject first = json.at(0).toObject();
  QCOMPARE(first["trigger"].toObject()["kind"].toString(), QString("key"));
  QCOMPARE(first["trigger"].toObject()["key"].toString(), QString("M"));
  QCOMPARE(first["target"].toObject()["element"].toString(), QString("layer"));
  QCOMPARE(first["target"].toObject()["id"].toInt(), 3);
  QCOMPARE(first["target"].toObject()["setting"].toString(), QString("visible"));
  QCOMPARE(first["action"].toString(), QString("toggle"));

  LinkTable read;
  QStringList skipped;
  read.fromJson(json, &skipped);
  QVERIFY(skipped.isEmpty());
  QCOMPARE(read.count(), 2);
  QCOMPARE(read.links().at(1).target, off.target);
  QCOMPARE(read.links().at(1).trigger, off.trigger);
  QVERIFY(read.links().at(1).action == LinkAction::SwitchOff);
}

void TestLinks::skipsUnreadableLinks()
{
  const QByteArray text = R"([
    {"trigger": {"kind": "key", "key": "M"}, "target": {"element": "layer", "id": 3, "setting": "visible"}},
    {"trigger": {"kind": "pedal", "key": "M"}, "target": {"element": "layer", "id": 3, "setting": "solo"}},
    {"trigger": {"kind": "key", "key": "N"}, "target": {"element": "cue", "id": 3, "setting": "solo"}},
    {"trigger": {"kind": "key", "key": "O"}, "target": {"element": "layer", "id": 3, "setting": "solo"}, "action": "flash"},
    "not a link"
  ])";
  LinkTable table;
  QStringList skipped;
  table.fromJson(QJsonDocument::fromJson(text).array(), &skipped);
  QCOMPARE(table.count(), 1);
  // Without an action, a link toggles.
  QVERIFY(table.links().first().action == LinkAction::Toggle);
  QCOMPARE(skipped.size(), 4);
  QVERIFY(skipped.at(0).startsWith("links[1]"));
}

void TestLinks::toggleSwitchesBoolean()
{
  SettingGroup root;
  Setting* visible = root.addSetting(SettingInfo::boolean("visible", "Visible").defaultValue(true));
  QVERIFY(applyLinkAction(visible, LinkAction::Toggle));
  QCOMPARE(visible->value().toBool(), false);
  QVERIFY(applyLinkAction(visible, LinkAction::Toggle));
  QCOMPARE(visible->value().toBool(), true);
}

void TestLinks::toggleGoesToNextOption()
{
  SettingGroup root;
  Setting* mode = root.addSetting(SettingInfo::choice("mode", "Mode",
                                  { { "a", "A" }, { "b", "B" }, { "c", "C" } }).defaultValue("a"));
  QVERIFY(applyLinkAction(mode, LinkAction::Toggle));
  QCOMPARE(mode->value().toString(), QString("b"));
  applyLinkAction(mode, LinkAction::Toggle);
  QCOMPARE(mode->value().toString(), QString("c"));
  // Back to the first after the last.
  applyLinkAction(mode, LinkAction::Toggle);
  QCOMPARE(mode->value().toString(), QString("a"));
}

void TestLinks::toggleSendsNumberToOtherEnd()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(SettingInfo::number("opacity", "Opacity")
                                     .bounds(0.0, 1.0).defaultValue(0.3));
  QVERIFY(applyLinkAction(opacity, LinkAction::Toggle));
  QCOMPARE(opacity->value().toDouble(), 1.0);
  QVERIFY(applyLinkAction(opacity, LinkAction::Toggle));
  QCOMPARE(opacity->value().toDouble(), 0.0);
}

void TestLinks::switchOnAndOff()
{
  SettingGroup root;
  Setting* visible = root.addSetting(SettingInfo::boolean("visible", "Visible"));
  Setting* opacity = root.addSetting(SettingInfo::number("opacity", "Opacity").bounds(0.0, 1.0));
  // Pressed twice, a key that shows or hides keeps doing so.
  for (int i = 0; i < 2; ++i) {
    QVERIFY(applyLinkAction(visible, LinkAction::SwitchOn));
    QCOMPARE(visible->value().toBool(), true);
  }
  QVERIFY(applyLinkAction(visible, LinkAction::SwitchOff));
  QCOMPARE(visible->value().toBool(), false);
  QVERIFY(applyLinkAction(opacity, LinkAction::SwitchOn));
  QCOMPARE(opacity->value().toDouble(), 1.0);
  QVERIFY(applyLinkAction(opacity, LinkAction::SwitchOff));
  QCOMPARE(opacity->value().toDouble(), 0.0);
}

void TestLinks::onlyNormalizedSettingsCanBeLinked()
{
  SettingGroup root;
  Setting* point = root.addSetting(SettingInfo::point("center", "Center"));
  Setting* text = root.addSetting(SettingInfo::text("name", "Name"));
  Setting* open = root.addSetting(SettingInfo::number("rate", "Rate"));
  Setting* fixed = root.addSetting(SettingInfo::boolean("fixed", "Fixed").readOnly());
  for (Setting* setting : { point, text, open, fixed }) {
    QVERIFY2(!canBeLinked(setting), qPrintable(setting->id()));
    QVERIFY(!applyLinkAction(setting, LinkAction::Toggle));
  }
  QVERIFY(!canBeLinked(nullptr));
}

void TestLinks::findsTargetsInProject()
{
  MappingManager manager;
  Source::ptr color(new Color());
  manager.addSource(color);
  Layer::ptr layer = makeColorLayer(color);
  manager.addLayer(layer);

  const LinkTarget eye = LinkTarget::of(LinkTarget::LayerElement, layer->getId(), "visible");
  QCOMPARE(manager.linkTargetSetting(eye), layer->settings()->find("visible"));
  const LinkTarget sourceOpacity = LinkTarget::of(LinkTarget::SourceElement, color->getId(), "opacity");
  QCOMPARE(manager.linkTargetSetting(sourceOpacity), color->settings()->find("opacity"));
  QVERIFY(!manager.linkTargetSetting(LinkTarget::of(LinkTarget::LayerElement, layer->getId(), "nothing")));

  // A deleted layer makes its links do nothing; they are kept in the table.
  manager.links().add(keyLink("M", layer->getId(), "visible"));
  manager.removeLayer(layer->getId());
  QVERIFY(!manager.linkTargetSetting(eye));
  QCOMPARE(manager.links().count(), 1);

  manager.clearAll();
  QVERIFY(manager.links().isEmpty());
}

void TestLinks::windowListsLinks()
{
  MappingManager manager;
  Source::ptr color(new Color());
  manager.addSource(color);
  Layer::ptr layer = makeColorLayer(color);
  layer->setName("Facade");
  manager.addLayer(layer);

  LinksWindow window(&manager);
  QVERIFY(!window.table()->isVisibleTo(&window));

  manager.links().add(keyLink("M", layer->getId(), "visible"));
  QTRY_COMPARE(window.table()->rowCount(), 1);
  QCOMPARE(window.table()->item(0, LinksWindow::TriggerColumn)->text(), QString("M"));
  QCOMPARE(window.table()->item(0, LinksWindow::KindColumn)->text(), QString("Keyboard"));
  QCOMPARE(window.table()->item(0, LinksWindow::ElementColumn)->text(), QString("Facade"));
  QCOMPARE(window.table()->item(0, LinksWindow::SettingColumn)->text(), QString("Visible"));

  // A link to a deleted layer is listed as such.
  manager.removeLayer(layer->getId());
  window.refresh();
  QCOMPARE(window.table()->item(0, LinksWindow::ElementColumn)->text(), QString("(deleted)"));

  // The button of the row removes the link.
  QAbstractButton* remove = qobject_cast<QAbstractButton*>(window.table()->cellWidget(0, LinksWindow::RemoveColumn));
  QVERIFY(remove);
  remove->click();
  QVERIFY(manager.links().isEmpty());
  QTRY_COMPARE(window.table()->rowCount(), 0);
}
