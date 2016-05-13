/*
 * Paint.cpp
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

#include "Paint.h"
#include "VideoImpl.h"
#include "VideoUriDecodeBinImpl.h"
#include "VideoV4l2SrcImpl.h"
#include "VideoShmSrcImpl.h"
#include <iostream>

MM_BEGIN_NAMESPACE

UidAllocator Paint::allocator;

void Texture::update()
{
  if (textureId == 0)
  {
    glGenTextures(1, &textureId);
  }
}

void Texture::read(const QDomElement& obj)
{
  Paint::read(obj);
  // Set x and y.
  QDomElement e;
  e = obj.firstChildElement("x");
  if (!e.isNull())
    setX(e.text().toDouble());
  e = obj.firstChildElement("y");
  if (!e.isNull())
    setY(e.text().toDouble());
}

void Texture::write(QDomElement& obj)
{
  Paint::write(obj);
  _writeNode(obj, "x", QString::number(getX()));
  _writeNode(obj, "y", QString::number(getY()));
}

Paint::Paint(uid id)
  : Element(id, &allocator),
    _isPlaying(false)
{
}

Paint::~Paint()
{
  allocator.free(getId());
}

bool Image::setUri(const QString &uri)
{
  if (uri != _uri)
  {
    _uri = uri;
    build();
    _emitPropertyChanged("uri");
  }
  return !_image.isNull();
}

/* Implementation of the Video class */
Video::Video(int id) : Texture(id),
    _uri(""),
    _impl(NULL)
{
  _impl = new VideoUriDecodeBinImpl();
  setRate(1);
  setVolume(1);
}

Video::Video(const QString uri_, VideoType type, double rate, uid id):
    Texture(id),
    _uri(""),
    _impl(NULL)
{
  switch (type) {
    case VIDEO_URI:
      _impl = new VideoUriDecodeBinImpl();
      break;
    case VIDEO_WEBCAM:
      _impl = new VideoV4l2SrcImpl();
      break;
    case VIDEO_SHMSRC:
      _impl = new VideoShmSrcImpl();
      break;
    default:
      fprintf (stderr, "Could not determine type for video source\n ");
      break;
  }
  //_impl = new VideoShmSrcImpl();//V4l2SrcImpl();//UriDecodeBinImpl();
  setRate(rate);
  setVolume(1);
  setUri(uri_);
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
  // Check if we're actually changing the uri.
  if (uri != _uri)
  {
    // Try to load movie.
    if (!_impl->loadMovie(uri))
    {
      qDebug() << "Cannot load movie " << uri << "." << endl;
      return false;
    }

    // Set uri.
    _uri = uri;

    // Try to get thumbnail.
    // Wait for the first samples to be available to make sure we are ready.
    if (!_impl->waitForNextBits(1000))
    {
      qDebug() << "No bits coming" << endl;
      return false;
    }

    if (!_generateThumbnail())
      qDebug() << "Could not generate thumbnail for " << uri << ": using generic icon." << endl;

    _emitPropertyChanged("uri");
  }

  // Return success.
  return true;
}

void Video::_doPlay()
{
  _impl->setPlayState(true);
}

void Video::_doPause()
{
  _impl->setPlayState(false);
}

bool Video::_generateThumbnail()
{
  static QFileIconProvider provider;

  // Default (in case seeking and loading don't work).
  _icon = provider.icon(QFileInfo(_uri));

  // Try seeking to the middle of the movie.
  if (!_impl->seekTo(0.5))
  {
    _impl->resetMovie();
    return false;
  }

  // Try to get a sample from the current position.
  // NOTE: There is no guarantee the sample has yet been acquired.
  const uchar* bits;
  if (!_impl->waitForNextBits(ICON_TIMEOUT, &bits))
  {
    qDebug() << "Second waiting wrong..." << endl;
    return false;
  }

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
  _icon = QIcon(QPixmap::fromImage(thumbnail));

  // Reset movie.
  _impl->resetMovie();

  return true;
}

MM_END_NAMESPACE
