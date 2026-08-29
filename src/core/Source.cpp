/*
 * Source.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#include "Source.h"
#include "VideoImpl.h"
#include "VideoPlayerImpl.h"
#include "CameraImpl.h"
#include <iostream>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QImageReader>
#include <QSettings>
#include <QTimer>

namespace mmp {

UidAllocator Source::allocator;

QVector<GLuint> Texture::_orphanedTextures;
QMutex          Texture::_orphanedTexturesMutex;

void Texture::orphanTexture(GLuint id)
{
  QMutexLocker locker(&_orphanedTexturesMutex);
  _orphanedTextures.append(id);
}

void Texture::deleteOrphanedTextures()
{
  QMutexLocker locker(&_orphanedTexturesMutex);
  if (_orphanedTextures.isEmpty())
    return;
  glDeleteTextures((GLsizei) _orphanedTextures.size(), _orphanedTextures.constData());
  _orphanedTextures.clear();
}

void Texture::update()
{
  if (textureId == 0)
  {
    glGenTextures(1, &textureId);
  }
}

void Texture::read(const QJsonObject& obj)
{
  Source::read(obj);
  if (obj.contains("x"))
    setX(obj["x"].toDouble());
  if (obj.contains("y"))
    setY(obj["y"].toDouble());
}

void Texture::write(QJsonObject& obj)
{
  Source::write(obj);
  obj["x"] = getX();
  obj["y"] = getY();
}

Source::Source(uid id)
  : Element(id, &allocator),
    _isPlaying(false)
{
}

Source::~Source()
{
  allocator.free(getId());
}

Image::Image(int id)
  : Texture(id),
    _rate(0),
    _currentFrame(0),
    _currentFrameReal(0.0),
    _prevTime(0),
    _bits(0)
  {
    setRate(1.0);
  }

Image::Image(const QString uri_, uid id)
  : Texture(id),
    _rate(0),
    _currentFrame(-1),
    _currentFrameReal(0.0),
    _prevTime(0),
    _bits(0)
  {
    setUri(uri_);
    setRate(1.0);
  }

bool Image::setUri(const QString &uri)
{
  if (uri != _uri)
  {
    _uri = uri;
    build();
    _emitPropertyChanged("uri");
  }
  return !_images.isEmpty();
}

void Image::build()
{
  // Read all images and convert to RGBA for direct upload to OpenGL.
  QImageReader reader(_uri);
  _images.clear();
  for (int i = 0; i < reader.imageCount(); i++) {
    QImage raw = reader.read().convertToFormat(QImage::Format_RGBA8888);
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    _images.push_back(raw.mirrored(true, false).transformed(QTransform().rotate(180)));
    QT_WARNING_POP
  }

  rewind();
}

void Image::update()
{
  if (isAnimation() && isPlaying())
  {
    // Compute the interval of time since last call to update().
    qreal currentTime = _elapsedTime();
    qreal diffTime = currentTime - _prevTime;

    // Update next frame.
    _currentFrameReal += diffTime * _rate * MM::DEFAULT_FRAMES_PER_SECOND;
    _currentFrameReal = wrapAround(_currentFrameReal, (qreal)_images.size());
    uint nextFrame = (int)_currentFrameReal;

    // If frame changed, update image bits pointer.
    if (nextFrame != _currentFrame)
    {
      _currentFrame = nextFrame;
      _bits = _images[_currentFrame].bits();
      bitsChanged = true;
    }

    // Reset previous time.
    _prevTime = currentTime;
  }
}

void Image::rewind()
{
  // Reset/restart everything.
  if (isAnimation())
  {
    _currentFrame     = 0;
    _currentFrameReal = 0.0;
    _prevTime         = 0;
    _timer.start();
  }
  _bits = _images.isEmpty() ? 0 : _images[0].bits();
  bitsChanged = true;
}

const uchar* Image::getBits() {
  return _bits;
}

QIcon Image::getIcon() const
{
  static QFileIconProvider provider;

  if (!_images.isEmpty())
    // Create icon from image.
    return QIcon(QPixmap::fromImage(_images[0]).scaled(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE,
                                    Qt::IgnoreAspectRatio));
  else
    // Return default icon from filesystem.
    return provider.icon(QFileInfo(_uri));
}


void Image::setRate(double rate)
{
  _rate = rate;
}

void Image::_doPlay()
{
  _prevTime = _elapsedTime();
}

/* Implementation of the Video class */
Video::Video(int id) : Texture(id),
    _uri(""),
    _videoType(VIDEO_URI),
    _impl(nullptr)
{
  _impl = new VideoPlayerImpl();
  setRate(1);
  setVolume(1);
}

Video::Video(const QString uri_, VideoType type, double rate, uid id):
    Texture(id),
    _uri(""),
    _videoType(type),
    _impl(nullptr)
{
  switch (type) {
    case VIDEO_URI:
      _impl = new VideoPlayerImpl();
      break;
    case VIDEO_WEBCAM:
      _impl = new CameraImpl();
      break;
    default:
      _impl = new VideoPlayerImpl();
      break;
  }
  setRate(rate);
  setVolume(1);
  setUri(uri_);
  _videoType = type;
}

// vertigo

Video::~Video()
{
  delete _impl;
}

void Video::build()
{
  this->_impl->build();
}

int Video::getWidth() const
{
  return this->_impl->getWidth();
}

int Video::getHeight() const
{
  return this->_impl->getHeight();
}

void Video::update() {
  _impl->update();
  Texture::update();
}

void Video::rewind()
{
  _impl->resetMovie();
}

void Video::releaseResources()
{
  // Release the capture device / media player but keep the impl object, so the
  // device is freed as soon as the source is removed (e.g. to let the same
  // camera be re-opened) without invalidating the source. See reacquireResources().
  if (_impl)
    _impl->unloadMovie();
}

void Video::reacquireResources()
{
  // Re-open using the stored URI/device id (set by loadMovie()).
  if (_impl)
    _impl->build();
}

void Video::lockMutex() {
  _impl->lockMutex();
}

void Video::unlockMutex() {
  _impl->unlockMutex();
}

const uchar* Video::getBits()
{
  return this->_impl->getBits();
}

bool Video::bitsHaveChanged() const
{
  return this->_impl->bitsHaveChanged();
}

void Video::setRate(double rate)
{
  if (rate != _impl->getRate())
  {
    _impl->setRate(rate);
    _emitPropertyChanged("rate");
  }
}

double Video::getRate() const
{
  return _impl->getRate();
}

void Video::setVolume(double volume)
{
  if (volume != _impl->getVolume())
  {
    _impl->setVolume(volume);
    _emitPropertyChanged("volume");
  }
}

double Video::getVolume() const
{
  return _impl->getVolume();
}

bool Video::hasVideoSupport()
{
  return VideoImpl::hasVideoSupport();
}

bool Video::setUri(const QString &uri)
{
  QSettings settings;
  bool sameMediaSourceOSC = settings.value("oscSameMediaSource").toBool();
  // Check if we're actually changing the uri.
  // In some case with OSC message the user may need to allow
  // the same media source (uri)
  if (sameMediaSourceOSC || uri != _uri)
  {
    // Try to load movie.
    if (!_impl->loadMovie(uri))
    {
      qDebug() << "Cannot load movie " << uri << "." << Qt::endl;
      return false;
    }

    // Set uri.
    _uri = uri;

    // Show a generic icon right away. A real thumbnail (video files only) is
    // filled in below, asynchronously, once frames actually start arriving:
    // we cannot wait for it here without blocking the GUI thread, which is
    // also the thread Qt Multimedia needs free to deliver those frames.
    _setFallbackIcon();

    const int maxAttempts = ICON_TIMEOUT / THUMBNAIL_POLL_INTERVAL;

    if (_videoType == VIDEO_WEBCAM)
    {
      // No thumbnail to generate for a camera: just confirm the feed comes up.
      _pollForBits([this]() {
        _emitPropertyChanged("icon");
      }, maxAttempts);
    }
    else
    {
      _pollForBits([this, maxAttempts]() {
        // Try seeking to the middle of the movie for a representative frame.
        if (_impl->seekTo(0.5))
        {
          _pollForBits([this]() {
            if (!_generateThumbnail())
              qDebug() << "Could not generate thumbnail for " << _uri << ": using generic icon." << Qt::endl;
            _impl->resetMovie();
            _emitPropertyChanged("icon");
          }, maxAttempts);
        }
        else
        {
          _impl->resetMovie();
          _emitPropertyChanged("icon");
        }
      }, maxAttempts);
    }

    _emitPropertyChanged("uri");

    // Return success.
    return true;
  }

  return false;
}

void Video::_doPlay()
{
  _impl->setPlayState(true);
}

void Video::_doPause()
{
  _impl->setPlayState(false);
}

void Video::_setFallbackIcon()
{
  static QFileIconProvider provider;

  _icon = provider.icon(QFileInfo(_uri));
  if (_icon.isNull())
    _icon = (_videoType == VIDEO_WEBCAM) ? QIcon(":/add-camera") : QIcon(":/add-video");
}

void Video::_pollForBits(std::function<void()> onReady, int attemptsLeft)
{
  if (_impl->hasBits() && _impl->bitsHaveChanged())
  {
    onReady();
    return;
  }

  if (attemptsLeft <= 0)
  {
    qDebug() << "No bits coming for " << _uri << Qt::endl;
    return;
  }

  // Re-check shortly, giving the Qt event loop a chance to actually run and
  // deliver the frame we're waiting for.
  QTimer::singleShot(THUMBNAIL_POLL_INTERVAL, this, [this, onReady, attemptsLeft]() {
    _pollForBits(onReady, attemptsLeft - 1);
  });
}

bool Video::_generateThumbnail()
{
  // Assumes the caller (_pollForBits()'s onReady callback) has already
  // confirmed a fresh frame is available.
  const uchar* bits = _impl->getBits();
  if (!bits)
    return false;

  // Copy bits into thumbnail QImage.
  QImage thumbnail(getWidth(), getHeight(), QImage::Format_ARGB32);
  for (int y=0; y<getHeight(); y++)
    for (int x=0; x<getWidth(); x++)
    {
      // Transfer RGBA to ARGB.
      uint r = *bits++;
      uint b = *bits++;
      uint g = *bits++;
      bits++; // skip alpha
      thumbnail.setPixel(x, y, qRgb(r, g, b));
    }

  // Generate icon.
  _icon = QIcon(QPixmap::fromImage(thumbnail).scaled(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE,
                                                     Qt::IgnoreAspectRatio));

  return true;
}

}
