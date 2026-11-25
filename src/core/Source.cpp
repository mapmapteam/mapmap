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
#include "FrameStats.h"
#include "Util.h"
#include "YuvTexture.h"
#include <iostream>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QImageReader>
#include <QSettings>
#include <QTimer>

namespace mmp {

/// Factor between a value kept between 0 and 1 and the percentage shown to the user.
static const double PERCENT_DISPLAY_SCALE = 100.0;
/// Lowest audio volume of a video: silent.
static const double MIN_VOLUME = 0.0;
/// Highest audio volume of a video: as loud as the file.
static const double MAX_VOLUME = 1.0;

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

void Texture::bind()
{
  glBindTexture(GL_TEXTURE_2D, textureId);
  uploadBits();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  // Linear minification avoids shimmering when sources are drawn smaller.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::uploadBits()
{
  lockMutex();
  _uploadBitsLocked();
  unlockMutex();
}

void Texture::_uploadBitsLocked()
{
  if (bitsHaveChanged())
  {
    int width  = getWidth();
    int height = getHeight();
    const uchar* bits = getBits(); // Resets bitsHaveChanged().
    Util::uploadTextureBits(width, height, getBitsFormat(), getBitsType(), bits,
                            _uploadedWidth, _uploadedHeight);
  }
}

Texture::Texture(uid id) :
  Source(id),
  textureId(0),
  x(0),
  y(0),
  bitsChanged(false),
  _uploadedWidth(0),
  _uploadedHeight(0)
{
  SettingGroup* root = settings();

  // Read-only: x and y are written by the project reader, see read().
  root->addSetting(SettingInfo::number("x", tr("X"))
                   .valueType(QMetaType::Float)
                   .readOnly().notStored().hidden()
                   .description(tr("The horizontal position of the top-left corner of the texture.")))
      ->bind([this]() { return QVariant(float(getX())); },
             [](const QVariant&) { return false; });

  root->addSetting(SettingInfo::number("y", tr("Y"))
                   .valueType(QMetaType::Float)
                   .readOnly().notStored().hidden()
                   .description(tr("The vertical position of the top-left corner of the texture.")))
      ->bind([this]() { return QVariant(float(getY())); },
             [](const QVariant&) { return false; });
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

Color::Color(int id) : Source(id)
{
  settings()->addSetting(SettingInfo::color("color", tr("Color"))
                         .description(tr("The colour that fills the layers showing this source.")))
      ->bind([this]() { return QVariant(getColor()); },
             [this](const QVariant& value) { setColor(value.value<QColor>()); return true; });
}

Color::Color(const QColor& color_, uid id) : Color(int(id))
{
  color = color_;
}

void Color::setColor(const QColor& color_)
{
  if (color_ != color)
  {
    color = color_;
    _emitPropertyChanged("color");
  }
}

Image::Image(int id)
  : Texture(id),
    _rate(0),
    _currentFrame(0),
    _currentFrameReal(0.0),
    _prevTime(0),
    _bits(0)
  {
    _declareSettings();
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
    _declareSettings();
    setUri(uri_);
    setRate(1.0);
  }

void Image::_declareSettings()
{
  SettingGroup* root = settings();

  root->addSetting(SettingInfo::filePath("uri", tr("Image file"),
                                         tr("Image files (%1);;All files (*)").arg(MM::IMAGE_FILES_FILTER))
                   .description(tr("The image file shown by this source.")))
      ->bind([this]() { return QVariant(getUri()); },
             [this](const QVariant& value) {
               // The path is kept even when no image can be read from it, as when a project is read.
               setUri(value.toString());
               return true;
             });

  root->addSetting(SettingInfo::number("rate", tr("Speed"))
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_DISPLAY_SCALE)
                   .description(tr("How fast an animated image plays; a negative speed plays it backwards.")))
      ->bind([this]() { return QVariant(getRate()); },
             [this](const QVariant& value) { setRate(value.toDouble()); return true; });
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
    // Upload rows top-first, like video and camera frames (see
    // CameraImpl::CameraImpl()): the old mirror + 180-degree
    // rotation, a net vertical flip, made images appear upside-down.
    _images.push_back(reader.read().convertToFormat(QImage::Format_RGBA8888));
  }

  rewind();
}

void Image::update()
{
  // Allocate our own texture name (images used to share texture 0).
  Texture::update();

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
  bitsChanged = false;
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
  if (rate != _rate)
  {
    _rate = rate;
    _emitPropertyChanged("rate");
  }
}

void Image::_doPlay()
{
  _prevTime = _elapsedTime();
}

/* Implementation of the Video class */
Video::Video(int id) : Video(VIDEO_URI, id)
{
}

Video::Video(VideoType type, int id) : Texture(id),
    _uri(""),
    _videoType(type),
    _impl(nullptr),
    _yuv(nullptr),
    _yuvActive(false)
{
  switch (type) {
    case VIDEO_WEBCAM:
      _impl = new CameraImpl();
      break;
    case VIDEO_URI:
    default:
      _impl = new VideoPlayerImpl();
      break;
  }

  // Declared once _impl exists, before the setters below notify them.
  SettingGroup* root = settings();

  root->addSetting(SettingInfo::filePath("uri", tr("Source"),
                                         tr("Video files (%1);;All files (*)").arg(MM::VIDEO_FILES_FILTER))
                   .description(tr("The video file, or the camera, shown by this source.")))
      ->bind([this]() { return QVariant(getUri()); },
             [this](const QVariant& value) { return setUri(value.toString()); });

  root->addSetting(SettingInfo::number("rate", tr("Speed"))
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_DISPLAY_SCALE)
                   .description(tr("How fast the video plays; a negative speed plays it backwards.")))
      ->bind([this]() { return QVariant(getRate()); },
             [this](const QVariant& value) { setRate(value.toDouble()); return true; });

  root->addSetting(SettingInfo::number("volume", tr("Volume"))
                   .bounds(MIN_VOLUME, MAX_VOLUME)
                   .unit(SettingInfo::Percent)
                   .displayScale(PERCENT_DISPLAY_SCALE)
                   .description(tr("How loud the sound of the video plays.")))
      ->bind([this]() { return QVariant(getVolume()); },
             [this](const QVariant& value) { setVolume(value.toDouble()); return true; });

  setRate(1);
  setVolume(1);
}

Video::Video(const QString uri_, VideoType type, double rate, uid id):
    Video(type, id)
{
  setRate(rate);
  setUri(uri_);
}

// vertigo

Video::~Video()
{
  delete _impl;
  if (_yuv) {
    // No GL context is current here (see orphanTexture()).
    for (GLuint id : _yuv->textureIds())
      orphanTexture(id);
    delete _yuv;
  }
}

void Video::bind()
{
  lockMutex();
  const bool uploading = _impl->bitsHaveChanged();
  const bool measuring = uploading && FrameStats::isEnabled();
  const qint64 startNs = measuring ? FrameStats::now() : 0;

  if (uploading)
  {
    if (_impl->hasVideoFrame() && YuvTexture::isAvailable())
    {
      if (!_yuv)
        _yuv = new YuvTexture;
      _yuvActive = _yuv->upload(_impl->takeVideoFrame());
    }
    else
      _yuvActive = false; // RGB frame: uploaded below.
  }

  if (_yuvActive)
    _yuv->bind();
  else
  {
    glBindTexture(GL_TEXTURE_2D, textureId);
    _uploadBitsLocked();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  if (measuring)
  {
    // OpenGL works asynchronously: without glFinish() this only times
    // the sending of the commands, not the copy itself.
    if (FrameStats::isSynchronous())
      glFinish();
    const qint64 endNs = FrameStats::now();
    _impl->frameStats()->record(FrameStats::UPLOAD, endNs - startNs, endNs);
  }
  unlockMutex();
}

FrameStats* Video::frameStats() const
{
  return _impl->frameStats();
}

void Video::release()
{
  if (_yuvActive)
    _yuv->release();
}

/* Implementation of the Camera class */
Camera::Camera(int id) : Video(VIDEO_WEBCAM, id)
{
}

Camera::Camera(const QString deviceId, double rate, uid id):
    Video(deviceId, VIDEO_WEBCAM, rate, id)
{
}

bool Camera::isCaptureDeviceId(const QString& uri)
{
  if (uri.isEmpty())
    return false;

  for (const QCameraDevice& device : QMediaDevices::videoInputs())
  {
    if (QString::fromUtf8(device.id()) == uri)
      return true;
  }
  return false;
}

void Video::build()
{
  this->_impl->build();
}

int Video::getWidth() const
{
  int w = this->_impl->getWidth();
  return w > 0 ? w : DEFAULT_WIDTH;
}

int Video::getHeight() const
{
  int h = this->_impl->getHeight();
  return h > 0 ? h : DEFAULT_HEIGHT;
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

GLenum Video::getBitsFormat() const
{
  GLenum format, type;
  Util::glUploadFormat(_impl->getFrameFormat(), format, type);
  return format;
}

GLenum Video::getBitsType() const
{
  GLenum format, type;
  Util::glUploadFormat(_impl->getFrameFormat(), format, type);
  return type;
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
  // The bounds are those of the setting.
  bool ok = false;
  const QVariant clean = settings()->find("volume")->sanitized(volume, &ok);
  if (!ok)
    return;
  volume = clean.toDouble();
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

    const int firstFrameMaxAttempts = FIRST_FRAME_TIMEOUT / THUMBNAIL_POLL_INTERVAL;
    const int thumbnailMaxAttempts  = ICON_TIMEOUT / THUMBNAIL_POLL_INTERVAL;

    if (_videoType == VIDEO_WEBCAM)
    {
      // No thumbnail to generate for a camera: just confirm the feed comes up.
      _pollForBits([this]() {
        emit frameSizeKnown(getId(), getWidth(), getHeight());
        emit iconChanged(getId());
      }, firstFrameMaxAttempts);
    }
    else
    {
      _pollForBits([this]() {
        // The first frame just arrived, so getWidth()/getHeight() now report
        // the real resolution instead of the DEFAULT_WIDTH/HEIGHT placeholder.
        emit frameSizeKnown(getId(), getWidth(), getHeight());

        // Try seeking to the middle of the movie for a representative frame.
        if (_impl->seekTo(0.5))
        {
          _pollForBits([this]() {
            if (!_generateThumbnail())
              qDebug() << "Could not generate thumbnail for " << _uri << ": using generic icon." << Qt::endl;
            _impl->resetMovie();
            emit iconChanged(getId());
          }, thumbnailMaxAttempts);
        }
        else
        {
          _impl->resetMovie();
          emit iconChanged(getId());
        }
      }, firstFrameMaxAttempts);
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
  // Copy the frame without consuming it (getBits() would reset
  // bitsHaveChanged() and delay the first texture upload).
  QImage thumbnail = _impl->getFrameImage();
  if (thumbnail.isNull())
    return false;

  // Generate icon.
  _icon = QIcon(QPixmap::fromImage(thumbnail).scaled(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE,
                                                     Qt::IgnoreAspectRatio));

  return true;
}

}
