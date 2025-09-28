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

#if QT_VERSION >= 0x060000
#include <QMediaDevices>
#include <QMediaCaptureSession>
#endif
#include <QMessageBox>

namespace mmp {

CameraImpl::CameraImpl() :
  _camera(nullptr),
  _cameraSurface(nullptr)
#if QT_VERSION >= 0x060000
  ,_captureSession(nullptr)
#endif
{

}

CameraImpl::~CameraImpl()
{
  if (_camera) {
#if QT_VERSION < 0x060000
    _camera->stop();
#else
    _camera->setActive(false);
    delete _captureSession;
#endif
  }
  delete _camera;
  delete _cameraSurface;
}

bool CameraImpl::loadMovie(const QString &deviceName)
{
  VideoImpl::loadMovie(deviceName);

#if QT_VERSION < 0x060000
  _camera = new QCamera(deviceName.toLocal8Bit());
#else
  // Find the camera device by ID
  const auto devices = QMediaDevices::videoInputs();
  QCameraDevice selectedDevice;
  for (const auto &device : devices) {
    if (device.id() == deviceName.toLocal8Bit()) {
      selectedDevice = device;
      break;
    }
  }
  
  if (selectedDevice.isNull() && !devices.isEmpty()) {
    selectedDevice = devices.first(); // fallback to first device
  }
  
  _camera = new QCamera(selectedDevice);
#endif

  _cameraSurface = new CameraSurface();

#if QT_VERSION < 0x060000
  _camera->setViewfinder(_cameraSurface);

  if (_camera->isAvailable())
    _camera->start();
#else
  _captureSession = new QMediaCaptureSession();
  _captureSession->setCamera(_camera);
  _captureSession->setVideoSink(_cameraSurface);

  if (_camera->isAvailable())
    _camera->setActive(true);
#endif

#if QT_VERSION < 0x060000
  if (_camera->state() == QCamera::ActiveState)
    return true;

  if (_camera->error() != QCamera::NoError)
    QMessageBox(QMessageBox::Critical, "Camera Error",
                "Failed to start: " + _camera->errorString()).exec();
#else
  if (_camera->isActive())
    return true;

  if (_camera->error() != QCamera::NoError)
    QMessageBox(QMessageBox::Critical, "Camera Error",
                "Failed to start: " + _camera->errorString()).exec();
#endif

  return false;
}

int CameraImpl::getWidth() const
{
#if QT_VERSION < 0x060000
  return _cameraSurface->surfaceFormat().frameWidth();
#else
  return _cameraSurface->frameWidth();
#endif
}

int CameraImpl::getHeight() const
{
#if QT_VERSION < 0x060000
  return _cameraSurface->surfaceFormat().frameHeight();
#else
  return _cameraSurface->frameHeight();
#endif
}

const uchar *CameraImpl::getBits()
{
  return _cameraSurface->bits();
}

}
