/*
 * Element.cpp
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

#include "Element.h"

#include <QDebug>

namespace mmp {

/// Lowest opacity of an element: fully transparent.
static const float MIN_OPACITY = 0.0f;
/// Highest opacity of an element: fully opaque.
static const float MAX_OPACITY = 1.0f;
/// Factor between the opacity and the percentage shown to the user.
static const double OPACITY_DISPLAY_SCALE = 100.0;

Element::Element(uid id, UidAllocator* allocator) : _name(""), _isLocked(false), _opacity(MAX_OPACITY), _allocator(allocator)
{
  if (id == NULL_UID)
    id = allocator->allocate();
  else
  {
    Q_ASSERT(!allocator->exists(id));
    allocator->reserve(id);
  }
  // Assign id.
  _id = id;

  SettingGroup* root = settings();
  root->setListener(this);

  root->addSetting(SettingInfo::integer("id", tr("ID"))
                   .readOnly().notStored()
                   .description(tr("The number that identifies this element in the project.")))
      ->bind([this]() { return QVariant(int(getId())); },
             [](const QVariant&) { return false; });

  root->addSetting(SettingInfo::text("name", tr("Name"))
                   .hidden()
                   .description(tr("The name shown in the lists and used to find this element by OSC.")))
      ->bind([this]() { return QVariant(getName()); },
             [this](const QVariant& value) { setName(value.toString()); return true; });

  root->addSetting(SettingInfo::boolean("locked", tr("Locked"))
                   .hidden()
                   .description(tr("Whether this element is protected from changes made with the mouse.")))
      ->bind([this]() { return QVariant(isLocked()); },
             [this](const QVariant& value) { setLocked(value.toBool()); return true; });

  root->addSetting(SettingInfo::number("opacity", tr("Opacity"))
                   .valueType(QMetaType::Float)
                   .bounds(MIN_OPACITY, MAX_OPACITY)
                   .defaultValue(MAX_OPACITY)
                   .unit(SettingInfo::Percent)
                   .displayScale(OPACITY_DISPLAY_SCALE)
                   .description(tr("How opaque this element is, from fully transparent to fully opaque.")))
      ->bind([this]() { return QVariant(getOpacity()); },
             [this](const QVariant& value) { setOpacity(value.toFloat()); return true; });
}

Element::~Element() {
  _allocator->free(_id);
}

void Element::setName(const QString& name)
{
  if (name != _name)
  {
    _name = name;
    _emitPropertyChanged("name");
  }
}

void Element::setLocked(bool locked)
{
  if (locked != _isLocked)
  {
    _isLocked = locked;
    _emitPropertyChanged("locked");
  }
}

void Element::setOpacity(float opacity)
{
  // The bounds are those of the setting.
  bool ok = false;
  const QVariant clean = settings()->find("opacity")->sanitized(opacity, &ok);
  if (!ok)
    return;
  opacity = clean.toFloat();
  if (opacity != _opacity)
  {
    _opacity = opacity;
    _emitPropertyChanged("opacity");
  }
}

void Element::read(const QJsonObject& obj)
{
  Serializable::read(obj);

  // Check id.
  Q_ASSERT(_id == obj["id"].toInt());
}

void Element::write(QJsonObject& obj)
{
  Serializable::write(obj);
  // Set id.
  obj["id"] = getId();
}

void Element::settingValueChanged(const QString& path, const QVariant& value)
{
  emit settingChanged(getId(), path, value);
}

void Element::settingStructureChanged()
{
  emit settingsStructureChanged(getId());
}

void Element::_emitPropertyChanged(const QString& propertyName)
{
  SettingNode* node = settings()->findNode(propertyName);
  if (!node)
  {
    qWarning() << "Element" << getId() << "has no setting" << propertyName << "to notify";
    return;
  }

  if (node->kind() == SettingNode::SettingKind)
    static_cast<Setting*>(node)->notifyChanged();
  else if (node->kind() == SettingNode::ListKind)
    static_cast<SettingList*>(node)->notifyChanged();
}

}
