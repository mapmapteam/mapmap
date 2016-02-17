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
#include "MediaImpl.h"
#include <iostream>

UidAllocator Paint::allocator;

Paint::Paint(uid id) : Element(id, &allocator)
{
}

Paint::~Paint()
{
  allocator.free(getId());
}

bool Image::setUri(const QString &uri)
{
  this->uri = uri;
  build();
  return !image.isNull();
}

/* Implementation of the Video class */
Media::Media(int id) : Texture(id),
    uri(""),
    impl_(NULL)
{
  impl_ = new MediaImpl("", false);
  setRate(1);
  setVolume(1);
}

Media::Media(const QString uri_, bool live, double rate, uid id):
    Texture(id),
    uri(uri_),
    impl_(NULL)
{
  impl_ = new MediaImpl("", live);
  setRate(rate);
  setVolume(1);
  setUri(uri_);
}

// vertigo

Media::~Media()
{
  delete impl_;
}

void Media::build()
{
  this->impl_->build();
}

int Media::getWidth() const
{
  while (!this->impl_->videoIsConnected());
  return this->impl_->getWidth();
}

int Media::getHeight() const
{
  while (!this->impl_->videoIsConnected());
  return this->impl_->getHeight();
}

void Media::update() {
  impl_->update();
}

void Media::play()
{
  impl_->setPlayState(true);
}

void Media::pause()
{
  impl_->setPlayState(false);
}

void Media::rewind()
{
  impl_->resetMovie();
}

void Media::lockMutex() {
  impl_->lockMutex();
}

void Media::unlockMutex() {
  impl_->unlockMutex();
}

const uchar* Media::getBits()
{
  return this->impl_->getBits();
}

bool Media::bitsHaveChanged() const
{
  return this->impl_->bitsHaveChanged();
}

void Media::setRate(double rate)
{
  impl_->setRate(rate);
}

double Media::getRate() const
{
  return impl_->getRate();
}

void Media::setVolume(double rate)
{
  impl_->setVolume(rate);
}

double Media::getVolume() const
{
  return impl_->getVolume();
}

bool Media::hasVideoSupport()
{
  return MediaImpl::hasVideoSupport();
}

bool Media::setUri(const QString &uri)
{
  static QFileIconProvider provider;

  // Default (in case seeking and loading don't work).
  icon = provider.icon(QFileInfo(uri));

  // Try to load movie.
  bool success = false;
  this->uri = uri;
  success = impl_->loadMovie(uri);
  if (! success)
  {
    qDebug() << "Cannot load movie " << uri << "." << endl;
    return false;
  }

  // Try to get thumbnail.

  // Wait for the first samples to be available to make sure we are ready.
  if (!impl_->waitForNextBits(1000))
  {
    return false;
  }

  // Try seeking to the middle of the movie.
  if (!impl_->seekTo(0.5))
  {
    impl_->resetMovie();
    return false;
  }

  // Try to get a sample from the current position.
  // NOTE: There is no guarantee the sample has yet been acquired.
  const uint* bits;
  {
    qDebug() << "Second waiting wrong..." << endl;
    return false;
  }


  // Copy bits into thumbnail QImage.
  QImage thumbnail(getWidth(), getHeight(), QImage::Format_ARGB32);
  int i=0;
  for (int y=0; y<getHeight(); y++)
    for (int x=0; x<getWidth(); x++)
      thumbnail.setPixel(x, y, bits[i++]);

  icon = QIcon(QPixmap::fromImage(thumbnail));

  // Reset movie.
  impl_->resetMovie();

  // Return success.
  return success;
}

