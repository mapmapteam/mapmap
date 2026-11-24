/*
 * CameraImpl.cpp
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

#include "CameraImpl.h"
#include <QMessageBox>
#include <QDebug>

namespace mmp {

CameraImpl::CameraImpl()
  : _camera(nullptr),
    _captureSession(nullptr),
    _videoSink(new QVideoSink)
{
  // Direct connection: frames are copied / converted on the thread that
  // delivers them (see _setFrame()), keeping that work off the GUI thread.
  //
  // Qt 6 delivers upright, top-left-origin frames here on every platform, the
  // same as the video-file path (VideoPlayerImpl), so no flip is needed. The
  // old flip (a historical OpenGL orientation workaround, mirror + 180-degree
  // rotation, net a vertical flip) made camera sources appear upside-down;
  // already removed on macOS/Windows in f1e51b4, confirmed to be the same bug
  // on Linux by injecting a synthetic top-red/bottom-blue QVideoFrame.
  QObject::connect(_videoSink, &QVideoSink::videoFrameChanged, _videoSink,
                   [this](const QVideoFrame& frame) {
                     if (frame.isValid())
                       _setFrame(frame);
                   }, Qt::DirectConnection);
}

CameraImpl::~CameraImpl()
{
  // Stop the camera before the sink goes away.
  _releaseCamera();
  delete _videoSink;
}

void CameraImpl::_releaseCamera()
{
  if (_camera)
    _camera->stop();
  delete _captureSession;
  _captureSession = nullptr;
  delete _camera;
  _camera = nullptr;
}

void CameraImpl::freeResources()
{
  _releaseCamera();
  VideoImpl::freeResources();
}

bool CameraImpl::loadMovie(const QString &deviceId)
{
  VideoImpl::loadMovie(deviceId);

  // Release any previously-opened device before re-acquiring.
  freeResources();

  // Find the camera device matching the given ID.
  QCameraDevice dev;
  for (const QCameraDevice& d : QMediaDevices::videoInputs()) {
    if (QString::fromUtf8(d.id()) == deviceId) {
      dev = d;
      break;
    }
  }

  if (dev.isNull()) {
    // Fall back to default camera.
    dev = QMediaDevices::defaultVideoInput();
    if (dev.isNull()) {
      qWarning() << "No camera available for device:" << deviceId;
      return false;
    }
  }

  _frameStats.setName(dev.description());

  _camera          = new QCamera(dev);
  _captureSession  = new QMediaCaptureSession();

  _captureSession->setCamera(_camera);
  _captureSession->setVideoSink(_videoSink);

  _camera->start();

  if (!_camera->isActive()) {
    qWarning() << "Failed to start camera.";
    return false;
  }

  _videoIsConnected = true;
  _setMovieReady(true);
  return true;
}

}
