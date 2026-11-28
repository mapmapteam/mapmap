/*
 * Link.cpp
 *
 * (c) 2026 MapMap contributors
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

#include "Link.h"

#include <QKeySequence>

#include "Setting.h"

namespace mmp {

namespace {

// Keys of the links in project files.
const QString TRIGGER_KEY = QStringLiteral("trigger");
const QString TARGET_KEY = QStringLiteral("target");
const QString ACTION_KEY = QStringLiteral("action");
const QString KIND_KEY = QStringLiteral("kind");
const QString KEY_KEY = QStringLiteral("key");
const QString ELEMENT_KEY = QStringLiteral("element");
const QString ID_KEY = QStringLiteral("id");
const QString SETTING_KEY = QStringLiteral("setting");

// Values of the links in project files.
const QString KEY_KIND = QStringLiteral("key");
const QString SOURCE_ELEMENT = QStringLiteral("source");
const QString LAYER_ELEMENT = QStringLiteral("layer");
const QString TOGGLE_ACTION = QStringLiteral("toggle");
const QString ON_ACTION = QStringLiteral("on");
const QString OFF_ACTION = QStringLiteral("off");

/// Position of the minimum of a setting, between 0 and 1.
const double MINIMUM_POSITION = 0.0;

/// Position of the maximum of a setting, between 0 and 1.
const double MAXIMUM_POSITION = 1.0;

/// Position from which Toggle sends a number back to its minimum.
const double MIDDLE_POSITION = 0.5;

/// Returns the key in the portable text of QKeySequence, as kept by LinkTrigger.
QString portableKey(const QString& key)
{
  return QKeySequence::fromString(key, QKeySequence::PortableText).toString(QKeySequence::PortableText);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// LinkTrigger

LinkTrigger LinkTrigger::forKey(const QString& key)
{
  LinkTrigger trigger;
  trigger.kind = Key;
  trigger.key = portableKey(key);
  return trigger;
}

bool LinkTrigger::isValid() const
{
  switch (kind) {
  case Key:
    return !key.isEmpty() && QKeySequence::fromString(key, QKeySequence::PortableText).count() == 1;
  }
  return false;
}

QString LinkTrigger::text() const
{
  switch (kind) {
  case Key:
    return QKeySequence::fromString(key, QKeySequence::PortableText).toString(QKeySequence::NativeText);
  }
  return QString();
}

QString LinkTrigger::kindText() const
{
  switch (kind) {
  case Key:
    return QObject::tr("Keyboard");
  }
  return QString();
}

QJsonObject LinkTrigger::toJson() const
{
  QJsonObject object;
  switch (kind) {
  case Key:
    object[KIND_KEY] = KEY_KIND;
    object[KEY_KEY] = key;
    break;
  }
  return object;
}

LinkTrigger LinkTrigger::fromJson(const QJsonObject& object)
{
  LinkTrigger trigger;
  if (object.value(KIND_KEY).toString() == KEY_KIND)
    trigger = forKey(object.value(KEY_KEY).toString());
  else
    trigger.key.clear();
  return trigger;
}

bool LinkTrigger::operator==(const LinkTrigger& other) const
{
  return kind == other.kind && key == other.key;
}

// ---------------------------------------------------------------------------------------------
// LinkTarget

LinkTarget LinkTarget::of(ElementKind element, uid id, const QString& path)
{
  LinkTarget target;
  target.element = element;
  target.id = id;
  target.path = path;
  return target;
}

bool LinkTarget::isValid() const
{
  return id > NULL_UID && !path.isEmpty();
}

QJsonObject LinkTarget::toJson() const
{
  QJsonObject object;
  object[ELEMENT_KEY] = element == SourceElement ? SOURCE_ELEMENT : LAYER_ELEMENT;
  object[ID_KEY] = id;
  object[SETTING_KEY] = path;
  return object;
}

LinkTarget LinkTarget::fromJson(const QJsonObject& object)
{
  LinkTarget target;
  const QString element = object.value(ELEMENT_KEY).toString();
  if (element == SOURCE_ELEMENT)
    target.element = SourceElement;
  else if (element == LAYER_ELEMENT)
    target.element = LayerElement;
  else
    return LinkTarget();
  target.id = object.value(ID_KEY).toInt(NULL_UID);
  target.path = object.value(SETTING_KEY).toString();
  return target;
}

QString LinkTarget::describe() const
{
  return QStringLiteral("%1 %2, %3")
      .arg(element == SourceElement ? SOURCE_ELEMENT : LAYER_ELEMENT)
      .arg(id)
      .arg(path);
}

bool LinkTarget::operator==(const LinkTarget& other) const
{
  return element == other.element && id == other.id && path == other.path;
}

// ---------------------------------------------------------------------------------------------
// Actions

QString linkActionName(LinkAction action)
{
  switch (action) {
  case LinkAction::Toggle:    return TOGGLE_ACTION;
  case LinkAction::SwitchOn:  return ON_ACTION;
  case LinkAction::SwitchOff: return OFF_ACTION;
  }
  return TOGGLE_ACTION;
}

bool linkActionFromName(const QString& name, LinkAction* action)
{
  if (name == TOGGLE_ACTION)
    *action = LinkAction::Toggle;
  else if (name == ON_ACTION)
    *action = LinkAction::SwitchOn;
  else if (name == OFF_ACTION)
    *action = LinkAction::SwitchOff;
  else
    return false;
  return true;
}

QString linkActionText(LinkAction action, const Setting* setting)
{
  const SettingInfo::Type type = setting ? setting->info().type() : SettingInfo::Boolean;
  switch (type) {
  case SettingInfo::Boolean:
    switch (action) {
    case LinkAction::Toggle:    return QObject::tr("Toggle");
    case LinkAction::SwitchOn:  return QObject::tr("Turn on");
    case LinkAction::SwitchOff: return QObject::tr("Turn off");
    }
    break;
  case SettingInfo::Choice:
    switch (action) {
    case LinkAction::Toggle:    return QObject::tr("Next option");
    case LinkAction::SwitchOn:  return QObject::tr("Last option");
    case LinkAction::SwitchOff: return QObject::tr("First option");
    }
    break;
  default:
    switch (action) {
    case LinkAction::Toggle:    return QObject::tr("Minimum or maximum");
    case LinkAction::SwitchOn:  return QObject::tr("Set to maximum");
    case LinkAction::SwitchOff: return QObject::tr("Set to minimum");
    }
    break;
  }
  return QString();
}

bool canBeLinked(const Setting* setting)
{
  return setting && setting->hasNormalized() && !setting->info().isReadOnly();
}

bool applyLinkAction(Setting* setting, LinkAction action)
{
  if (!canBeLinked(setting))
    return false;

  switch (action) {
  case LinkAction::SwitchOn:
    return setting->setNormalized(MAXIMUM_POSITION);
  case LinkAction::SwitchOff:
    return setting->setNormalized(MINIMUM_POSITION);
  case LinkAction::Toggle:
    break;
  }

  const SettingInfo& info = setting->info();
  if (info.type() == SettingInfo::Boolean)
    return setting->setValue(!setting->value().toBool());

  if (info.type() == SettingInfo::Choice) {
    // The next option, back to the first after the last; an unknown id goes to the first.
    const QList<SettingOption> options = info.options();
    const QString current = setting->value().toString();
    int index = -1;
    for (int i = 0; i < options.size(); ++i) {
      if (options.at(i).id == current)
        index = i;
    }
    return setting->setValue(options.at((index + 1) % options.size()).id);
  }

  // A number goes to the other end.
  return setting->setNormalized(setting->normalized() < MIDDLE_POSITION ? MAXIMUM_POSITION
                                                                         : MINIMUM_POSITION);
}

// ---------------------------------------------------------------------------------------------
// Link

QJsonObject Link::toJson() const
{
  QJsonObject object;
  object[TRIGGER_KEY] = trigger.toJson();
  object[TARGET_KEY] = target.toJson();
  object[ACTION_KEY] = linkActionName(action);
  return object;
}

bool Link::fromJson(const QJsonObject& object, Link* link, QString* reason)
{
  Link read;
  read.trigger = LinkTrigger::fromJson(object.value(TRIGGER_KEY).toObject());
  if (!read.trigger.isValid()) {
    *reason = QStringLiteral("its trigger cannot be read");
    return false;
  }
  read.target = LinkTarget::fromJson(object.value(TARGET_KEY).toObject());
  if (!read.target.isValid()) {
    *reason = QStringLiteral("its target cannot be read");
    return false;
  }
  // Without an action, a link toggles.
  if (object.contains(ACTION_KEY) && !linkActionFromName(object.value(ACTION_KEY).toString(), &read.action)) {
    *reason = QStringLiteral("its action %1 is unknown").arg(object.value(ACTION_KEY).toString());
    return false;
  }
  *link = read;
  return true;
}

// ---------------------------------------------------------------------------------------------
// LinkTable

LinkTable::LinkTable(QObject* parent)
  : QObject(parent)
{
}

bool LinkTable::add(const Link& link)
{
  if (!link.trigger.isValid() || !link.target.isValid())
    return false;

  // A trigger drives one setting, and a setting has one trigger of each kind.
  for (int i = _links.size() - 1; i >= 0; --i) {
    const Link& other = _links.at(i);
    if (other.trigger == link.trigger
        || (other.target == link.target && other.trigger.kind == link.trigger.kind))
      _links.removeAt(i);
  }
  _links.append(link);
  emit changed();
  return true;
}

bool LinkTable::removeAt(int index)
{
  if (index < 0 || index >= _links.size())
    return false;
  _links.removeAt(index);
  emit changed();
  return true;
}

bool LinkTable::remove(const LinkTarget& target, LinkTrigger::Kind kind)
{
  return removeAt(indexOf(target, kind));
}

bool LinkTable::setAction(int index, LinkAction action)
{
  if (index < 0 || index >= _links.size())
    return false;
  if (_links.at(index).action != action) {
    _links[index].action = action;
    emit changed();
  }
  return true;
}

void LinkTable::clear()
{
  if (_links.isEmpty())
    return;
  _links.clear();
  emit changed();
}

int LinkTable::indexOf(const LinkTrigger& trigger) const
{
  for (int i = 0; i < _links.size(); ++i) {
    if (_links.at(i).trigger == trigger)
      return i;
  }
  return -1;
}

int LinkTable::indexOf(const LinkTarget& target, LinkTrigger::Kind kind) const
{
  for (int i = 0; i < _links.size(); ++i) {
    if (_links.at(i).target == target && _links.at(i).trigger.kind == kind)
      return i;
  }
  return -1;
}

QList<int> LinkTable::indexesOf(const LinkTarget& target) const
{
  QList<int> indexes;
  for (int i = 0; i < _links.size(); ++i) {
    if (_links.at(i).target == target)
      indexes << i;
  }
  return indexes;
}

QJsonArray LinkTable::toJson() const
{
  QJsonArray array;
  for (const Link& link : _links)
    array.append(link.toJson());
  return array;
}

void LinkTable::fromJson(const QJsonArray& array, QStringList* skipped)
{
  _links.clear();
  for (qsizetype i = 0; i < array.size(); ++i) {
    Link link;
    QString reason;
    if (!Link::fromJson(array.at(i).toObject(), &link, &reason)) {
      skipped->append(QStringLiteral("links[%1]: %2").arg(i).arg(reason));
      continue;
    }
    // A later link contradicting an earlier one replaces it, as when learning.
    for (int j = _links.size() - 1; j >= 0; --j) {
      const Link& other = _links.at(j);
      if (other.trigger == link.trigger
          || (other.target == link.target && other.trigger.kind == link.trigger.kind))
        _links.removeAt(j);
    }
    _links.append(link);
  }
  emit changed();
}

} // namespace mmp
