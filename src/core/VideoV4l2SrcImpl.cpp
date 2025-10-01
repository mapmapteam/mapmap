/*
 * VideoV4l2SrcImpl.cpp
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
#include "VideoV4l2SrcImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoV4l2SrcImpl::VideoV4l2SrcImpl() :
_camera(nullptr),
_captureSession(nullptr)
{
}


bool VideoV4l2SrcImpl::loadMovie(const QString& path) {
  Q_UNUSED(path);
  
  // Create camera and capture session
  _camera = new QCamera();
  _captureSession = new QMediaCaptureSession();
  _videoSink = new QVideoSink();
  
  if (!_camera || !_captureSession || !_videoSink)
  {
    qWarning() << "Camera components could not be created." << Qt::endl;
    unloadMovie();
    return false;
  }

  // Connect the video sink to receive frames
  connect(_videoSink, &QVideoSink::videoFrameChanged, this, &VideoImpl::onVideoFrameChanged);

  // Set up the capture session
  _captureSession->setCamera(_camera);
  _captureSession->setVideoSink(_videoSink);

  // Configure default resolution
  _width = 640;
  _height = 480;
  _seekEnabled = false;
  
  // Start the camera
  _camera->start();
  
  _videoIsConnected = true;
  _setMovieReady(true);
  
  return true;
}

VideoV4l2SrcImpl::~VideoV4l2SrcImpl()
{
  if (_camera) {
    _camera->stop();
    delete _camera;
  }
  if (_captureSession) {
    delete _captureSession;
  }
}
}
