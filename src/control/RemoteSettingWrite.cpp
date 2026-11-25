/*
 * RemoteSettingWrite.cpp
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

#include "RemoteSettingWrite.h"

#include <QSettings>

#include "MM.h"
#include "Setting.h"
#include "Source.h"

namespace mmp {

bool sameMediaSourceAllowed()
{
  return QSettings().value("oscSameMediaSource", MM::OSC_SAME_MEDIA_SOURCE).toBool();
}

bool writeRemoteSetting(Element& element, Setting* setting, const QVariantList& arguments,
                        bool sameMediaSource)
{
  // Allowed to, a video given the file it already plays reloads it: Video::setUri() reads
  // the same preference. Through the setting, an unchanged value would do nothing.
  Video* video = sameMediaSource ? qobject_cast<Video*>(&element) : nullptr;
  if (video && setting == video->settings()->find("uri") && arguments.size() == 1
      && arguments.first().toString() == video->getUri())
  {
    return video->setUri(video->getUri());
  }

  return setting->setFromArguments(arguments);
}

} // namespace mmp
