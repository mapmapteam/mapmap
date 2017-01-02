/*
 * VideoShmSrcImpl.h
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

#ifndef VIDEO_SHMSRC_IMPL_H_
#define VIDEO_SHMSRC_IMPL_H_

// GStreamer includes.
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>

// Other includes.
#include "MM.h"
#include <QtOpenGL>
#include <QMutex>
#include <QWaitCondition>

#include <glib.h>
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "VideoImpl.h"

namespace mmp {

class VideoShmSrcImpl : public VideoImpl 
{
  public:
  VideoShmSrcImpl();
  ~VideoShmSrcImpl();
  bool loadMovie(const QString& path);
  bool isLive() {return true;}
  bool getAttached();
  void setAttached(bool attach);

  private:
  GstElement *_shmsrc0;
  GstElement *_gdpdepay0;
  /**
   * shmsrc socket poller.
   */
  GSource *_pollSource;
  /// Whether or not we are attached to a shmsrc.
  bool _attached;
};

}
#endif /* ifndef */
