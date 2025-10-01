/*
 * VideoUriDecodeBinImpl.cpp
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
#include "VideoUriDecodeBinImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoUriDecodeBinImpl::VideoUriDecodeBinImpl()
{
}

bool VideoUriDecodeBinImpl::loadMovie(const QString& path) {
  // Use the base class implementation which now uses Qt Multimedia
  return VideoImpl::loadMovie(path);
}

VideoUriDecodeBinImpl::~VideoUriDecodeBinImpl()
{
}

}
