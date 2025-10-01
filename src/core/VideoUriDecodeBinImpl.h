/*
 * VideoUriDecodeBinImpl.h
 *
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

#ifndef VIDEO_URIDECODEBIN_IMPL_H_
#define VIDEO_URIDECODEBIN_IMPL_H_

// Qt Multimedia includes.
#include <QMediaPlayer>
#include <QVideoSink>
#include <QAudioOutput>

// Other includes.
#include "MM.h"
#include <QtOpenGL>
#include <QMutex>
#include <QWaitCondition>

#if __APPLE__
#include <OpenGL/gl.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "VideoImpl.h"

namespace mmp {

class VideoUriDecodeBinImpl : public VideoImpl 
{
  public:
  VideoUriDecodeBinImpl();
  ~VideoUriDecodeBinImpl();
  bool loadMovie(const QString& path);
  bool isLive() {return false;}

  private:
  // No additional members needed - Qt Multimedia handles everything
};

}

#endif /* ifndef */
