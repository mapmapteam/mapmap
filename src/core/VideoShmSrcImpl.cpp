/*
 * VideoShmSrcImpl.cpp
 *
 * (c) 2016 Vasilis Liaskovitis -- vliaskov@gmail.com
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
 * (c) 2004 Mathieu Guindon, Julien Keable
 *           Based on code from Drone http://github.com/sofian/drone
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
#include "VideoShmSrcImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoShmSrcImpl::VideoShmSrcImpl() :
_attached(false)
{
}

bool VideoShmSrcImpl::getAttached()
{
  return _attached;
}

void VideoShmSrcImpl::setAttached(bool attach)
{
  _attached = attach;
}

bool VideoShmSrcImpl::loadMovie(const QString& path) {
  // Note: Shared memory source support is not yet implemented with Qt Multimedia
  // This would require a custom implementation using QSharedMemory or similar
  qWarning() << "Shared memory video sources are not currently supported with Qt Multimedia backend." << Qt::endl;
  qWarning() << "Attempted to load: " << path << Qt::endl;
  
  // For now, just mark as not attached
  _attached = false;
  _videoIsConnected = false;
  
  return false;
}

VideoShmSrcImpl::~VideoShmSrcImpl()
{
  // Cleanup if needed
}

}
