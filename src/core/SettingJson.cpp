/*
 * SettingJson.cpp
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

#include "SettingJson.h"

#include <QColor>
#include <QPointF>

namespace mmp {

namespace {

/// Factor between a value and the number shown, for a value shown as it is.
const double DEFAULT_DISPLAY_SCALE = 1.0;

/// Returns the lowercase name of a type, as given in the descriptions.
QString typeName(SettingInfo::Type type)
{
  switch (type) {
    case SettingInfo::Number:    return QStringLiteral("number");
    case SettingInfo::Integer:   return QStringLiteral("integer");
    case SettingInfo::Boolean:   return QStringLiteral("boolean");
    case SettingInfo::Color:     return QStringLiteral("color");
    case SettingInfo::Text:      return QStringLiteral("text");
    case SettingInfo::Choice:    return QStringLiteral("choice");
    case SettingInfo::Point:     return QStringLiteral("point");
    case SettingInfo::Range:     return QStringLiteral("range");
    case SettingInfo::Reference: return QStringLiteral("reference");
  }
  return QString();
}

/// Returns the lowercase name of a unit; empty for no unit.
QString unitName(SettingInfo::Unit unit)
{
  switch (unit) {
    case SettingInfo::NoUnit:  return QString();
    case SettingInfo::Percent: return QStringLiteral("percent");
    case SettingInfo::Degrees: return QStringLiteral("degrees");
    case SettingInfo::Hertz:   return QStringLiteral("hertz");
    case SettingInfo::Seconds: return QStringLiteral("seconds");
    case SettingInfo::Pixels:  return QStringLiteral("pixels");
  }
  return QString();
}

/// Converts a value of a given type to JSON: a colour to text, a point or a range (a QPointF)
/// to an array of two numbers, anything else as QJsonValue does. A scalar given for a point or
/// a range, such as a bound, stays a number.
QJsonValue toJson(SettingInfo::Type type, const QVariant& value)
{
  if (type == SettingInfo::Color) {
    const QColor color = qvariant_cast<QColor>(value);
    return color.alpha() < 255 ? color.name(QColor::HexArgb) : color.name();
  }
  if ((type == SettingInfo::Point || type == SettingInfo::Range) && value.typeId() == QMetaType::QPointF) {
    const QPointF pair = value.toPointF();
    return QJsonArray { pair.x(), pair.y() };
  }
  return QJsonValue::fromVariant(value);
}

/// Describes one leaf under the given path.
QJsonObject describeSetting(const QString& path, const Setting* setting)
{
  const SettingInfo& info = setting->info();
  const SettingInfo::Type type = info.type();

  QJsonObject object;
  object["path"] = path;
  object["label"] = info.label();
  object["type"] = typeName(type);
  object["value"] = toJson(type, setting->value());
  if (!info.description().isEmpty())
    object["description"] = info.description();
  if (info.minimum().isValid())
    object["minimum"] = toJson(type, info.minimum());
  if (info.maximum().isValid())
    object["maximum"] = toJson(type, info.maximum());
  if (info.step() > 0)
    object["step"] = info.step();
  if (info.defaultValue().isValid())
    object["default"] = toJson(type, info.defaultValue());
  if (info.unit() != SettingInfo::NoUnit)
    object["unit"] = unitName(info.unit());
  if (info.displayScale() != DEFAULT_DISPLAY_SCALE)
    object["displayScale"] = info.displayScale();
  if (type == SettingInfo::Choice || type == SettingInfo::Reference) {
    QJsonArray options;
    for (const SettingOption& option : info.options())
      options.append(QJsonObject { { "id", option.id }, { "label", option.label } });
    object["options"] = options;
  }
  if (info.isReadOnly())
    object["readOnly"] = true;
  if (!setting->isAvailable())
    object["available"] = false;
  return object;
}

/// Appends the description of every leaf below a node, paths starting with prefix.
void describeNode(SettingNode* node, const QString& prefix, QJsonArray* out)
{
  const QString path = prefix.isEmpty() ? node->id() : prefix + "/" + node->id();
  switch (node->kind()) {
    case SettingNode::SettingKind:
      out->append(describeSetting(path, static_cast<Setting*>(node)));
      break;
    case SettingNode::GroupKind:
      for (SettingNode* child : static_cast<SettingGroup*>(node)->children())
        describeNode(child, path, out);
      break;
    case SettingNode::ListKind: {
      SettingList* list = static_cast<SettingList*>(node);
      for (int index = 0; index < list->count(); index++) {
        const Setting* element = list->at(index);
        if (element)
          out->append(describeSetting(path + "/" + QString::number(index), element));
      }
      break;
    }
  }
}

} // namespace

QJsonArray describeSettings(const SettingGroup* group)
{
  QJsonArray settings;
  if (!group)
    return settings;
  for (SettingNode* child : group->children())
    describeNode(child, QString(), &settings);
  return settings;
}

QJsonObject settingValues(const SettingGroup* group)
{
  QJsonObject values;
  if (!group)
    return values;
  for (const SettingNode* child : group->children()) {
    if (child->kind() != SettingNode::SettingKind)
      continue;
    const Setting* setting = static_cast<const Setting*>(child);
    values[setting->id()] = toJson(setting->info().type(), setting->value());
  }
  return values;
}

} // namespace mmp
