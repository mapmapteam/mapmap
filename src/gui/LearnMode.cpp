/*
 * LearnMode.cpp
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

#include "LearnMode.h"

namespace mmp {

namespace {

/// The learn mode the widgets ask.
LearnMode* currentLearnMode = nullptr;

} // namespace

LearnMode::LearnMode(QObject* parent)
  : QObject(parent),
    _active(false)
{
}

LearnMode::~LearnMode()
{
  if (currentLearnMode == this)
    currentLearnMode = nullptr;
}

LearnMode* LearnMode::current()
{
  return currentLearnMode;
}

void LearnMode::makeCurrent()
{
  currentLearnMode = this;
}

void LearnMode::setLinks(LinkTable* links)
{
  disconnect(_linksConnection);
  _links = links;
  if (links)
    _linksConnection = connect(links, &LinkTable::changed, this, &LearnMode::linksChanged);
  emit linksChanged();
}

bool LearnMode::targetOf(const SettingGroup* root, const QString& path, LinkTarget* target) const
{
  if (!_finder || !root || path.isEmpty())
    return false;
  LinkTarget::ElementKind element;
  uid id;
  if (!_finder(root, &element, &id))
    return false;
  *target = LinkTarget::of(element, id, path);
  return true;
}

QString LearnMode::triggerText(const LinkTarget& target, LinkTrigger::Kind kind) const
{
  if (!_links)
    return QString();
  QStringList texts;
  for (int index : _links->indexesOf(target)) {
    if (_links->links().at(index).trigger.kind == kind)
      texts << _links->links().at(index).trigger.text();
  }
  return texts.join(QStringLiteral(", "));
}

QStringList LearnMode::triggerLines(const LinkTarget& target) const
{
  QStringList lines;
  if (!_links)
    return lines;
  for (int index : _links->indexesOf(target)) {
    const LinkTrigger& trigger = _links->links().at(index).trigger;
    lines << tr("%1: %2").arg(trigger.kindText(), trigger.text());
  }
  return lines;
}

void LearnMode::start()
{
  if (_active && !hasTarget())
    return;
  _active = true;
  _target = LinkTarget();
  emit changed();
}

void LearnMode::startWith(const LinkTarget& target)
{
  _active = true;
  _target = target;
  emit changed();
}

void LearnMode::stop()
{
  if (!_active)
    return;
  _active = false;
  _target = LinkTarget();
  emit changed();
}

void LearnMode::choose(const LinkTarget& target)
{
  if (_active && _target == target)
    return;
  startWith(target);
}

void LearnMode::clearTarget()
{
  if (!hasTarget())
    return;
  _target = LinkTarget();
  emit changed();
}

} // namespace mmp
