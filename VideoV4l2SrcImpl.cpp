/*
 * VideoV4l2SrcImpl.cpp
 *
 * (c) 2016 Vasilis Liaskovitis -- vliaskov@gmail.com
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
 * (c) 2004 Mathieu Guindon, Julien Keable
 *           Based on code from Drone http://github.com/sofian/drone
 *           Based on code from the GStreamer Tutorials http://docs.gstreamer.com/display/GstSDK/Tutorials
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
#include "VideoV4l2SrcImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoV4l2SrcImpl::VideoV4l2SrcImpl() :
_v4l2src0(NULL)
{
}


bool VideoV4l2SrcImpl::loadMovie(const QString& path) {
  VideoImpl::loadMovie(path);

  _v4l2src0 = gst_element_factory_make("v4l2src", NULL);

  if ( !_v4l2src0)
  {
    qWarning() << "Not all elements could be created." << endl;
    unloadMovie();
    return (-1);
  }

  // Build the pipeline. Note that we are NOT linking the source at this
  // point. We will do it later.
  gst_bin_add_many (GST_BIN (_pipeline),
      _v4l2src0,
      NULL);

  if (! gst_element_link_many (_v4l2src0, _queue0, NULL))
  {
    qDebug() << "Could not link v4l2src" << endl;
    unloadMovie();
    return false;
  }

  // Configure video appsink.
  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA,width=640,height=480");
  g_object_set (_capsfilter0, "caps", videoCaps, NULL);
  gst_caps_unref (videoCaps);

  // Retrieve meta-info.
  _width = 640;
  _height = 480;
  //_duration = ;
  _seekEnabled = false;

  setPlayState(true);
  return TRUE;
}

VideoV4l2SrcImpl::~VideoV4l2SrcImpl()
{
}
}
