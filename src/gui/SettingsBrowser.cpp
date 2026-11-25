/*
 * SettingsBrowser.cpp
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

#include "SettingsBrowser.h"

#include <algorithm>
#include <limits>

#include <QColor>
#include <QPointF>

#include "qttreepropertybrowser.h"
#include "qtvariantproperty.h"
#include "variantmanager.h"

namespace mmp {

namespace {

/// Part of a row showing the whole value of its setting.
const int WHOLE_VALUE = -1;

/// Part of a row showing the low bound of a Range.
const int RANGE_LOW = 0;

/// Part of a row showing the high bound of a Range.
const int RANGE_HIGH = 1;

/// Position of a display group used by settings but described nowhere: after the others.
const int UNDESCRIBED_DISPLAY_ORDER = std::numeric_limits<int>::max();

/// Step of a setting that has none.
const double NO_STEP = 0.0;

/// Smallest increment of an integer row.
const int MINIMUM_INTEGER_STEP = 1;

/// Format of the numbers compared at the panel's precision.
const char FIXED_FORMAT = 'f';

/// Key of a group of rows made for a group of the tree, followed by its path.
const QString TREE_GROUP_KEY = QStringLiteral("group:");

/// Key of a group of rows made for a display group, followed by the path of its tree group.
const QString DISPLAY_GROUP_KEY = QStringLiteral("display:");

/// Separates the path of the tree group from the id of a display group in a key.
const QChar DISPLAY_GROUP_KEY_SEPARATOR('#');

/// Returns how the panel shows the display group id used in group, described on group or on
/// a group holding it; a display group described nowhere is titled with its id, open, last.
SettingDisplayGroup describeDisplayGroup(SettingGroup* group, const QString& id)
{
  for (SettingGroup* holder = group; holder; holder = holder->parent()) {
    const QList<SettingDisplayGroup> groups = holder->displayGroups();
    for (const SettingDisplayGroup& description : groups) {
      if (description.id == id)
        return description;
    }
  }
  return { id, id, UNDESCRIBED_DISPLAY_ORDER, false };
}

/// Returns a number as shown in a number row.
QString shownNumber(const QVariant& number)
{
  return QString::number(number.toDouble(), FIXED_FORMAT, SettingsBrowser::PANEL_DECIMALS);
}

} // namespace

SettingsBrowser::SettingsBrowser(SettingGroup* root, QtTreePropertyBrowser* browser,
                                 VariantManager* manager, QObject* parent)
  : QObject(parent),
    _browser(browser),
    _manager(manager),
    _root(root),
    _showing(false),
    _editing(false),
    _rebuildPending(false)
{
  connect(manager, &QtVariantPropertyManager::valueChanged, this, &SettingsBrowser::_writeRow);
  rebuild();
}

SettingsBrowser::~SettingsBrowser()
{
  _clear();
}

void SettingsBrowser::setGroupLabel(const QString& path, const QString& label)
{
  _groupLabels.insert(path, label);
  QtProperty* fold = _folds.value(TREE_GROUP_KEY + path);
  if (fold)
    fold->setPropertyName(label);
}

void SettingsBrowser::rebuild()
{
  // Rows cannot be deleted while one of them is sending its new value.
  if (_editing) {
    _rebuildPending = true;
    return;
  }
  if (!_browser || !_manager || !_root)
    return;

  _clear();

  // Before the rows added by other code, in their order.
  QtProperty* previous = nullptr;
  const QList<QtProperty*> rows = _buildGroup(_root);
  for (QtProperty* row : rows) {
    _browser->insertProperty(row, previous);
    previous = row;
  }

  for (auto fold = _folds.constBegin(); fold != _folds.constEnd(); ++fold) {
    const QList<QtBrowserItem*> items = _browser->items(fold.value());
    for (QtBrowserItem* item : items)
      _browser->setExpanded(item, _expanded.value(fold.key()));
  }
}

void SettingsBrowser::refresh(const QString& path, const QVariant& value)
{
  Q_UNUSED(value);
  if (!_root)
    return;

  SettingNode* node = _root->findNode(path);
  if (!node)
    return;

  if (node->kind() == SettingNode::SettingKind) {
    _showSetting(static_cast<Setting*>(node));
  }
  else if (node->kind() == SettingNode::ListKind) {
    SettingList* list = static_cast<SettingList*>(node);
    for (int index = 0; index < list->count(); index++)
      _showSetting(list->at(index));
  }
}

void SettingsBrowser::_writeRow(QtProperty* property, const QVariant& value)
{
  // A value shown by this class, or a row changed while the tree takes an edit.
  if (_showing || _editing)
    return;

  const auto found = _rows.constFind(property);
  if (found == _rows.constEnd())
    return;
  const Row row = found.value();

  Setting* setting = _findSetting(row);
  if (!setting)
    return;

  // Only a value different from the one shown for the tree is written.
  bool accepted = false;
  _editing = true;
  switch (row.type) {
  case SettingInfo::Number:
  case SettingInfo::Integer:
    if (!_showsValue(row, value, setting->displayValue()))
      accepted = setting->setDisplayValue(value);
    break;
  case SettingInfo::Range: {
    // Each bound is shown scaled, like a number; a zero scale gives a value the tree refuses.
    const double scale = setting->info().displayScale();
    QPointF range = setting->value().toPointF();
    const QVariant current = (row.part == RANGE_LOW ? range.x() : range.y()) * scale;
    if (!_showsValue(row, value, current)) {
      if (row.part == RANGE_LOW)
        range.setX(value.toDouble() / scale);
      else
        range.setY(value.toDouble() / scale);
      accepted = setting->setValue(range);
    }
    break;
  }
  case SettingInfo::Choice:
  case SettingInfo::Reference: {
    const int index = value.toInt();
    if (index >= 0 && index < row.optionIds.size()
        && row.optionIds.at(index) != setting->value().toString())
      accepted = setting->setValue(row.optionIds.at(index));
    break;
  }
  default:
    if (value != setting->value())
      accepted = setting->setValue(value);
    break;
  }

  // A refused or adjusted value: show what the tree kept.
  if (!_rebuildPending) {
    if (Setting* written = _findSetting(row))
      _showSetting(written);
  }

  // Still inside the row's call: a slot changing the structure must not delete the row.
  if (accepted)
    emit edited();
  _editing = false;

  if (_rebuildPending) {
    // The structure changed: rebuild once the row has finished sending its value.
    _rebuildPending = false;
    QMetaObject::invokeMethod(this, &SettingsBrowser::rebuild, Qt::QueuedConnection);
  }
}

QList<QtProperty*> SettingsBrowser::_buildGroup(SettingGroup* group)
{
  QList<QtProperty*> rows;
  QStringList displayGroupIds;
  QHash<QString, QList<QtProperty*> > displayGroupRows;

  const QList<SettingNode*> children = group->children();
  for (SettingNode* child : children) {
    QList<QtProperty*> made;
    QString displayGroupId;

    switch (child->kind()) {
    case SettingNode::SettingKind: {
      Setting* setting = static_cast<Setting*>(child);
      if (setting->info().isHidden())
        continue;
      made << _buildRow(setting, setting->info().label());
      displayGroupId = setting->info().displayGroup();
      break;
    }
    case SettingNode::ListKind: {
      SettingList* list = static_cast<SettingList*>(child);
      const SettingInfo& info = list->elementInfo();
      if (info.isHidden())
        continue;
      for (int index = 0; index < list->count(); index++)
        made << _buildRow(list->at(index), tr("%1 %2").arg(info.label()).arg(index));
      displayGroupId = info.displayGroup();
      break;
    }
    case SettingNode::GroupKind: {
      SettingGroup* subgroup = static_cast<SettingGroup*>(child);
      const QList<QtProperty*> subrows = _buildGroup(subgroup);
      if (subrows.isEmpty())
        continue;
      const QString path = subgroup->path();
      QtProperty* fold = _buildFold(TREE_GROUP_KEY + path, _groupLabels.value(path, subgroup->id()), false);
      for (QtProperty* subrow : subrows)
        fold->addSubProperty(subrow);
      made << fold;
      break;
    }
    }

    made.removeAll(nullptr);
    if (made.isEmpty())
      continue;
    if (displayGroupId.isEmpty()) {
      rows << made;
    }
    else {
      if (!displayGroupRows.contains(displayGroupId))
        displayGroupIds << displayGroupId;
      displayGroupRows[displayGroupId] << made;
    }
  }

  // Display groups come after the other rows, by their order, then by first use.
  QList<SettingDisplayGroup> displayGroups;
  for (const QString& id : displayGroupIds)
    displayGroups << describeDisplayGroup(group, id);
  std::stable_sort(displayGroups.begin(), displayGroups.end(),
                   [](const SettingDisplayGroup& a, const SettingDisplayGroup& b) { return a.order < b.order; });

  for (const SettingDisplayGroup& displayGroup : displayGroups) {
    const QString key = DISPLAY_GROUP_KEY + group->path() + DISPLAY_GROUP_KEY_SEPARATOR + displayGroup.id;
    QtProperty* fold = _buildFold(key, displayGroup.label, displayGroup.collapsedByDefault);
    const QList<QtProperty*> members = displayGroupRows.value(displayGroup.id);
    for (QtProperty* member : members)
      fold->addSubProperty(member);
    rows << fold;
  }
  return rows;
}

QtProperty* SettingsBrowser::_buildRow(Setting* setting, const QString& label)
{
  if (!setting)
    return nullptr;

  const SettingInfo& info = setting->info();
  const bool enabled = !info.isReadOnly() && setting->isAvailable();

  if (info.type() == SettingInfo::Range) {
    QtVariantProperty* range = _addProperty(QtVariantPropertyManager::groupTypeId(), _labelWithUnit(info, label));
    range->setToolTip(info.description());
    range->setEnabled(enabled);
    range->addSubProperty(_buildRangePart(setting, tr("Low"), RANGE_LOW));
    range->addSubProperty(_buildRangePart(setting, tr("High"), RANGE_HIGH));
    return range;
  }

  int propertyType = QMetaType::QString;
  switch (info.type()) {
  case SettingInfo::Number:    propertyType = QMetaType::Double; break;
  case SettingInfo::Integer:   propertyType = QMetaType::Int; break;
  case SettingInfo::Boolean:   propertyType = QMetaType::Bool; break;
  case SettingInfo::Color:     propertyType = QMetaType::QColor; break;
  case SettingInfo::Point:     propertyType = QMetaType::QPointF; break;
  case SettingInfo::Choice:
  case SettingInfo::Reference: propertyType = QtVariantPropertyManager::enumTypeId(); break;
  case SettingInfo::Text:
    propertyType = info.isFilePath() ? VariantManager::filePathTypeId() : int(QMetaType::QString);
    break;
  case SettingInfo::Range:
    break;
  }

  QtVariantProperty* property = _addProperty(propertyType, _labelWithUnit(info, label));
  property->setToolTip(info.description());
  property->setEnabled(enabled);

  // Attributes before the value: a new bound moves the value.
  const double scale = info.displayScale();
  bool hasMinimum = false;
  bool hasMaximum = false;
  const double minimum = info.minimum().toDouble(&hasMinimum) * scale;
  const double maximum = info.maximum().toDouble(&hasMaximum) * scale;
  if (info.type() == SettingInfo::Number) {
    property->setAttribute(QStringLiteral("decimals"), PANEL_DECIMALS);
    if (hasMinimum)
      property->setAttribute(QStringLiteral("minimum"), minimum);
    if (hasMaximum)
      property->setAttribute(QStringLiteral("maximum"), maximum);
    if (info.step() > NO_STEP)
      property->setAttribute(QStringLiteral("singleStep"), info.step() * scale);
  }
  else if (info.type() == SettingInfo::Integer) {
    if (hasMinimum)
      property->setAttribute(QStringLiteral("minimum"), qRound(minimum));
    if (hasMaximum)
      property->setAttribute(QStringLiteral("maximum"), qRound(maximum));
    if (info.step() > NO_STEP)
      property->setAttribute(QStringLiteral("singleStep"), qMax(MINIMUM_INTEGER_STEP, qRound(info.step() * scale)));
  }
  else if (info.type() == SettingInfo::Text && info.isFilePath()) {
    property->setAttribute(QStringLiteral("filter"), info.fileFilter());
  }

  _addRow(property, setting, WHOLE_VALUE);
  return property;
}

QtVariantProperty* SettingsBrowser::_buildRangePart(Setting* setting, const QString& label, int part)
{
  // Each bound of a Range is shown like a number, in the unit its group's label gives.
  const SettingInfo& info = setting->info();
  const double scale = info.displayScale();
  QtVariantProperty* property = _addProperty(QMetaType::Double, label);
  property->setToolTip(info.description());
  property->setEnabled(!info.isReadOnly() && setting->isAvailable());
  property->setAttribute(QStringLiteral("decimals"), PANEL_DECIMALS);
  bool hasMinimum = false;
  bool hasMaximum = false;
  const double minimum = info.minimum().toDouble(&hasMinimum) * scale;
  const double maximum = info.maximum().toDouble(&hasMaximum) * scale;
  if (hasMinimum)
    property->setAttribute(QStringLiteral("minimum"), minimum);
  if (hasMaximum)
    property->setAttribute(QStringLiteral("maximum"), maximum);
  if (info.step() > NO_STEP)
    property->setAttribute(QStringLiteral("singleStep"), info.step() * scale);

  _addRow(property, setting, part);
  return property;
}

QtProperty* SettingsBrowser::_buildFold(const QString& key, const QString& label, bool collapsedByDefault)
{
  QtVariantProperty* fold = _addProperty(QtVariantPropertyManager::groupTypeId(), label);
  _folds.insert(key, fold);
  if (!_expanded.contains(key))
    _expanded.insert(key, !collapsedByDefault);
  return fold;
}

QtVariantProperty* SettingsBrowser::_addProperty(int propertyType, const QString& label)
{
  QtVariantProperty* property = _manager->addProperty(propertyType, label);
  Q_CHECK_PTR(property);
  _properties << property;
  return property;
}

void SettingsBrowser::_addRow(QtVariantProperty* property, Setting* setting, int part)
{
  const QString path = setting->path();
  Row row = { path, setting->info().type(), part, QStringList() };
  auto added = _rows.insert(property, row);
  _rowsByPath.insert(path, property);
  _showRow(property, added.value(), setting);
}

QString SettingsBrowser::_labelWithUnit(const SettingInfo& info, const QString& label)
{
  const bool isNumber = info.type() == SettingInfo::Number || info.type() == SettingInfo::Integer
      || info.type() == SettingInfo::Range;
  const QString symbol = _unitSymbol(info.unit());
  if (!isNumber || symbol.isEmpty())
    return label;
  return tr("%1 (%2)").arg(label, symbol);
}

QString SettingsBrowser::_unitSymbol(SettingInfo::Unit unit)
{
  switch (unit) {
  case SettingInfo::Percent: return tr("%");
  case SettingInfo::Degrees: return tr("°");
  case SettingInfo::Hertz:   return tr("Hz");
  case SettingInfo::Seconds: return tr("s");
  case SettingInfo::Pixels:  return tr("px");
  case SettingInfo::NoUnit:  break;
  }
  return QString();
}

void SettingsBrowser::_showSetting(Setting* setting)
{
  if (!setting)
    return;
  const QList<QtVariantProperty*> properties = _rowsByPath.values(setting->path());
  for (QtVariantProperty* property : properties) {
    auto row = _rows.find(property);
    if (row != _rows.end() && row.value().type == setting->info().type())
      _showRow(property, row.value(), setting);
  }
}

void SettingsBrowser::_showRow(QtVariantProperty* property, Row& row, Setting* setting)
{
  const bool wasShowing = _showing;
  _showing = true;
  const QVariant wanted = _rowValue(property, row, setting);
  if (!_showsValue(row, property->value(), wanted))
    property->setValue(wanted);
  _showing = wasShowing;
}

QVariant SettingsBrowser::_rowValue(QtVariantProperty* property, Row& row, Setting* setting)
{
  switch (row.type) {
  case SettingInfo::Number:
    return setting->displayValue().toDouble();
  case SettingInfo::Integer:
    return qRound(setting->displayValue().toDouble());
  case SettingInfo::Boolean:
    return setting->value().toBool();
  case SettingInfo::Color:
    return setting->value().value<QColor>();
  case SettingInfo::Text:
    return setting->value().toString();
  case SettingInfo::Point:
    return setting->value().toPointF();
  case SettingInfo::Range: {
    const QPointF range = setting->value().toPointF();
    return (row.part == RANGE_LOW ? range.x() : range.y()) * setting->info().displayScale();
  }
  case SettingInfo::Choice:
  case SettingInfo::Reference: {
    // The menu lists the options; a value missing from them is shown as it is.
    QStringList ids;
    QStringList labels;
    const QList<SettingOption> options = setting->info().options();
    for (const SettingOption& option : options) {
      ids << option.id;
      labels << option.label;
    }
    const QString current = setting->value().toString();
    int index = int(ids.indexOf(current));
    if (index < 0) {
      ids << current;
      labels << current;
      index = int(ids.size()) - 1;
    }
    row.optionIds = ids;
    const QString enumNames = QStringLiteral("enumNames");
    if (property->attributeValue(enumNames).toStringList() != labels)
      property->setAttribute(enumNames, labels);
    return index;
  }
  }
  return QVariant();
}

bool SettingsBrowser::_showsValue(const Row& row, const QVariant& shown, const QVariant& wanted) const
{
  switch (row.type) {
  case SettingInfo::Number:
  case SettingInfo::Range:
    return shownNumber(shown) == shownNumber(wanted);
  case SettingInfo::Integer:
  case SettingInfo::Choice:
  case SettingInfo::Reference:
    return shown.toInt() == wanted.toInt();
  default:
    return shown == wanted;
  }
}

Setting* SettingsBrowser::_findSetting(const Row& row) const
{
  if (!_root)
    return nullptr;
  Setting* setting = _root->find(row.path);
  return (setting && setting->info().type() == row.type) ? setting : nullptr;
}

void SettingsBrowser::_clear()
{
  // Groups of rows open now stay open after a rebuild.
  if (_browser) {
    for (auto fold = _folds.constBegin(); fold != _folds.constEnd(); ++fold) {
      const QList<QtBrowserItem*> items = _browser->items(fold.value());
      if (!items.isEmpty())
        _expanded.insert(fold.key(), _browser->isExpanded(items.first()));
    }
  }

  _folds.clear();
  _rows.clear();
  _rowsByPath.clear();

  // Deleting a property removes it from the browser; the manager deletes its own when it goes.
  QList<QtProperty*> properties;
  properties.swap(_properties);
  if (_manager)
    qDeleteAll(properties);
}

} // namespace mmp
