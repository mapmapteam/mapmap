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
    _cameraSurface(nullptr)
{
}

CameraImpl::~CameraImpl()
{
  freeResources();
}

void CameraImpl::freeResources()
{
  if (_camera)
    _camera->stop();
  delete _camera;
  _camera = nullptr;
  delete _captureSession;
  _captureSession = nullptr;
  delete _cameraSurface;
  _cameraSurface = nullptr;
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

  _cameraSurface   = new CameraSurface();
  _camera          = new QCamera(dev);
  _captureSession  = new QMediaCaptureSession();

  _captureSession->setCamera(_camera);
  _captureSession->setVideoSink(_cameraSurface->videoSink());

  _camera->start();

  if (!_camera->isActive()) {
    qWarning() << "Failed to start camera.";
    return false;
  }

  _videoIsConnected = true;
  _setMovieReady(true);
  return true;
}

int CameraImpl::getWidth() const
{
  return _cameraSurface ? _cameraSurface->frameWidth() : -1;
}

int CameraImpl::getHeight() const
{
  return _cameraSurface ? _cameraSurface->frameHeight() : -1;
}

const uchar* CameraImpl::getBits()
{
  return _cameraSurface ? _cameraSurface->bits() : nullptr;
}

}
