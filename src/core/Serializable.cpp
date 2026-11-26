/*
 * Serializable.cpp
 *
 * (c) 2016 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "Serializable.h"

namespace mmp {

namespace {

/// Returns true when a project file gives the value of the setting: stored and writable.
bool isReadFromProject(const Setting* setting)
{
  return setting->info().isStored() && !setting->info().isReadOnly();
}

/// Returns false for a Point or a Range, whose Setting::toText() is empty.
bool hasTextForm(const Setting* setting)
{
  const SettingInfo::Type type = setting->info().type();
  return type != SettingInfo::Point && type != SettingInfo::Range;
}

} // namespace

Serializable::Serializable()
{
}

SettingGroup* Serializable::settings() const
{
  if (!_settings)
  {
    // The tree exists before the declarations run: they call settings() themselves.
    Serializable* self = const_cast<Serializable*>(this);
    _settings.reset(new SettingGroup);
    _settings->setSettingAddedHandler([self](Setting* setting) { self->_takePendingKey(setting); });
    self->_declareSettings(_settings.get());
  }
  return _settings.get();
}

QString Serializable::cleanClassName() const
{
  return classNameRealToClean(metaObject()->className());
}

QString Serializable::classNameRealToClean(const QString& realClassName)
{
  Q_ASSERT(realClassName.startsWith(MM::NAMESPACE_PREFIX));
  // Removes the NAMESPACE:: prefix.
  return realClassName.right( realClassName.size() - MM::NAMESPACE_PREFIX.size() );
}

QString Serializable::classNameCleanToReal(const QString& cleanClassName)
{
  // A malformed class name from a project will simply fail registry lookup.
  return MM::NAMESPACE_PREFIX + cleanClassName;
}

void Serializable::read(const QJsonObject& obj)
{
  _pendingKeys = QJsonObject();

  // Declaration order: a setting may need one declared before it (the columns and rows of a
  // mesh before its vertices).
  QStringList settingIds;
  const QList<SettingNode*> children = settings()->children();
  for (SettingNode* node : children)
  {
    if (node->kind() != SettingNode::SettingKind)
      continue;

    // The key of a setting that a project file does not give stays pending.
    Setting* setting = static_cast<Setting*>(node);
    if (!isReadFromProject(setting))
      continue;
    settingIds.append(setting->id());
    settingIds.append(setting->info().formerIds());
    const QString key = projectKey(obj, setting);
    if (!key.isEmpty())
      _readSetting(setting, obj.value(key));
  }

  const QStringList reservedKeys = _reservedKeys();
  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
  {
    const QString key = it.key();
    if (key == ProjectLabels::CLASS_NAME || reservedKeys.contains(key) || settingIds.contains(key))
      continue;
    _pendingKeys.insert(key, it.value());
  }
}

void Serializable::write(QJsonObject& obj)
{
  // First, so that what this object writes takes precedence.
  for (auto it = _pendingKeys.constBegin(); it != _pendingKeys.constEnd(); ++it)
    obj[it.key()] = it.value();

  obj[ProjectLabels::CLASS_NAME] = cleanClassName();

  const QList<SettingNode*> children = settings()->children();
  for (SettingNode* node : children)
  {
    if (node->kind() != SettingNode::SettingKind)
      continue;

    const Setting* setting = static_cast<const Setting*>(node);
    if (!setting->info().isStored())
      continue;

    if (!hasTextForm(setting))
    {
      qWarning() << cleanClassName() << "setting" << setting->id()
                 << "has no text form: it is not written in the project file";
      continue;
    }

    switch (setting->info().type())
    {
      case SettingInfo::Number:
      case SettingInfo::Integer:
      case SettingInfo::Boolean:
        obj[setting->id()] = QJsonValue::fromVariant(setting->value());
        break;
      default:
        obj[setting->id()] = setting->toText();
        break;
    }
  }
}

QString Serializable::projectKey(const QJsonObject& obj, const Setting* setting)
{
  if (obj.contains(setting->id()))
    return setting->id();
  for (const QString& formerId : setting->info().formerIds())
  {
    if (obj.contains(formerId))
      return formerId;
  }
  return QString();
}

QStringList Serializable::_reservedKeys() const
{
  return QStringList();
}

void Serializable::_declareSettings(SettingGroup* root)
{
  Q_UNUSED(root);
}

void Serializable::_readSetting(Setting* setting, const QJsonValue& value) const
{
  if (value.isString())
  {
    const QString text = value.toString();
    bool readable = false;
    setting->sanitized(text, &readable);
    if (!readable || !hasTextForm(setting))
    {
      qWarning() << cleanClassName() << "setting" << setting->id()
                 << "cannot be read from" << text << ": skipped";
      return;
    }
    // The owner may still refuse the value, as a video file that cannot be opened: it says why.
    setting->setFromText(text);
  }
  else if (value.isDouble() || value.isBool())
  {
    // Warns by itself when the value is refused.
    setting->setFromArguments({ value.toVariant() });
  }
  else
  {
    qWarning() << cleanClassName() << "setting" << setting->id()
               << "cannot be read from" << value << ": skipped";
  }
}

void Serializable::_takePendingKey(Setting* setting)
{
  // A setting that a project file does not give leaves the key for the next save.
  if (!isReadFromProject(setting))
    return;
  const QString key = projectKey(_pendingKeys, setting);
  if (key.isEmpty())
    return;

  _readSetting(setting, _pendingKeys.take(key));
  // The key of an earlier version is not written again beside the id.
  for (const QString& formerId : setting->info().formerIds())
    _pendingKeys.remove(formerId);
}

}
