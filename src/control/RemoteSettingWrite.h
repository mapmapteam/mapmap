/*
 * RemoteSettingWrite.h
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

#ifndef REMOTE_SETTING_WRITE_H_
#define REMOTE_SETTING_WRITE_H_

#include <QVariantList>

namespace mmp {

class Element;
class Setting;

/// Returns the preference "Allow message with existing media source".
bool sameMediaSourceAllowed();

/**
 * Writes arguments received over OSC or MCP to a setting of an element, as
 * Setting::setFromArguments() does. When sameMediaSource is true, a video given the file
 * it already plays reloads it, as the preference "Allow message with existing media
 * source" asks. Returns false when the setting refuses the arguments.
 */
bool writeRemoteSetting(Element& element, Setting* setting, const QVariantList& arguments,
                        bool sameMediaSource);

} // namespace mmp

#endif /* REMOTE_SETTING_WRITE_H_ */
