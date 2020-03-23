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
#include <QCameraInfo>

namespace mmp {

class CameraImpl : public VideoImpl
{
public:
  CameraImpl();
  ~CameraImpl();

  bool loadMovie(const QString& deviceName);
  bool isLive() { return true; }

  int getWidth() const;
  int getHeight() const;

  const uchar* getBits();

  bool hasBits() const { return _cameraSurface->isActive(); }

  bool bitsHaveChanged() const { return true; }

private:
  QCamera *_camera;
  CameraSurface *_cameraSurface;

};

}

#endif // CAMERAIMPL_H_
