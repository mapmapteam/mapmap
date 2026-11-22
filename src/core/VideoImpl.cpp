/*
 * VideoImpl.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
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
#include "VideoImpl.h"
#include <QSettings>
#include <QDebug>

namespace mmp {

bool VideoImpl::hasVideoSupport()
{
  static bool printed = false;
  if (!printed) {
    qDebug() << "Using Qt Multimedia" << QT_VERSION_STR;
    printed = true;
  }
  return true;
}

int VideoImpl::getWidth() const  { return _width; }
int VideoImpl::getHeight() const { return _height; }
QString VideoImpl::getUri() const { return _uri; }

// Returns true if the video pixel format carries an alpha channel.
static bool _pixelFormatHasAlpha(QVideoFrameFormat::PixelFormat format)
{
  switch (format) {
  case QVideoFrameFormat::Format_ARGB8888:
  case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
  case QVideoFrameFormat::Format_BGRA8888:
  case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
  case QVideoFrameFormat::Format_ABGR8888:
  case QVideoFrameFormat::Format_RGBA8888:
  case QVideoFrameFormat::Format_AYUV:
  case QVideoFrameFormat::Format_AYUV_Premultiplied:
    return true;
  default:
    return false;
  }
}

QImage VideoImpl::frameToImage(const QVideoFrame& frame)
{
  QImage img = frame.toImage();
  switch (img.format()) {
  // Directly uploadable (see Util::glUploadFormat()).
  case QImage::Format_RGB32:
  case QImage::Format_ARGB32:
  case QImage::Format_RGBX8888:
  case QImage::Format_RGBA8888:
    return img;

  // Qt 6 delivers decoded YUV video as premultiplied RGBA. Opaque pixels are
  // identical premultiplied or not, so just relabel (no copy, no per-pixel pass).
  case QImage::Format_RGBA8888_Premultiplied:
    if (!_pixelFormatHasAlpha(frame.pixelFormat()) &&
        img.reinterpretAsFormat(QImage::Format_RGBX8888))
      return img;
    break;
  case QImage::Format_ARGB32_Premultiplied:
    if (!_pixelFormatHasAlpha(frame.pixelFormat()) &&
        img.reinterpretAsFormat(QImage::Format_RGB32))
      return img;
    break;

  default:
    break;
  }

  // Anything else (real alpha, other layouts): convert to straight alpha to
  // match our GL_SRC_ALPHA blending.
  return img.convertToFormat(QImage::Format_ARGB32);
}

const uchar* VideoImpl::getBits()
{
  _bitsChanged = false;
  return _data;
}

void VideoImpl::setRate(double rate)
{
  if (rate == 0.0) {
    qDebug() << "Cannot set rate to zero, ignoring.";
    return;
  }
  _rate = rate;
}

void VideoImpl::setVolume(double volume)
{
  _volume = volume;
}

void VideoImpl::build()
{
  qDebug() << "Building video impl";
  if (!loadMovie(_uri))
    qDebug() << "Cannot load movie" << _uri;
}

VideoImpl::VideoImpl()
  : _data(nullptr),
    _bitsChanged(false),
    _width(-1),
    _height(-1),
    _duration(0),
    _videoIsConnected(false),
    _audioIsConnected(false),
    _seekEnabled(false),
    _rate(1.0),
    _volume(1.0),
    _terminate(false),
    _movieReady(false),
    _playState(false),
    _uri("")
{
  QSettings settings;
  _playInLoop = settings.value("playInLoop", MM::PLAY_IN_LOOP).toBool();
}

VideoImpl::~VideoImpl()
{
  freeResources();
}

void VideoImpl::unloadMovie()
{
  _terminate = false;
  _seekEnabled = false;
  _setMovieReady(false);
  setPlayState(false);
  freeResources();
}

void VideoImpl::freeResources()
{
  _data = nullptr;
  _currentFrame = QImage();
  _bitsChanged = false;
  _width = _height = -1;
  _duration = 0;
  _videoIsConnected = false;
  _audioIsConnected = false;
}

bool VideoImpl::loadMovie(const QString& filename)
{
  _uri = filename;
  return true;
}

bool VideoImpl::setPlayState(bool play)
{
  _playState = play;
  return true;
}

bool VideoImpl::seekTo(double position)
{
  if (_duration <= 0)
    return false;
  position = qBound(0.0, position, 1.0);
  return seekTo(qint64(position * _duration));
}

void VideoImpl::resetMovie()
{
  if (_seekEnabled)
    seekTo(qint64(0));
  else
    loadMovie(_uri);
}

void VideoImpl::update()
{
  // Subclasses set _terminate via their own EOS detection.
  if (_terminate) {
    if (_playInLoop)
      resetMovie();
    _terminate = false;
  }
}

void VideoImpl::lockMutex()   { _mutex.lock(); }
void VideoImpl::unlockMutex() { _mutex.unlock(); }

}
