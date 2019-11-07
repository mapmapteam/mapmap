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

namespace mmp {

CameraImpl::CameraImpl() :
  _camera(nullptr),
  _cameraSurface(nullptr)
{

}

CameraImpl::~CameraImpl()
{
  _camera->stop();
  delete _camera;
  delete _cameraSurface;
}

bool CameraImpl::loadMovie(const QString &deviceName)
{
  VideoImpl::loadMovie(deviceName);

  _camera = new QCamera(deviceName.toLocal8Bit());

  _cameraSurface = new CameraSurface();

  _camera->setViewfinder(_cameraSurface);

  if (_camera->isAvailable())
    _camera->start();

  if (_camera->state() == QCamera::ActiveState)
    return true;

  if (_camera->error() != QCamera::NoError)
    QMessageBox(QMessageBox::Critical, "Camera Error",
                "Failed to start: " + _camera->errorString()).exec();

  return false;
}

int CameraImpl::getWidth() const
{
  return _cameraSurface->surfaceFormat().frameWidth();
}

int CameraImpl::getHeight() const
{
  return _cameraSurface->surfaceFormat().frameHeight();
}

const uchar *CameraImpl::getBits()
{
  return _cameraSurface->bits();
}

}
