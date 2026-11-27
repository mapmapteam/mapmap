/*
 * TestInspector.cpp
 *
 * (c) 2026 Sofian Audry -- info(@)sofianaudry(.)com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestInspector.h"

#include <memory>

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

#include "ColorPicker.h"
#include "Inspector.h"
#include "Layer.h"
#include "Mesh.h"
#include "Setting.h"
#include "SettingControl.h"
#include "Source.h"

using namespace mmp;

namespace {

/// No-break space, written before most units.
const QString NBSP = QString(QChar(0x00A0));

/// Width of reference of the inspector, in pixels.
const int REFERENCE_WIDTH = 360;

/// Height given to the inspector in tests, in pixels.
const int TEST_HEIGHT = 700;

/// Number of vertices on each side of the test mesh.
const int MESH_SIZE = 4;

/// Distance between two neighbouring vertices of that mesh, in pixels.
const qreal MESH_SPACING = 100;

/// Returns a mesh of MESH_SIZE x MESH_SIZE vertices, MESH_SPACING apart.
MShape::ptr makeMesh()
{
  QVector<QPointF> points;
  for (int row = 0; row < MESH_SIZE; row++)
    for (int column = 0; column < MESH_SIZE; column++)
      points.append(QPointF(column * MESH_SPACING, row * MESH_SPACING));
  return MShape::ptr(new Mesh(points, MESH_SIZE, MESH_SIZE));
}

/// Lists the given sources to the layers while it exists; the list can change meanwhile.
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

/// An opacity as Element declares it: 0..1 shown in %, modulable.
SettingInfo opacityInfo()
{
  return SettingInfo::number("opacity", "Opacity").bounds(0.0, 1.0).defaultValue(1.0)
      .unit(SettingInfo::Percent).displayScale(100.0).modulable();
}

/// Types text in a field, replacing what it shows, and presses Enter.
void typeIn(QLineEdit* field, const QString& text)
{
  field->setFocus();
  field->selectAll();
  QTest::keyClicks(field, text);
  QTest::keyClick(field, Qt::Key_Return);
}

/// Sends a mouse event to a widget, with modifiers.
void sendMouse(QWidget* widget, QEvent::Type type, const QPoint& point, Qt::MouseButtons buttons,
               Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
  QMouseEvent event(type, QPointF(point), QPointF(widget->mapToGlobal(point)),
                    type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons, modifiers);
  QCoreApplication::sendEvent(widget, &event);
}

} // namespace

void TestInspector::initTestCase()
{
  // Keep the preferences of the tests away from the user's.
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName(QStringLiteral("MapMapTests"));
  QCoreApplication::setApplicationName(QStringLiteral("TestInspector"));
  _previousLocale = QLocale();
  QLocale::setDefault(QLocale(QLocale::French, QLocale::Canada));
}

void TestInspector::cleanupTestCase()
{
  QSettings().clear();
  QLocale::setDefault(_previousLocale);
}

void TestInspector::init()
{
  QSettings().clear();
}

void TestInspector::controlForEachType()
{
  SettingGroup root;
  root.addSetting(SettingInfo::number("bounded", "Bounded").bounds(0.0, 1.0));
  root.addSetting(SettingInfo::number("open", "Open"));
  root.addSetting(SettingInfo::integer("count", "Count").bounds(1, 10));
  root.addSetting(SettingInfo::boolean("on", "On"));
  root.addSetting(SettingInfo::choice("blend", "Blend", { { "normal", "Normal" }, { "add", "Add" } })
                  .defaultValue("normal"));
  root.addSetting(SettingInfo::text("label", "Label"));
  root.addSetting(SettingInfo::filePath("file", "File", "Images (*.png)"));
  root.addSetting(SettingInfo::point("spot", "Spot").bounds(0.0, 1.0));
  root.addSetting(SettingInfo::point("vertex", "Vertex").unit(SettingInfo::Pixels));
  root.addSetting(SettingInfo::range("span", "Span").bounds(0.0, 10.0));
  root.addSetting(SettingInfo::color("tint", "Tint"));
  InspectorPage page(&root, "test");

  NumberControl* bounded = qobject_cast<NumberControl*>(page.control("bounded"));
  QVERIFY(bounded);
  QVERIFY(bounded->slider());
  NumberControl* open = qobject_cast<NumberControl*>(page.control("open"));
  QVERIFY(open);
  QVERIFY(!open->slider());
  QVERIFY(qobject_cast<NumberControl*>(page.control("count")));
  QVERIFY(qobject_cast<SwitchControl*>(page.control("on")));
  QVERIFY(qobject_cast<ChoiceControl*>(page.control("blend")));
  QVERIFY(qobject_cast<TextControl*>(page.control("label")));
  QVERIFY(qobject_cast<TextControl*>(page.control("file")));
  PointControl* spot = qobject_cast<PointControl*>(page.control("spot"));
  QVERIFY(spot);
  QVERIFY(spot->pad());
  PointControl* vertex = qobject_cast<PointControl*>(page.control("vertex"));
  QVERIFY(vertex);
  QVERIFY(!vertex->pad());
  RangeControl* span = qobject_cast<RangeControl*>(page.control("span"));
  QVERIFY(span);
  QVERIFY(span->bar());
  QVERIFY(qobject_cast<ColorControl*>(page.control("tint")));
  QCOMPARE(page.controls().size(), 11);
}

void TestInspector::colorSourceRows()
{
  Color red{QColor(Qt::red)};
  InspectorPage page(red.settings(), "source");
  // Hidden settings have no row; the id is read-only.
  QVERIFY(!page.control("name"));
  QVERIFY(!page.control("locked"));
  QVERIFY(page.control("id"));
  QVERIFY(!page.control("id")->isEnabled());
  QVERIFY(qobject_cast<NumberControl*>(page.control("opacity")));
  QVERIFY(qobject_cast<ColorControl*>(page.control("color")));
  QCOMPARE(qobject_cast<NumberControl*>(page.control("opacity"))->field()->text(),
           QString("100") + NBSP + "%");
}

void TestInspector::arrowsMoveByStepsShiftByTen()
{
  SettingGroup root;
  Setting* count = root.addSetting(SettingInfo::integer("count", "Count").bounds(0, 100).defaultValue(5));
  InspectorPage page(&root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page.control("count"));
  QVERIFY(control);
  QSignalSpy edited(&page, &InspectorPage::edited);

  QTest::keyClick(control->field(), Qt::Key_Up);
  QCOMPARE(count->value().toInt(), 6);
  QTest::keyClick(control->field(), Qt::Key_Up, Qt::ShiftModifier);
  QCOMPARE(count->value().toInt(), 16);
  QTest::keyClick(control->field(), Qt::Key_Down);
  QCOMPARE(count->value().toInt(), 15);
  QCOMPARE(control->field()->text(), QString("15"));
  QCOMPARE(edited.count(), 3);
}

void TestInspector::openBoundHasSliderRange()
{
  SettingGroup root;
  Setting* speed = root.addSetting(SettingInfo::number("rate", "Speed")
                                     .defaultValue(1.0)
                                     .sliderRange(0.0, 2.0)
                                     .unit(SettingInfo::Percent)
                                     .displayScale(100.0));
  InspectorPage page(&root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page.control("rate"));
  QVERIFY(control);
  QVERIFY(control->slider());
  QCOMPARE(control->slider()->position(), 0.5);

  emit control->slider()->moved(0.75);
  QCOMPARE(speed->value().toDouble(), 1.5);

  // Typed values go beyond the slider; the handle stays at the end.
  typeIn(control->field(), "400");
  QCOMPARE(speed->value().toDouble(), 4.0);
  QCOMPARE(control->slider()->position(), 1.0);
  typeIn(control->field(), "-50");
  QCOMPARE(speed->value().toDouble(), -0.5);
  QCOMPARE(control->slider()->position(), 0.0);

  // Without bounds nor slider range: a field alone.
  Setting* free = root.addSetting(SettingInfo::number("free", "Free"));
  Q_UNUSED(free);
  InspectorPage otherPage(&root, "test");
  NumberControl* freeControl = qobject_cast<NumberControl*>(otherPage.control("free"));
  QVERIFY(freeControl);
  QVERIFY(!freeControl->slider());
}

void TestInspector::controlsUseMapMapColors()
{
  Inspector inspector;
  SettingGroup root;
  root.addSetting(opacityInfo());
  InspectorPage* page = inspector.addPage("Test", &root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page->control("opacity"));
  QVERIFY(control);
  // The white of MapMap's style sheet, not the highlight colour of the system.
  QCOMPARE(control->slider()->palette().color(QPalette::Highlight), QColor(0xf6, 0xf5, 0xf5));
  QCOMPARE(control->field()->palette().color(QPalette::Base), QColor(0x27, 0x2a, 0x36));
}

void TestInspector::typedCommaIsAccepted()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(opacityInfo());
  InspectorPage page(&root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page.control("opacity"));
  QVERIFY(control);

  typeIn(control->field(), "33,5");
  QCOMPARE(opacity->value().toDouble(), 0.335);
  // Shown at the precision of the panel.
  QCOMPARE(control->field()->text(), QString("34") + NBSP + "%");
  QVERIFY(qAbs(control->slider()->position() - 0.335) < 1e-9);
}

void TestInspector::refusedTextShowsValueAgain()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(opacityInfo());
  opacity->setValue(0.5);
  InspectorPage page(&root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page.control("opacity"));

  typeIn(control->field(), "beaucoup");
  QCOMPARE(opacity->value().toDouble(), 0.5);
  QCOMPARE(control->field()->text(), QString("50") + NBSP + "%");

  // Above the maximum: kept at the maximum.
  typeIn(control->field(), "250");
  QCOMPARE(opacity->value().toDouble(), 1.0);
}

void TestInspector::labelDragAdjustsShiftIsFine()
{
  SettingGroup root;
  Setting* count = root.addSetting(SettingInfo::integer("count", "Count").bounds(0, 100).defaultValue(50));
  InspectorPage page(&root, "test");
  InspectorRow* row = page.row("count");
  QVERIFY(row);
  InspectorRowLabel* label = row->label();

  sendMouse(label, QEvent::MouseButtonPress, QPoint(5, 5), Qt::LeftButton);
  sendMouse(label, QEvent::MouseMove, QPoint(15, 5), Qt::LeftButton);
  QCOMPARE(count->value().toInt(), 60);
  // Shift: a tenth of a step per pixel.
  sendMouse(label, QEvent::MouseMove, QPoint(25, 5), Qt::LeftButton, Qt::ShiftModifier);
  QCOMPARE(count->value().toInt(), 61);
  sendMouse(label, QEvent::MouseMove, QPoint(5, 5), Qt::LeftButton);
  QCOMPARE(count->value().toInt(), 41);
  sendMouse(label, QEvent::MouseButtonRelease, QPoint(5, 5), Qt::NoButton);
}

void TestInspector::doubleClickResets()
{
  SettingGroup root;
  Setting* amount = root.addSetting(SettingInfo::number("amount", "Amount").bounds(0.0, 1.0).defaultValue(0.25));
  amount->setValue(0.8);
  InspectorPage page(&root, "test");
  QSignalSpy edited(&page, &InspectorPage::edited);

  QTest::mouseDClick(page.row("amount"), Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
  QCOMPARE(amount->value().toDouble(), 0.25);
  QCOMPARE(edited.count(), 1);
}

void TestInspector::readOnlyRowIsDisabled()
{
  SettingGroup root;
  Setting* fixed = root.addSetting(SettingInfo::number("fixed", "Fixed").readOnly());
  Setting* other = root.addSetting(SettingInfo::number("other", "Other"));
  InspectorPage page(&root, "test");
  QVERIFY(!page.control("fixed")->isEnabled());
  QVERIFY(page.control("other")->isEnabled());

  // A setting made unavailable by its owner is greyed out when the page shows it again.
  other->setAvailable(false);
  page.refresh("other");
  QVERIFY(!page.control("other")->isEnabled());
  Q_UNUSED(fixed);
}

void TestInspector::switchToggles()
{
  SettingGroup root;
  Setting* on = root.addSetting(SettingInfo::boolean("on", "On"));
  InspectorPage page(&root, "test");
  SwitchControl* control = qobject_cast<SwitchControl*>(page.control("on"));
  control->resize(control->sizeHint());

  QTest::mouseClick(control, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
  QVERIFY(on->value().toBool());
  QVERIFY(control->isOn());
  QTest::keyClick(control, Qt::Key_Space);
  QVERIFY(!on->value().toBool());
}

void TestInspector::shortChoiceShowsButtons()
{
  SettingGroup root;
  Setting* blend = root.addSetting(SettingInfo::choice("blend", "Blend", { { "normal", "Normal" }, { "add", "Add" } })
                                   .defaultValue("normal"));
  root.addSetting(SettingInfo::choice("shape", "Shape",
                                      { { "a", "Circle" }, { "b", "Square" }, { "c", "Diamond" },
                                        { "d", "Triangle" }, { "e", "Star" } }).defaultValue("a"));
  InspectorPage page(&root, "test");
  ChoiceControl* short_ = qobject_cast<ChoiceControl*>(page.control("blend"));
  ChoiceControl* long_ = qobject_cast<ChoiceControl*>(page.control("shape"));
  QVERIFY(short_->showsButtons());
  QVERIFY(!long_->showsButtons());
  QVERIFY(long_->menu());

  QList<QPushButton*> buttons = short_->findChildren<QPushButton*>();
  QCOMPARE(buttons.size(), 2);
  QVERIFY(buttons.at(0)->isChecked());
  buttons.at(1)->click();
  QCOMPARE(blend->value().toString(), QString("add"));
  QVERIFY(buttons.at(1)->isChecked());
}

void TestInspector::choiceKeepsMissingId()
{
  SettingGroup root;
  Setting* blend = root.addSetting(SettingInfo::choice("blend", "Blend", { { "normal", "Normal" }, { "add", "Add" } }));
  QVERIFY(blend->setValue("gone"));
  InspectorPage page(&root, "test");
  ChoiceControl* control = qobject_cast<ChoiceControl*>(page.control("blend"));
  QVERIFY(!control->showsButtons());
  QCOMPARE(control->menu()->count(), 3);
  QCOMPARE(control->menu()->currentIndex(), 2);
  QVERIFY(control->menu()->currentText().contains("gone"));

  // Choosing a real entry writes its id; the missing entry then goes away.
  emit control->menu()->activated(1);
  QCOMPARE(blend->value().toString(), QString("add"));
  QCOMPARE(control->menu()->count(), 2);
}

void TestInspector::referenceMenuFollowsSources()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  red->setName("red");
  SourceListing listing({ red });
  ColorLayer layer(red, makeMesh());
  InspectorPage page(layer.settings(), "layer");
  ChoiceControl* source = qobject_cast<ChoiceControl*>(page.control("source_id"));
  QVERIFY(source);
  QCOMPARE(source->menu()->count(), 1);
  QCOMPARE(source->menu()->currentText(), QString("red"));

  Source::ptr blue(new Color(QColor(Qt::blue)));
  blue->setName("blue");
  listing.sources->append(blue);
  source->refresh();
  QCOMPARE(source->menu()->count(), 2);
  emit source->menu()->activated(1);
  QCOMPARE(layer.getSource(), blue);
}

void TestInspector::rangeBoundsDoNotCross()
{
  SettingGroup root;
  Setting* span = root.addSetting(SettingInfo::range("span", "Span").bounds(0.0, 10.0).step(1));
  span->setValue(QPointF(2, 5));
  InspectorPage page(&root, "test");
  RangeControl* control = qobject_cast<RangeControl*>(page.control("span"));

  typeIn(control->field(0), "8");
  QCOMPARE(span->value().toPointF(), QPointF(5, 5));
  typeIn(control->field(1), "1");
  QCOMPARE(span->value().toPointF(), QPointF(5, 5));
  typeIn(control->field(1), "9");
  QCOMPARE(span->value().toPointF(), QPointF(5, 9));
}

void TestInspector::pointFieldsWriteCoordinates()
{
  SettingGroup root;
  Setting* vertex = root.addSetting(SettingInfo::point("vertex", "Vertex").unit(SettingInfo::Pixels));
  vertex->setValue(QPointF(10, 20));
  InspectorPage page(&root, "test");
  PointControl* control = qobject_cast<PointControl*>(page.control("vertex"));

  QCOMPARE(control->field(0)->text(), QString("10") + NBSP + "px");
  typeIn(control->field(0), "120");
  QCOMPARE(vertex->value().toPointF(), QPointF(120, 20));
  QTest::keyClick(control->field(1), Qt::Key_Up);
  QCOMPARE(vertex->value().toPointF(), QPointF(120, 21));
}

void TestInspector::externalValueDoesNotRebuild()
{
  SettingGroup root;
  Setting* opacity = root.addSetting(opacityInfo());
  InspectorPage page(&root, "test");
  const int builds = page.buildCount();
  NumberControl* control = qobject_cast<NumberControl*>(page.control("opacity"));

  opacity->setValue(0.5);
  page.refresh("opacity");
  QCOMPARE(control->field()->text(), QString("50") + NBSP + "%");
  // A burst of values: the last one is shown after the interval, without making rows again.
  opacity->setValue(0.6);
  page.refresh("opacity");
  opacity->setValue(0.7);
  page.refresh("opacity");
  QTRY_COMPARE(control->field()->text(), QString("70") + NBSP + "%");
  QCOMPARE(page.buildCount(), builds);
  QCOMPARE(page.control("opacity"), control);
}

void TestInspector::structureChangeRebuildsLater()
{
  SettingGroup root;
  root.addSetting(SettingInfo::number("first", "First"));
  InspectorPage page(&root, "test");
  const int builds = page.buildCount();

  root.addSetting(SettingInfo::number("second", "Second"));
  page.rebuild();
  page.rebuild();
  QCOMPARE(page.buildCount(), builds);
  QTRY_COMPARE(page.buildCount(), builds + 1);
  QVERIFY(page.control("second"));
}

void TestInspector::foldedGroupStaysFolded()
{
  SettingGroup root;
  root.describeDisplayGroup("more", "More", 1, false);
  root.addSetting(SettingInfo::number("extra", "Extra").displayGroup("more"));
  InspectorPage page(&root, "test");
  QCOMPARE(page.groups().size(), 1);
  QVERIFY(!page.groups().first()->isFolded());

  page.groups().first()->setFolded(true);
  page.rebuild();
  QTRY_VERIFY(page.groups().size() == 1 && page.groups().first()->isFolded());

  // Kept in the preferences, for the next launch.
  InspectorPage again(&root, "test");
  QVERIFY(again.groups().first()->isFolded());
}

void TestInspector::layerGroupsAndSharedRow()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  SourceListing listing({ red });
  ColorLayer layer(red, makeMesh());
  InspectorPage page(layer.settings(), "layer");
  page.setGroupLabel("output", "Output shape");
  QTRY_VERIFY(!page.groups().isEmpty());

  // The columns and rows of the mesh share one row, with a counter each.
  QVERIFY(qobject_cast<CounterControl*>(page.control("output/n_columns")));
  QVERIFY(qobject_cast<CounterControl*>(page.control("output/n_rows")));
  QCOMPARE(page.row("output/n_columns"), page.row("output/n_rows"));
  // Only the four corners of the mesh have a row.
  QVERIFY(page.control("output/vertices/0"));
  QVERIFY(page.control("output/vertices/15"));
  QVERIFY(!page.control("output/vertices/5"));

  CounterControl* columns = qobject_cast<CounterControl*>(page.control("output/n_columns"));
  QTest::keyClick(columns->field(), Qt::Key_Down);
  QCOMPARE(static_cast<Mesh*>(layer.getShape().data())->nColumns(), MESH_SIZE - 1);
}

void TestInspector::liveValueShowsMark()
{
  SettingGroup root;
  root.addSetting(opacityInfo());
  InspectorPage page(&root, "test");
  NumberControl* control = qobject_cast<NumberControl*>(page.control("opacity"));
  QVERIFY(control->slider()->livePosition() < 0);

  // A modulation that keeps the rendered opacity at 20 %.
  page.setLiveValueReader([](const Setting* setting) {
    return setting->path() == "opacity" ? QVariant(0.2) : setting->effectiveValue();
  });
  QTRY_VERIFY(qAbs(control->slider()->livePosition() - 0.2) < 1e-9);
  // The handle still shows the base value.
  QCOMPARE(control->slider()->position(), 1.0);
}

void TestInspector::colorCodeIsWrittenAndRemembered()
{
  SettingGroup root;
  Setting* tint = root.addSetting(SettingInfo::color("tint", "Tint").defaultValue(QColor(0, 0, 255, 128)));
  InspectorPage page(&root, "test");
  ColorControl* control = qobject_cast<ColorControl*>(page.control("tint"));

  typeIn(control->field(), "#ff0000");
  const QColor written = tint->value().value<QColor>();
  QCOMPARE(written.rgb(), QColor(Qt::red).rgb());
  // The code has no alpha: the setting keeps its own.
  QCOMPARE(written.alpha(), 128);
  QVERIFY(!ColorPicker::recentColors().isEmpty());
  QCOMPARE(ColorPicker::recentColors().first(), written);

  control->setPickerShown(true);
  QVERIFY(control->picker());
  QVERIFY(control->picker()->isVisibleTo(control));
  QCOMPARE(control->picker()->color().rgb(), written.rgb());
}

void TestInspector::pickerKeepsHueOfGrey()
{
  ColorPicker picker;
  picker.setColor(QColor::fromHsvF(0.6f, 1.0f, 1.0f));
  QVERIFY(qAbs(picker.hue() - 0.6) < 0.01);
  picker.setColor(QColor(128, 128, 128));
  QVERIFY(qAbs(picker.hue() - 0.6) < 0.01);
  QCOMPARE(picker.color().hsvSaturation(), 0);
  picker.setColor(QColor(Qt::black));
  QVERIFY(qAbs(picker.hue() - 0.6) < 0.01);
}

void TestInspector::noHorizontalScrollAtReferenceWidth()
{
  Source::ptr red(new Color(QColor(Qt::red)));
  SourceListing listing({ red });
  ColorLayer layer(red, makeMesh());
  Inspector inspector;
  inspector.setHeader("Layer 1", "Mesh");
  InspectorPage* surface = inspector.addPage("Surface", layer.settings(), "layer");
  InspectorPage* media = inspector.addPage("Media", red->settings(), "source");
  for (InspectorGroup* group : surface->groups())
    group->setFolded(false);
  qobject_cast<ColorControl*>(media->control("color"))->setPickerShown(true);
  inspector.resize(REFERENCE_WIDTH, TEST_HEIGHT);
  inspector.show();
  QVERIFY(QTest::qWaitForWindowExposed(&inspector));

  for (InspectorPage* page : { surface, media }) {
    page->widget()->adjustSize();
    QVERIFY2(page->widget()->minimumSizeHint().width() <= REFERENCE_WIDTH,
             qPrintable(QString::number(page->widget()->minimumSizeHint().width())));
  }
}
