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

/// Number of vertices of a mesh that the panel shows: its corners.
const int MESH_CORNERS = 4;

/// Row of the last corner of the test mesh, named after its index.
const QString LAST_POINT_LABEL = QStringLiteral("Point 15");

/// The same row once the mesh has FEWEST_COLUMNS columns.
const QString LAST_POINT_LABEL_FEWEST_COLUMNS = QStringLiteral("Point 7");

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

/// Lists the given sources to the layers, and finds them by id, while it exists; the list
/// can change meanwhile.
struct SourceListing {
  explicit SourceListing(const QVector<Source::ptr>& initial)
    : sources(new QVector<Source::ptr>(initial))
  {
    std::shared_ptr<QVector<Source::ptr> > listed = sources;
    Layer::setSourceLister([listed]() { return *listed; });
    Layer::setSourceResolver([listed](uid id) {
      for (const Source::ptr& source : *listed)
        if (source->getId() == id)
          return source;
      return Source::ptr();
    });
  }

  ~SourceListing()
  {
    Layer::setSourceLister(Layer::SourceLister());
    Layer::setSourceResolver(Layer::SourceResolver());
  }

  std::shared_ptr<QVector<Source::ptr> > sources;
};

/// Returns the rows under the group of rows with a label.
QList<QtProperty*> rowsOf(const QList<QtProperty*>& rows, const QString& label)
{
  QtProperty* group = findRow(rows, label);
  return group ? group->subProperties() : QList<QtProperty*>();
}

} // namespace

void TestSettingsBrowser::colorSourceRows()
{
  Color source { QColor(Qt::red) };
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
  QCOMPARE(shown(opacity->value()), QString("100.0"));
  QCOMPARE(opacity->toolTip(), source.settings()->find("opacity")->info().description());

  QtVariantProperty* color = findRow(rows, "Color");
  QCOMPARE(color->propertyType(), int(QMetaType::QColor));
  QCOMPARE(color->value().value<QColor>(), QColor(Qt::red));
}

void TestSettingsBrowser::editWritesTreeOnce()
{
  Color source { QColor(Qt::red) };
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
  Color source { QColor(Qt::red) };
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
  // The group of a shape starts folded, and so does its group of points.
  QVERIFY(!panel.browser.isExpanded(panel.browser.items(output).first()));

  QtProperty* points = findRow(output->subProperties(), "Points");
  // Only the corners of a mesh have a row, named after their index in the list.
  QCOMPARE(labels(points->subProperties()), QStringList({ "Point 0", "Point 3", "Point 12", "Point 15" }));
  const qreal farSide = (MESH_SIZE - 1) * MESH_SPACING;
  QCOMPARE(static_cast<QtVariantProperty*>(points->subProperties().last())->value().toPointF(),
           QPointF(farSide, farSide));

  // A vertex inside the mesh has no row, and is still reached by its path.
  const QPointF moved(123, 45);
  QVERIFY(layer.settings()->find("output/vertices/5")->setValue(moved));
  QCOMPARE(layer.getShape()->getVertex(5), moved);
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
  QCOMPARE(points->subProperties().size(), MESH_CORNERS);
  QCOMPARE(points->subProperties().last()->propertyName(), LAST_POINT_LABEL_FEWEST_COLUMNS);
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
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().last()->propertyName(), LAST_POINT_LABEL);
  QCoreApplication::processEvents();

  output = findRow(panel.browser.properties(), "output");
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().last()->propertyName(), LAST_POINT_LABEL_FEWEST_COLUMNS);
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
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().last()->propertyName(), LAST_POINT_LABEL);
  QCoreApplication::processEvents();
  output = findRow(panel.browser.properties(), "output");
  QCOMPARE(findRow(output->subProperties(), "Points")->subProperties().last()->propertyName(), LAST_POINT_LABEL_FEWEST_COLUMNS);
}

void TestSettingsBrowser::mediaSourceRows()
{
  Image image;
  Panel imagePanel(image.settings());
  QCOMPARE(labels(imagePanel.browser.properties()), QStringList({ "ID", "Opacity (%)", "Image file", "Speed (%)" }));

  Video video;
  Panel videoPanel(video.settings());
  QCOMPARE(labels(videoPanel.browser.properties()),
           QStringList({ "ID", "Opacity (%)", "Source", "Speed (%)", "Volume (%)" }));
  QtVariantProperty* volume = findRow(videoPanel.browser.properties(), "Volume (%)");
  QCOMPARE(volume->attributeValue("minimum").toDouble(), 0.0);

  // A number typed above the maximum is kept at the maximum, and the row shows it.
  volume->setValue(150.0);
  QCOMPARE(video.getVolume(), 1.0);
  QCOMPARE(shown(volume->value()), QString("100.0"));
}

void TestSettingsBrowser::filePathRowHasFilter()
{
  Image image;
  Panel panel(image.settings());
  QtVariantProperty* file = findRow(panel.browser.properties(), "Image file");
  QCOMPARE(file->propertyType(), VariantManager::filePathTypeId());
  const QString filter = image.settings()->find("uri")->info().fileFilter();
  QVERIFY(filter.endsWith(";;All files (*)"));
  QCOMPARE(file->attributeValue("filter").toString(), filter);
}

void TestSettingsBrowser::integerRowHasBounds()
{
  SettingGroup root;
  const int lowest = 2;
  const int highest = 12;
  const double step = 2;
  Setting* steps = root.addSetting(SettingInfo::integer("steps", "Steps").bounds(lowest, highest).step(step)
                                   .defaultValue(lowest));
  Panel panel(&root);

  QtVariantProperty* row = findRow(panel.browser.properties(), "Steps");
  QCOMPARE(row->propertyType(), int(QMetaType::Int));
  QCOMPARE(row->attributeValue("minimum").toInt(), lowest);
  QCOMPARE(row->attributeValue("singleStep").toInt(), int(step));

  // The row lets a number above the maximum be typed: it is kept at the maximum.
  const int tooHigh = 33;
  row->setValue(tooHigh);
  QCOMPARE(steps->value().toInt(), highest);
  QCOMPARE(row->value().toInt(), highest);
  row->setValue(lowest);

  row->setValue(highest);
  QCOMPARE(steps->value().toInt(), highest);
}

void TestSettingsBrowser::booleanRowWritesTree()
{
  SettingGroup root;
  Setting* loop = root.addSetting(SettingInfo::boolean("loop", "Loop"));
  Panel panel(&root);

  QtVariantProperty* row = findRow(panel.browser.properties(), "Loop");
  QCOMPARE(row->propertyType(), int(QMetaType::Bool));
  QCOMPARE(row->value().toBool(), false);
  row->setValue(true);
  QCOMPARE(loop->value().toBool(), true);

  loop->setValue(false);
  panel.settings->refresh("loop", false);
  QCOMPARE(row->value().toBool(), false);
}

void TestSettingsBrowser::pointRowWritesTree()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());
  QSignalSpy changes(&layer, &Element::settingChanged);
  QSignalSpy edits(panel.settings.get(), &SettingsBrowser::edited);

  QtVariantProperty* first = findRow(rowsOf(rowsOf(panel.browser.properties(), "output"), "Points"), "Point 0");
  QCOMPARE(first->propertyType(), int(QMetaType::QPointF));
  const QPointF moved(12.5, 7.5);
  first->setValue(moved);
  QCOMPARE(layer.getShape()->getVertex(0), moved);
  QCOMPARE(changes.count(), 1);
  QCOMPARE(edits.count(), 1);

  // A vertex moved elsewhere (the canvas) shows in its row.
  const QPointF dragged(40, 30);
  layer.getShape()->setVertex(0, dragged);
  QCOMPARE(first->value().toPointF(), dragged);
  QCOMPARE(edits.count(), 1);
}

void TestSettingsBrowser::referenceRowWritesSourceId()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  red->setName("red");
  Source::ptr blue(new Color(QColor(Qt::blue)));
  blue->setName("blue");
  Source::ptr image(new Image);
  image->setName("image");
  SourceListing listing({ red, image, blue });
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());

  // Only the compatible sources are offered; choosing one writes its id.
  QtVariantProperty* source = findRow(panel.browser.properties(), "Source");
  QCOMPARE(source->attributeValue("enumNames").toStringList(), QStringList({ "red", "blue" }));
  source->setValue(1);
  QCOMPARE(layer.getSource(), blue);
  QCOMPARE(layer.settings()->find("source_id")->value().toInt(), int(blue->getId()));
  QCOMPARE(source->value().toInt(), 1);
}

void TestSettingsBrowser::sourceMenuFollowsSourceList()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  red->setName("red");
  SourceListing listing({ red });
  ColorLayer layer(red, makeMesh());
  Panel panel(layer.settings());
  QtVariantProperty* source = findRow(panel.browser.properties(), "Source");

  // What LayerGui::updateSources() does when MainWindow says the list changed.
  Source::ptr green(new Color(QColor(Qt::green)));
  green->setName("green");
  listing.sources->append(green);
  panel.settings->refresh("source_id", QVariant());
  QCOMPARE(source->attributeValue("enumNames").toStringList(), QStringList({ "red", "green" }));

  green->setName("lime");
  panel.settings->refresh("source_id", QVariant());
  QCOMPARE(source->attributeValue("enumNames").toStringList(), QStringList({ "red", "lime" }));

  listing.sources->removeLast();
  panel.settings->refresh("source_id", QVariant());
  QCOMPARE(source->attributeValue("enumNames").toStringList(), QStringList({ "red" }));
  QCOMPARE(source->value().toInt(), 0);
}

void TestSettingsBrowser::rangeRowScalesLikeNumber()
{
  SettingGroup root;
  const double scale = 100;
  Setting* window = root.addSetting(SettingInfo::range("window", "Window").bounds(0.0, 1.0)
                                    .unit(SettingInfo::Percent).displayScale(scale)
                                    .defaultValue(QPointF(0.2, 0.6)));
  Panel panel(&root);

  // The label gives the unit; both parts are shown in it.
  QtProperty* group = findRow(panel.browser.properties(), "Window (%)");
  QVERIFY(group);
  QtVariantProperty* low = findRow(group->subProperties(), "Low");
  QtVariantProperty* high = findRow(group->subProperties(), "High");
  QCOMPARE(shown(low->value()), QString("20.0"));
  QCOMPARE(shown(high->value()), QString("60.0"));

  // A bound typed above the maximum is kept at the maximum.
  high->setValue(150.0);
  QCOMPARE(shown(window->value().toPointF().y() * scale), QString("100.0"));
  QCOMPARE(shown(high->value()), QString("100.0"));

  high->setValue(80.0);
  QCOMPARE(shown(window->value().toPointF().y() * scale), QString("80.0"));
  QCOMPARE(shown(window->value().toPointF().x() * scale), QString("20.0"));
}

void TestSettingsBrowser::textureMeshPanelKeepsGridsInStep()
{
  Source::ptr image(new Image);
  TextureLayer layer(image, makeMesh(), makeMesh());
  Panel panel(layer.settings());
  follow(&layer, panel.settings.get());
  QCOMPARE(labels(panel.browser.properties()), QStringList({ "ID", "Opacity (%)", "Source", "output", "input" }));

  findRow(rowsOf(panel.browser.properties(), "output"), "Columns")->setValue(FEWEST_COLUMNS);
  QCoreApplication::processEvents();

  // Both groups show the new grid of both meshes.
  for (const QString& group : { QString("output"), QString("input") }) {
    const QList<QtProperty*> rows = rowsOf(panel.browser.properties(), group);
    QCOMPARE(findRow(rows, "Columns")->value().toInt(), FEWEST_COLUMNS);
    QCOMPARE(rowsOf(rows, "Points").size(), MESH_CORNERS);
  }
}
