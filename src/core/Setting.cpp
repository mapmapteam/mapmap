/*
 * Setting.cpp
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

#include "Setting.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QColor>
#include <QPointF>
#include <QStringList>
#include <QtDebug>

namespace mmp {

namespace {

/// Step of a setting that has none.
const double NO_STEP = 0.0;

/// Where the steps of a setting without a minimum start.
const double STEP_ORIGIN = 0.0;

/// Smallest step of an Integer setting: its values are whole numbers.
const double INTEGER_STEP = 1.0;

/// Display scale of a value shown as it is.
const double DEFAULT_DISPLAY_SCALE = 1.0;

/// Joins the ids of a path.
const QChar PATH_SEPARATOR('/');

/// Index returned when a child is missing.
const int NOT_FOUND = -1;

/// Lowest normalized position.
const double NORMALIZED_MINIMUM = 0.0;

/// Highest normalized position.
const double NORMALIZED_MAXIMUM = 1.0;

/// Normalized position from which a Boolean is true.
const double NORMALIZED_TRUE_THRESHOLD = 0.5;

/// Number of arguments a Point or a Range takes: its two coordinates.
const int PAIR_ARGUMENT_COUNT = 2;

/// Number of arguments every other type takes.
const int SINGLE_ARGUMENT_COUNT = 1;

/// The number a Boolean takes as false; any other finite number is true.
const double BOOLEAN_FALSE_NUMBER = 0.0;

/**
 * Reads a Boolean: true or false, any finite number (0 is false), or the texts "true",
 * "false", "1" and "0" in any case. Returns false when the value is none of these.
 */
bool parseBoolean(const QVariant& value, bool* result)
{
  const int typeId = value.typeId();
  if (typeId == QMetaType::Bool) {
    *result = value.toBool();
    return true;
  }
  if (typeId == QMetaType::QString || typeId == QMetaType::QByteArray) {
    const QString text = value.toString().toLower();
    if (text == QLatin1String("true") || text == QLatin1String("1")) {
      *result = true;
      return true;
    }
    if (text == QLatin1String("false") || text == QLatin1String("0")) {
      *result = false;
      return true;
    }
    return false;
  }
  // Controllers send 127, or any non-zero number, for "on".
  bool isNumber = false;
  const double number = value.toDouble(&isNumber);
  if (isNumber && std::isfinite(number)) {
    *result = (number != BOOLEAN_FALSE_NUMBER);
    return true;
  }
  return false;
}

/**
 * Returns true when a number is within the range of the type it is stored as, so that
 * converting it to that type is defined.
 */
bool fitsValueType(double number, QMetaType::Type valueType)
{
  switch (valueType) {
  case QMetaType::Int:
    return number >= std::numeric_limits<int>::lowest() && number <= std::numeric_limits<int>::max();
  case QMetaType::Float:
    return std::fabs(number) <= std::numeric_limits<float>::max();
  default:
    return true;
  }
}

} // namespace

// ---------------------------------------------------------------------------
// SettingInfo

SettingInfo::SettingInfo(Type type, const QString& id, const QString& label,
                         QMetaType::Type valueType, const QVariant& defaultValue)
  : _id(id),
    _label(label),
    _type(type),
    _step(NO_STEP),
    _strictStep(false),
    _defaultValue(defaultValue),
    _unit(NoUnit),
    _displayScale(DEFAULT_DISPLAY_SCALE),
    _valueType(valueType),
    _filePath(false),
    _modulable(false),
    _stored(true),
    _readOnly(false),
    _hidden(false)
{
}

SettingInfo SettingInfo::number(const QString& id, const QString& label)
{
  return SettingInfo(Number, id, label, QMetaType::Double, 0.0);
}

SettingInfo SettingInfo::integer(const QString& id, const QString& label)
{
  SettingInfo info(Integer, id, label, QMetaType::Int, 0);
  info._step = INTEGER_STEP;
  return info;
}

SettingInfo SettingInfo::boolean(const QString& id, const QString& label)
{
  return SettingInfo(Boolean, id, label, QMetaType::Bool, false);
}

SettingInfo SettingInfo::color(const QString& id, const QString& label)
{
  return SettingInfo(Color, id, label, QMetaType::QColor, QColor(Qt::black));
}

SettingInfo SettingInfo::text(const QString& id, const QString& label)
{
  return SettingInfo(Text, id, label, QMetaType::QString, QString());
}

SettingInfo SettingInfo::filePath(const QString& id, const QString& label, const QString& filter)
{
  SettingInfo info = text(id, label);
  info._filePath = true;
  info._fileFilter = filter;
  return info;
}

SettingInfo SettingInfo::choice(const QString& id, const QString& label, const QList<SettingOption>& options)
{
  SettingInfo info(Choice, id, label, QMetaType::QString,
                   options.isEmpty() ? QString() : options.first().id);
  info._options = options;
  return info;
}

SettingInfo SettingInfo::point(const QString& id, const QString& label)
{
  return SettingInfo(Point, id, label, QMetaType::QPointF, QPointF());
}

SettingInfo SettingInfo::range(const QString& id, const QString& label)
{
  return SettingInfo(Range, id, label, QMetaType::QPointF, QPointF());
}

SettingInfo SettingInfo::reference(const QString& id, const QString& label)
{
  return SettingInfo(Reference, id, label, QMetaType::QString, QString());
}

SettingInfo& SettingInfo::bounds(const QVariant& minimum, const QVariant& maximum)
{
  _minimum = minimum;
  _maximum = maximum;
  return *this;
}

SettingInfo& SettingInfo::step(double step, bool strict)
{
  _step = step;
  _strictStep = strict;
  return *this;
}

SettingInfo& SettingInfo::defaultValue(const QVariant& value)
{
  _defaultValue = value;
  return *this;
}

SettingInfo& SettingInfo::unit(Unit unit)
{
  _unit = unit;
  return *this;
}

SettingInfo& SettingInfo::displayScale(double scale)
{
  _displayScale = scale;
  return *this;
}

SettingInfo& SettingInfo::displayGroup(const QString& groupId)
{
  _displayGroup = groupId;
  return *this;
}

SettingInfo& SettingInfo::valueType(QMetaType::Type type)
{
  _valueType = type;
  return *this;
}

SettingInfo& SettingInfo::optionsProvider(std::function<QList<SettingOption>()> provider)
{
  _optionsProvider = std::move(provider);
  return *this;
}

SettingInfo& SettingInfo::modulable()
{
  _modulable = true;
  return *this;
}

SettingInfo& SettingInfo::notStored()
{
  _stored = false;
  return *this;
}

SettingInfo& SettingInfo::readOnly()
{
  _readOnly = true;
  return *this;
}

SettingInfo& SettingInfo::hidden()
{
  _hidden = true;
  return *this;
}

SettingInfo& SettingInfo::displayRow(const QString& rowId)
{
  _displayRow = rowId;
  return *this;
}

SettingInfo& SettingInfo::description(const QString& text)
{
  _description = text;
  return *this;
}

QList<SettingOption> SettingInfo::options() const
{
  return _optionsProvider ? _optionsProvider() : _options;
}

// ---------------------------------------------------------------------------
// SettingNode

SettingNode::SettingNode(const QString& id)
  : _id(id),
    _parentNode(nullptr)
{
}

SettingNode::~SettingNode()
{
}

SettingGroup* SettingNode::parent() const
{
  SettingNode* node = _parentNode;
  while (node && node->kind() != GroupKind)
    node = node->_parentNode;
  return static_cast<SettingGroup*>(node);
}

QString SettingNode::path() const
{
  QStringList ids;
  for (const SettingNode* node = this; node->_parentNode; node = node->_parentNode)
    ids.prepend(node->_id);
  return ids.join(PATH_SEPARATOR);
}

const SettingGroup* SettingNode::_topmost() const
{
  const SettingNode* node = this;
  while (node->_parentNode)
    node = node->_parentNode;
  // A list being built has no group above it yet.
  return node->kind() == GroupKind ? static_cast<const SettingGroup*>(node) : nullptr;
}

void SettingNode::_notifyValueChanged(const QVariant& value) const
{
  const SettingGroup* top = _topmost();
  if (top && top->_listener)
    top->_listener->settingValueChanged(path(), value);
}

void SettingNode::_notifyStructureChanged() const
{
  const SettingGroup* top = _topmost();
  if (top && top->_listener)
    top->_listener->settingStructureChanged();
}

// ---------------------------------------------------------------------------
// Setting

Setting::Setting(const SettingInfo& info)
  : Setting(info.id(), info)
{
}

Setting::Setting(const QString& id, const SettingInfo& info)
  : SettingNode(id),
    _info(info),
    _available(true),
    _notified(false),
    _written(false)
{
  bool ok = false;
  QVariant value = sanitized(info.defaultValue(), &ok);
  if (!ok)
    qWarning() << "Default value" << info.defaultValue() << "of setting" << id << "cannot be sanitized";
  _value = ok ? value : info.defaultValue();
}

QVariant Setting::value() const
{
  return _read ? _read() : _value;
}

QVariant Setting::effectiveValue() const
{
  return value();
}

bool Setting::setValue(const QVariant& value)
{
  if (_info.isReadOnly())
    return false;

  bool ok = false;
  QVariant clean = sanitized(value, &ok);
  if (!ok)
    return false;
  if (clean == this->value())
    return true;

  _notified = false;
  if (_read) {
    if (!_write || !_write(clean))
      return false;
  }
  else {
    _value = clean;
    _written = true;
  }

  // The owner's write function may have notified already.
  if (!_notified)
    notifyChanged();
  return true;
}

QVariant Setting::sanitized(const QVariant& value, bool* ok) const
{
  const SettingInfo::Type type = _info.type();
  QVariant result;
  bool valid = false;

  switch (type) {
  case SettingInfo::Number:
  case SettingInfo::Integer: {
    double number = value.toDouble(&valid);
    valid = valid && _sanitizeNumber(&number) && fitsValueType(number, _info.valueType());
    if (valid) {
      result = number;
      valid = result.convert(QMetaType(_info.valueType()));
      // A narrower type may overflow or drop the fraction of a whole number.
      const double converted = result.toDouble();
      valid = valid && std::isfinite(converted)
          && (type != SettingInfo::Integer || converted == number);
    }
    break;
  }
  case SettingInfo::Point:
  case SettingInfo::Range: {
    QVariant point = value;
    valid = point.convert(QMetaType::fromType<QPointF>());
    double low = point.toPointF().x();
    double high = point.toPointF().y();
    valid = valid && _sanitizeNumber(&low) && _sanitizeNumber(&high);
    if (valid) {
      if (type == SettingInfo::Range && low > high)
        std::swap(low, high);
      result = QPointF(low, high);
      valid = result.convert(QMetaType(_info.valueType()));
    }
    break;
  }
  case SettingInfo::Color:
    result = value;
    valid = result.convert(QMetaType::fromType<QColor>()) && result.value<QColor>().isValid()
        && result.convert(QMetaType(_info.valueType()));
    break;
  case SettingInfo::Boolean: {
    // Qt would turn any text but "", "0" and "false" into true.
    bool flag = false;
    valid = parseBoolean(value, &flag);
    if (valid) {
      result = flag;
      valid = result.convert(QMetaType(_info.valueType()));
    }
    break;
  }
  case SettingInfo::Text:
  case SettingInfo::Choice:
  case SettingInfo::Reference:
    // A choice or a reference keeps an id missing from its options.
    result = value;
    valid = result.convert(QMetaType(_info.valueType()));
    break;
  }

  if (ok)
    *ok = valid;
  return valid ? result : QVariant();
}

bool Setting::_sanitizeNumber(double* number) const
{
  double x = *number;
  if (!std::isfinite(x))
    return false;

  bool hasMinimum = false;
  bool hasMaximum = false;
  const double minimum = _info.minimum().toDouble(&hasMinimum);
  const double maximum = _info.maximum().toDouble(&hasMaximum);
  const bool isInteger = (_info.type() == SettingInfo::Integer);

  // The step only rounds what is kept for whole numbers and strict steps.
  double step = _info.step();
  if (isInteger)
    step = qMax(step, INTEGER_STEP);
  if (step > NO_STEP && (isInteger || _info.isStrictStep())) {
    const double origin = hasMinimum ? minimum : STEP_ORIGIN;
    x = origin + std::round((x - origin) / step) * step;
  }
  if (isInteger)
    x = std::round(x);

  // The bounds apply to every write.
  if (hasMinimum && x < minimum)
    x = minimum;
  if (hasMaximum && x > maximum)
    x = maximum;

  *number = x;
  return true;
}

void Setting::reset()
{
  setValue(_info.defaultValue());
}

bool Setting::hasNormalized() const
{
  switch (_info.type()) {
  case SettingInfo::Number:
  case SettingInfo::Integer: {
    double minimum = 0.0;
    double maximum = 0.0;
    return _numberBounds(&minimum, &maximum);
  }
  case SettingInfo::Boolean:
    return true;
  case SettingInfo::Choice:
    return !_info.options().isEmpty();
  default:
    return false;
  }
}

double Setting::normalized() const
{
  switch (_info.type()) {
  case SettingInfo::Number:
  case SettingInfo::Integer: {
    double minimum = 0.0;
    double maximum = 0.0;
    if (!_numberBounds(&minimum, &maximum))
      return NORMALIZED_MINIMUM;
    // The owner's storage may hold a value outside the bounds.
    const double position = (value().toDouble() - minimum) / (maximum - minimum);
    return qBound(NORMALIZED_MINIMUM, position, NORMALIZED_MAXIMUM);
  }
  case SettingInfo::Boolean:
    return value().toBool() ? NORMALIZED_MAXIMUM : NORMALIZED_MINIMUM;
  case SettingInfo::Choice: {
    const QList<SettingOption> options = _info.options();
    const QString id = value().toString();
    const int last = int(options.size()) - 1;
    for (int index = 0; index <= last; index++) {
      if (options.at(index).id == id)
        return last > 0 ? double(index) / last : NORMALIZED_MINIMUM;
    }
    return NORMALIZED_MINIMUM;
  }
  default:
    return NORMALIZED_MINIMUM;
  }
}

bool Setting::setNormalized(double position)
{
  if (!hasNormalized()) {
    qWarning() << "Setting" << path() << "has no normalized position";
    return false;
  }
  if (!std::isfinite(position))
    return false;
  position = qBound(NORMALIZED_MINIMUM, position, NORMALIZED_MAXIMUM);

  switch (_info.type()) {
  case SettingInfo::Number:
  case SettingInfo::Integer: {
    double minimum = 0.0;
    double maximum = 0.0;
    _numberBounds(&minimum, &maximum);
    return setValue(minimum + position * (maximum - minimum));
  }
  case SettingInfo::Boolean:
    return setValue(position >= NORMALIZED_TRUE_THRESHOLD);
  case SettingInfo::Choice: {
    // Equal steps: [0, 1/n) is the first option, ..., [(n-1)/n, 1] the last.
    const QList<SettingOption> options = _info.options();
    const int count = int(options.size());
    const int index = qMin(int(std::floor(position * count)), count - 1);
    return setValue(options.at(index).id);
  }
  default:
    return false;
  }
}

QVariant Setting::displayValue() const
{
  if (_isNumber())
    return value().toDouble() * _info.displayScale();
  return value();
}

bool Setting::setDisplayValue(const QVariant& value)
{
  if (!_isNumber())
    return setValue(value);
  bool isNumber = false;
  const double shown = value.toDouble(&isNumber);
  // A zero scale gives a value that is not finite, which setValue() refuses.
  return isNumber && setValue(shown / _info.displayScale());
}

QString Setting::toText() const
{
  // The text of the value as the project file stores it, in the type of the setting.
  QVariant stored = value();
  stored.convert(QMetaType(_info.valueType()));
  return stored.toString();
}

bool Setting::setFromText(const QString& text)
{
  return setValue(text);
}

bool Setting::setFromArguments(const QVariantList& arguments)
{
  const SettingInfo::Type type = _info.type();
  const bool isPair = (type == SettingInfo::Point || type == SettingInfo::Range);
  const int expected = isPair ? PAIR_ARGUMENT_COUNT : SINGLE_ARGUMENT_COUNT;
  if (arguments.size() != expected) {
    qWarning() << "Setting" << path() << "expects" << expected
               << (expected == SINGLE_ARGUMENT_COUNT ? "argument," : "arguments,")
               << "got" << arguments.size();
    return false;
  }

  bool accepted = false;
  if (isPair) {
    bool isFirstNumber = false;
    bool isSecondNumber = false;
    const double first = arguments.at(0).toDouble(&isFirstNumber);
    const double second = arguments.at(1).toDouble(&isSecondNumber);
    accepted = isFirstNumber && isSecondNumber && setValue(QPointF(first, second));
  }
  else {
    accepted = setValue(arguments.first());
  }
  if (!accepted)
    qWarning() << "Setting" << path() << "refused the arguments" << arguments;
  return accepted;
}

bool Setting::_isNumber() const
{
  return _info.type() == SettingInfo::Number || _info.type() == SettingInfo::Integer;
}

bool Setting::_numberBounds(double* minimum, double* maximum) const
{
  bool hasMinimum = false;
  bool hasMaximum = false;
  *minimum = _info.minimum().toDouble(&hasMinimum);
  *maximum = _info.maximum().toDouble(&hasMaximum);
  return hasMinimum && hasMaximum && std::isfinite(*minimum) && std::isfinite(*maximum)
      && *minimum < *maximum;
}

void Setting::setAvailable(bool available)
{
  if (_available == available)
    return;
  _available = available;
  _notifyStructureChanged();
}

void Setting::bind(std::function<QVariant()> read, std::function<bool(const QVariant&)> write)
{
  if (bool(read) != bool(write)) {
    qWarning() << "Cannot bind setting" << path() << ": it needs both a read and a write function";
    return;
  }
  _read = std::move(read);
  _write = std::move(write);

  // A value written while it was kept here would be lost: the owner's storage takes it.
  if (_read && _written) {
    _written = false;
    if (_value != _read() && !_write(_value))
      qWarning() << "Setting" << path() << "refused the value" << _value << "written before it was bound";
  }
}

void Setting::notifyChanged()
{
  _notified = true;
  _notifyValueChanged(value());
}

// ---------------------------------------------------------------------------
// SettingList

SettingList::SettingList(const QString& id, const SettingInfo& elementInfo,
                         std::function<int()> count,
                         std::function<QVariant(int)> read,
                         std::function<bool(int, const QVariant&)> write)
  : SettingNode(id),
    _elementInfo(elementInfo),
    _count(std::move(count)),
    _read(std::move(read)),
    _write(std::move(write))
{
}

SettingList::~SettingList()
{
}

void SettingList::setShownInPanel(std::function<bool(int)> shown)
{
  _shownInPanel = std::move(shown);
}

bool SettingList::isShownInPanel(int index) const
{
  if (index < 0 || index >= count())
    return false;
  return !_shownInPanel || _shownInPanel(index);
}

int SettingList::count() const
{
  return int(_elements.size());
}

Setting* SettingList::at(int index)
{
  return _element(index);
}

Setting* SettingList::_element(int index) const
{
  if (index < 0 || index >= int(_elements.size()))
    return nullptr;
  return _elements[index].get();
}

void SettingList::sync()
{
  const int count = _count ? qMax(0, _count()) : 0;
  const int previous = int(_elements.size());
  if (count == previous)
    return;

  while (int(_elements.size()) > count)
    _elements.pop_back();
  for (int index = previous; index < count; index++) {
    std::unique_ptr<Setting> element(new Setting(QString::number(index), _elementInfo));
    element->_parentNode = this;
    element->bind([this, index]() { return _read(index); },
                  [this, index](const QVariant& value) { return _write && _write(index, value); });
    _elements.push_back(std::move(element));
  }
  _notifyStructureChanged();
}

void SettingList::notifyChanged()
{
  QVariantList values;
  for (const std::unique_ptr<Setting>& element : _elements)
    values.append(element->value());
  _notifyValueChanged(values);
}

// ---------------------------------------------------------------------------
// SettingGroup

SettingGroup::SettingGroup()
  : SettingGroup(QString())
{
}

SettingGroup::SettingGroup(const QString& id)
  : SettingNode(id),
    _listener(nullptr)
{
}

SettingGroup::~SettingGroup()
{
  // A group is only ever held by a group. An owned one is detached before being destroyed,
  // so a group still holding this one has it mounted: leave it, without notifying.
  if (_parentNode) {
    SettingGroup* holder = static_cast<SettingGroup*>(_parentNode);
    for (int index = 0; index < holder->_children.size(); index++) {
      if (holder->_children.at(index).node == this) {
        holder->_children.removeAt(index);
        break;
      }
    }
  }

  const QList<Child> children = _children;
  _children.clear();
  for (const Child& child : children) {
    child.node->_parentNode = nullptr;
    if (child.owned)
      delete child.node;
  }
}

Setting* SettingGroup::addSetting(const SettingInfo& info)
{
  if (!_acceptsId(info.id()))
    return nullptr;
  Setting* setting = new Setting(info);
  _append(setting, true);
  if (_settingAddedHandler)
    _settingAddedHandler(setting);
  return setting;
}

SettingGroup* SettingGroup::addGroup(const QString& id)
{
  if (!_acceptsId(id))
    return nullptr;
  SettingGroup* group = new SettingGroup(id);
  _append(group, true);
  return group;
}

SettingList* SettingGroup::addList(const QString& id, const SettingInfo& elementInfo,
                                   std::function<int()> count,
                                   std::function<QVariant(int)> read,
                                   std::function<bool(int, const QVariant&)> write)
{
  if (!count || !read || !write) {
    qWarning() << "Cannot add settings list" << id << ": it needs a count, a read and a write function";
    return nullptr;
  }
  if (!_acceptsId(id))
    return nullptr;
  SettingList* list = new SettingList(id, elementInfo, std::move(count), std::move(read), std::move(write));
  // Built before it joins the tree: one structure notification, from _append().
  list->sync();
  _append(list, true);
  return list;
}

void SettingGroup::mount(const QString& id, SettingGroup* group)
{
  if (!group) {
    qWarning() << "Cannot mount a null settings group as" << id;
    return;
  }
  if (group->_parentNode) {
    qWarning() << "Cannot mount settings group" << id << ": it is already in a tree";
    return;
  }
  for (const SettingNode* node = this; node; node = node->_parentNode) {
    if (node == group) {
      qWarning() << "Cannot mount settings group" << id << "inside itself";
      return;
    }
  }
  if (!_acceptsId(id))
    return;
  group->_id = id;
  _append(group, false);
}

void SettingGroup::unmount(const QString& id)
{
  const int index = _indexOf(id);
  if (index == NOT_FOUND || _children.at(index).owned) {
    qWarning() << "No settings group is mounted as" << id;
    return;
  }
  _removeAt(index);
}

bool SettingGroup::remove(const QString& id)
{
  const int index = _indexOf(id);
  if (index == NOT_FOUND)
    return false;
  _removeAt(index);
  return true;
}

QList<SettingNode*> SettingGroup::children() const
{
  QList<SettingNode*> nodes;
  for (const Child& child : _children)
    nodes.append(child.node);
  return nodes;
}

SettingNode* SettingGroup::findNode(const QString& path) const
{
  if (path.isEmpty())
    return const_cast<SettingGroup*>(this);

  const SettingNode* node = this;
  const QStringList ids = path.split(PATH_SEPARATOR);
  for (const QString& id : ids) {
    switch (node->kind()) {
    case GroupKind: {
      const SettingGroup* group = static_cast<const SettingGroup*>(node);
      const int index = group->_indexOf(id);
      node = (index == NOT_FOUND ? nullptr : group->_children.at(index).node);
      break;
    }
    case ListKind: {
      // Only the canonical spelling of an index: "1", not "01" or "+1".
      bool isNumber = false;
      const int index = id.toInt(&isNumber);
      node = (isNumber && id == QString::number(index)
              ? static_cast<const SettingList*>(node)->_element(index) : nullptr);
      break;
    }
    case SettingKind:
      node = nullptr;
      break;
    }
    if (!node)
      return nullptr;
  }
  return const_cast<SettingNode*>(node);
}

Setting* SettingGroup::find(const QString& path) const
{
  SettingNode* node = findNode(path);
  return (node && node->kind() == SettingKind) ? static_cast<Setting*>(node) : nullptr;
}

void SettingGroup::describeDisplayGroup(const QString& groupId, const QString& label, int order,
                                        bool collapsedByDefault)
{
  const SettingDisplayGroup description = { groupId, label, order, collapsedByDefault };
  for (SettingDisplayGroup& existing : _displayGroups) {
    if (existing.id == groupId) {
      existing = description;
      return;
    }
  }
  _displayGroups.append(description);
}

bool SettingGroup::_acceptsId(const QString& id) const
{
  if (id.isEmpty() || id.contains(PATH_SEPARATOR)) {
    qWarning() << "Invalid setting id" << id;
    return false;
  }
  if (_indexOf(id) != NOT_FOUND) {
    qWarning() << "Setting id" << id << "is already taken in" << path();
    return false;
  }
  return true;
}

int SettingGroup::_indexOf(const QString& id) const
{
  for (int index = 0; index < _children.size(); index++) {
    if (_children.at(index).node->_id == id)
      return index;
  }
  return NOT_FOUND;
}

void SettingGroup::_append(SettingNode* node, bool owned)
{
  node->_parentNode = this;
  _children.append({ node, owned });
  _notifyStructureChanged();
}

void SettingGroup::_removeAt(int index)
{
  const Child child = _children.takeAt(index);
  child.node->_parentNode = nullptr;
  if (child.owned)
    delete child.node;
  _notifyStructureChanged();
}

} // namespace mmp
