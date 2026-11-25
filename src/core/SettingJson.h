/*
 * SettingJson.h
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

/**
 * JSON descriptions of a settings tree, for the MCP server and any client that
 * wants to discover what an element lets it adjust.
 */

#ifndef SETTING_JSON_H_
#define SETTING_JSON_H_

#include <QJsonArray>
#include <QJsonObject>

#include "Setting.h"

namespace mmp {

/// Describes every setting below a group: one object per leaf, in declaration order,
/// the elements of a list included. Keys: path (relative to the group), label, type, value,
/// and when set: description, minimum, maximum, step, default, unit, displayScale (when not
/// 1), options (array of {id, label}), readOnly (true), available (false). Values and bounds
/// are in the stored range, not multiplied by displayScale. A colour is a text, a point or a range
/// an array of two numbers; the minimum and maximum of a point or a range are plain numbers
/// applying to each coordinate.
QJsonArray describeSettings(const SettingGroup* group);

/// Current values of the settings directly under a group, keyed by id (colors as "#rrggbb",
/// with the alpha first, "#aarrggbb", when not opaque).
QJsonObject settingValues(const SettingGroup* group);

} // namespace mmp

#endif /* SETTING_JSON_H_ */
