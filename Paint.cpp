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

Paint::Paint(uid id)
{
  if (id == NULL_UID)
    id = allocator.allocate();
  else
  {
    Q_ASSERT(!allocator.exists(id));
    allocator.reserve(id);
  }
  // Assign id.
  _id = id;
}

Paint::~Paint()
{
  allocator.free(_id);
}

bool Image::setUri(const QString &uri)
{
  this->uri = uri;
  build();
  return true;
}

/* Implementation of the Video class */

Media::Media(const QString uri_, bool live, double rate, uid id):
    Texture(id),
    uri(uri_),
    impl_(NULL)
{
  impl_ = new MediaImpl(uri_, live);
  setRate(rate);
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

bool Media::hasVideoSupport()
{
  return MediaImpl::hasVideoSupport();
}

bool Media::setUri(const QString &uri)
{
  bool success = false;
  this->uri = uri;
  success = this->impl_->loadMovie(uri);
  if (! success)
    qDebug() << "Cannot load movie " << uri << "." << endl;
  return success;
}

