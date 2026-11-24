/*
 * CameraImpl.h
 *
 * (c) 2019 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef CAMERAIMPL_H_
#define CAMERAIMPL_H_

#include "VideoImpl.h"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QVideoSink>

namespace mmp {

/**
 * Video implementation for a capture device (camera).
 *
 * Frames go through VideoImpl::_setFrame(), like those of a video file: YUV
 * frames that YuvTexture can draw are converted to RGB on the GPU, the others
 * on the CPU.
 */
class CameraImpl : public VideoImpl
{
public:
  CameraImpl();
  ~CameraImpl();

  bool loadMovie(const QString& deviceName) override;
  bool isLive() override { return true; }
  bool seekTo(qint64) override { return false; }

  /// The sink that receives the frames of the camera.
  QVideoSink* videoSink() const { return _videoSink; }

protected:
  // Stops and releases the capture device (called by unloadMovie() and dtor).
  void freeResources() override;

private:
  // Stops and releases the capture device, without touching the frames.
  void _releaseCamera();

  QCamera                *_camera;
  QMediaCaptureSession   *_captureSession;
  // Kept for the life of the object: frames are delivered to _setFrame().
  QVideoSink             *_videoSink;
};

}

#endif // CAMERAIMPL_H_
