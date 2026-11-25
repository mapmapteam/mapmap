/*
 * TestSettingsBrowser.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestSettingsBrowser.h"

#include <memory>

#include <QColor>
#include <QPointF>
#include <QSignalSpy>
#include <QVector>

#include "Layer.h"
#include "Mesh.h"
#include "Setting.h"
#include "SettingsBrowser.h"
#include "Source.h"

#include "qttreepropertybrowser.h"
#include "qtvariantproperty.h"
#include "variantmanager.h"

using namespace mmp;

namespace {

/// Number of vertices on each side of the test mesh.
const int MESH_SIZE = 4;

/// Distance between two neighbouring vertices of that mesh, in pixels.
const qreal MESH_SPACING = 100;

/// Fewest columns of a mesh.
const int FEWEST_COLUMNS = 2;

/// Returns a mesh of MESH_SIZE x MESH_SIZE vertices, MESH_SPACING apart.
MShape::ptr makeMesh()
{
  QVector<QPointF> points;
  for (int row = 0; row < MESH_SIZE; row++)
    for (int column = 0; column < MESH_SIZE; column++)
      points.append(QPointF(column * MESH_SPACING, row * MESH_SPACING));
  return MShape::ptr(new Mesh(points, MESH_SIZE, MESH_SIZE));
}

/// A property browser filled from a tree; members are destroyed in the reverse order.
struct Panel {
  explicit Panel(SettingGroup* root)
    : settings(new SettingsBrowser(root, &browser, &manager))
  {
  }

  QtTreePropertyBrowser browser;
  VariantManager manager;
  std::unique_ptr<SettingsBrowser> settings;
};

/// Keeps the panel in step with an element, as MainWindow does.
void follow(Element* element, SettingsBrowser* settings)
{
  QObject::connect(element, &Element::settingChanged, settings,
                   [settings](uid, const QString& path, const QVariant& value) { settings->refresh(path, value); });
  QObject::connect(element, &Element::settingsStructureChanged, settings, &SettingsBrowser::rebuild);
}

/// Returns the labels of rows.
QStringList labels(const QList<QtProperty*>& rows)
{
  QStringList result;
  for (QtProperty* row : rows)
    result << row->propertyName();
  return result;
}

/// Returns the row with a label, or nullptr.
QtVariantProperty* findRow(const QList<QtProperty*>& rows, const QString& label)
{
  for (QtProperty* row : rows)
    if (row->propertyName() == label)
      return static_cast<QtVariantProperty*>(row);
  return nullptr;
}

/// Returns a number as a number row shows it.
QString shown(const QVariant& number)
{
  return QString::number(number.toDouble(), 'f', SettingsBrowser::PANEL_DECIMALS);
}

/// Lists the given sources to the layers while it exists.
struct SourceListing {
  explicit SourceListing(const QVector<Source::ptr>& sources)
  {
    Layer::setSourceLister([sources]() { return sources; });
  }

  ~SourceListing()
  {
    Layer::setSourceLister(Layer::SourceLister());
  }
};

} // namespace

void TestSettingsBrowser::colorSourceRows()
{
  Color source(QColor(Qt::red));
  Panel panel(source.settings());

  // Hidden settings (name, locked) have no row.
  const QList<QtProperty*> rows = panel.browser.properties();
  QCOMPARE(labels(rows), QStringList({ "ID", "Opacity (%)", "Color" }));

  QtVariantProperty* id = findRow(rows, "ID");
  QCOMPARE(id->propertyType(), int(QMetaType::Int));
  QCOMPARE(id->value().toInt(), int(source.getId()));
  QVERIFY(!id->isEnabled());

  QtVariantProperty* opacity = findRow(rows, "Opacity (%)");
  QCOMPARE(opacity->propertyType(), int(QMetaType::Double));
  QVERIFY(opacity->isEnabled());
  QCOMPARE(opacity->attributeValue("decimals").toInt(), SettingsBrowser::PANEL_DECIMALS);
  QCOMPARE(opacity->attributeValue("minimum").toDouble(), 0.0);
  QCOMPARE(opacity->attributeValue("maximum").toDouble(), 100.0);
  QCOMPARE(shown(opacity->value()), QString("100.0"));
  QCOMPARE(opacity->toolTip(), source.settings()->find("opacity")->info().description());

  QtVariantProperty* color = findRow(rows, "Color");
  QCOMPARE(color->propertyType(), int(QMetaType::QColor));
  QCOMPARE(color->value().value<QColor>(), QColor(Qt::red));
}

void TestSettingsBrowser::editWritesTreeOnce()
{
  Color source(QColor(Qt::red));
  Panel panel(source.settings());
  follow(&source, panel.settings.get());
  QSignalSpy changes(&source, &Element::settingChanged);
  QSignalSpy edits(panel.settings.get(), &SettingsBrowser::edited);

  // The value comes back through refresh(), which does not write it again.
  findRow(panel.browser.properties(), "Opacity (%)")->setValue(50.0);
  QCOMPARE(source.getOpacity(), 0.5f);
  QCOMPARE(changes.count(), 1);
  QCOMPARE(edits.count(), 1);

  findRow(panel.browser.properties(), "Color")->setValue(QColor(Qt::blue));
  QCOMPARE(source.getColor(), QColor(Qt::blue));
  QCOMPARE(changes.count(), 2);
  QCOMPARE(edits.count(), 2);
}

void TestSettingsBrowser::refreshDoesNotRewriteRow()
{
  Color source(QColor(Qt::red));
  Panel panel(source.settings());
  follow(&source, panel.settings.get());
  QSignalSpy rowWrites(&panel.manager, &QtVariantPropertyManager::valueChanged);
  QSignalSpy changes(&source, &Element::settingChanged);
  QSignalSpy edits(panel.settings.get(), &SettingsBrowser::edited);

  // 0.3f is shown as 30.0000011920929 by the tree: the row compares at its precision.
  source.setOpacity(0.3f);
  QtVariantProperty* opacity = findRow(panel.browser.properties(), "Opacity (%)");
  QCOMPARE(shown(opacity->value()), QString("30.0"));
  QCOMPARE(rowWrites.count(), 1);

  panel.settings->refresh("opacity", source.settings()->find("opacity")->value());
  QCOMPARE(rowWrites.count(), 1);
  QCOMPARE(changes.count(), 1);
  QCOMPARE(edits.count(), 0);

  // An edit comes back widened (40.0000005960464): the row keeps what the user entered.
  opacity->setValue(40.0);
  QCOMPARE(source.getOpacity(), 0.4f);
  QCOMPARE(opacity->value().toDouble(), 40.0);
  QCOMPARE(rowWrites.count(), 2);
  QCOMPARE(changes.count(), 2);
  QCOMPARE(edits.count(), 1);

  // Out of bounds over OSC: the tree clamps, the row shows what it kept.
  QVERIFY(source.settings()->find("opacity")->setFromArguments({ 5.0 }));
  QCOMPARE(shown(opacity->value()), QString("100.0"));
  QCOMPARE(edits.count(), 1);
}

void TestSettingsBrowser::unavailableRowIsDisabled()
{
  SettingGroup root;
  Setting* gain = root.addSetting(SettingInfo::number("gain", "Gain"));
  root.addSetting(SettingInfo::boolean("secret", "Secret").hidden());
  root.addSetting(SettingInfo::text("note", "Note").readOnly());
  Panel panel(&root);

  QCOMPARE(labels(panel.browser.properties()), QStringList({ "Gain", "Note" }));
  QVERIFY(findRow(panel.browser.properties(), "Gain")->isEnabled());
  QVERIFY(!findRow(panel.browser.properties(), "Note")->isEnabled());

  gain->setAvailable(false);
  panel.settings->rebuild();
  QVERIFY(!findRow(panel.browser.properties(), "Gain")->isEnabled());
}

void TestSettingsBrowser::choiceKeepsMissingId()
{
  SettingGroup root;
  Setting* mode = root.addSetting(SettingInfo::choice("mode", "Mode", { { "add", "Add" }, { "multiply", "Multiply" } })
                                  .defaultValue("screen"));
  Panel panel(&root);

  QtVariantProperty* row = findRow(panel.browser.properties(), "Mode");
  QCOMPARE(row->propertyType(), QtVariantPropertyManager::enumTypeId());
  QCOMPARE(row->attributeValue("enumNames").toStringList(), QStringList({ "Add", "Multiply", "screen" }));
  QCOMPARE(row->value().toInt(), 2);

  // The menu gives a label, the tree keeps the id.
  row->setValue(1);
  QCOMPARE(mode->value().toString(), QString("multiply"));
  QCOMPARE(row->attributeValue("enumNames").toStringList(), QStringList({ "Add", "Multiply" }));
  QCOMPARE(row->value().toInt(), 1);
}

void TestSettingsBrowser::layerRowsAndFolds()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  red->setName("red");
  SourceListing listing({ red });
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  panel.settings->setGroupLabel("output", "Output shape");

  const QList<QtProperty*> rows = panel.browser.properties();
  QCOMPARE(labels(rows), QStringList({ "ID", "Opacity (%)", "Source", "Output shape" }));

  QtVariantProperty* source = findRow(rows, "Source");
  QCOMPARE(source->attributeValue("enumNames").toStringList(), QStringList({ "red" }));
  QCOMPARE(source->value().toInt(), 0);

  // Display groups come after the other rows of their group.
  QtProperty* output = findRow(rows, "Output shape");
  QCOMPARE(labels(output->subProperties()), QStringList({ "Columns", "Rows", "Points" }));
  QVERIFY(panel.browser.isExpanded(panel.browser.items(output).first()));

  QtProperty* points = findRow(output->subProperties(), "Points");
  QCOMPARE(points->subProperties().size(), MESH_SIZE * MESH_SIZE);
  QCOMPARE(points->subProperties().first()->propertyName(), QString("Point 0"));
  QCOMPARE(static_cast<QtVariantProperty*>(points->subProperties().at(5))->value().toPointF(),
           QPointF(MESH_SPACING, MESH_SPACING));
  QVERIFY(!panel.browser.isExpanded(panel.browser.items(points).first()));
}

void TestSettingsBrowser::rebuildFollowsShrinkingList()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());

  Mesh* mesh = static_cast<Mesh*>(layer.getShape().data());
  mesh->setNColumns(FEWEST_COLUMNS);

  QtProperty* output = findRow(panel.browser.properties(), "output");
  QtProperty* points = findRow(output->subProperties(), "Points");
  QCOMPARE(points->subProperties().size(), FEWEST_COLUMNS * MESH_SIZE);
  QCOMPARE(findRow(output->subProperties(), "Columns")->value().toInt(), FEWEST_COLUMNS);

  // The rows made by the rebuild follow the vertices.
  mesh->setVertex(0, QPointF(5, 5));
  QCOMPARE(static_cast<QtVariantProperty*>(points->subProperties().first())->value().toPointF(), QPointF(5, 5));
}

void TestSettingsBrowser::editThatShrinksListRebuildsLater()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());
  QSignalSpy edits(panel.settings.get(), &SettingsBrowser::edited);

  QtProperty* output = findRow(panel.browser.properties(), "output");
  findRow(output->subProperties(), "Columns")->setValue(FEWEST_COLUMNS);
  Mesh* mesh = static_cast<Mesh*>(layer.getShape().data());
  QCOMPARE(mesh->nColumns(), FEWEST_COLUMNS);
  QCOMPARE(edits.count(), 1);

  // The row that sent the value still exists until the event loop runs.
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().size(), MESH_SIZE * MESH_SIZE);
  QCoreApplication::processEvents();

  output = findRow(panel.browser.properties(), "output");
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().size(), FEWEST_COLUMNS * MESH_SIZE);
  QCOMPARE(findRow(output->subProperties(), "Columns")->value().toInt(), FEWEST_COLUMNS);
  QCOMPARE(edits.count(), 1);
}

void TestSettingsBrowser::openFoldStaysOpen()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());

  QtProperty* output = findRow(panel.browser.properties(), "output");
  QtProperty* points = findRow(output->subProperties(), "Points");
  panel.browser.setExpanded(panel.browser.items(points).first(), true);

  panel.settings->rebuild();
  output = findRow(panel.browser.properties(), "output");
  points = findRow(output->subProperties(), "Points");
  QVERIFY(panel.browser.isExpanded(panel.browser.items(points).first()));
}

void TestSettingsBrowser::rebuildFromEditedWaitsForEditToEnd()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());

  // A slot of edited() changes the structure while the row is still sending its value.
  Mesh* mesh = static_cast<Mesh*>(layer.getShape().data());
  QObject::connect(panel.settings.get(), &SettingsBrowser::edited, panel.settings.get(),
                   [mesh]() { mesh->setNColumns(FEWEST_COLUMNS); });

  findRow(panel.browser.properties(), "Opacity (%)")->setValue(50.0);
  QCOMPARE(layer.getOpacity(), 0.5f);
  QCOMPARE(mesh->nColumns(), FEWEST_COLUMNS);

  // The rows are rebuilt from the event loop, once the edit has ended.
  QtProperty* output = findRow(panel.browser.properties(), "output");
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().size(), MESH_SIZE * MESH_SIZE);
  QCoreApplication::processEvents();
  output = findRow(panel.browser.properties(), "output");
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().size(), FEWEST_COLUMNS * MESH_SIZE);
}
