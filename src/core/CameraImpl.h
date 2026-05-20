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

#include "CameraSurface.h"
#include "VideoImpl.h"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QCameraDevice>

namespace mmp {

class CameraImpl : public VideoImpl
{
public:
  CameraImpl();
  ~CameraImpl();

  bool loadMovie(const QString& deviceName) override;
  bool isLive() override { return true; }
  bool seekTo(qint64) override { return false; }

  int getWidth() const override;
  int getHeight() const override;

  const uchar* getBits() override;

  bool hasBits() const override { return _cameraSurface && _cameraSurface->isActive(); }
  bool bitsHaveChanged() const override { return true; }

private:
  QCamera                *_camera;
  QMediaCaptureSession   *_captureSession;
  CameraSurface          *_cameraSurface;
};

}

#endif // CAMERAIMPL_H_
