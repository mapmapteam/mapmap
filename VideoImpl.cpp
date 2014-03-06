/*
 * VideoImpl.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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
#include "VideoImpl.h"
#include <iostream>

// -------- private implementation of VideoImpl -------

bool VideoImpl::hasVideoSupport()
{
  static bool did_print_gst_version = false;
  if (! did_print_gst_version)
  {
    std::cout << "Using GStreamer version " <<
      GST_VERSION_MAJOR << "." <<
      GST_VERSION_MINOR << "." <<
      GST_VERSION_MICRO << std::endl;
    did_print_gst_version = true;
  }
  // TODO: actually check if we have it
  return true;
}

int VideoImpl::getWidth() const
{
  // TODO
  std::cout << "TODO" << std::endl;
  return 0;
}

int VideoImpl::getHeight() const
{
  // TODO
  std::cout << "TODO" << std::endl;
  return 0;
}

const uchar* VideoImpl::getBits() const
{
  // TODO
  std::cout << "TODO" << std::endl;
  return 0;
}

void VideoImpl::build()
{
  std::cout << "TODO" << std::endl;
}

VideoImpl::VideoImpl()
{
  std::cout << "TODO" << std::endl;
}

VideoImpl::~VideoImpl()
{
  std::cout << "TODO" << std::endl;
}

void VideoImpl::setUri(QString uri)
{
  this->uri_ = uri;
  std::cout << "TODO" << std::endl;
}

