/*
 * VideoImplQtMultiMedia.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
 * (c) 2004 Mathieu Guindon, Julien Keable
 *           Based on code from Drone http://github.com/sofian/drone
 *           Based on code from the GStreamer Tutorials http://docs.gstreamer.com/display/GstSDK/Tutorials
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

#include "VideoImplQtMultiMedia.h"
#include <cstring>
#include <iostream>

namespace mmp {

// -------- private implementation of VideoImpl -------

bool VideoImplQtMultiMedia::hasVideoSupport()
{
  // TODO: actually check if we have it
  return true;
}

int VideoImplQtMultiMedia::getWidth() const
{
  return videoSurface->surfaceFormat().frameWidth();
//  return _width;
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.width;
}

int VideoImplQtMultiMedia::getHeight() const
{
  return videoSurface->surfaceFormat().frameHeight();
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.height;
}

const uchar* VideoImplQtMultiMedia::getBits()
{
  // Reset bits changed.
//  _bitsChanged = false;

  // Return data.
  return _img.bits();
//  return videoSurface->bits();
//  return (hasBits() ? videoSurface->bits() : NULL);
}

QString VideoImplQtMultiMedia::getUri() const
{
  return _uri;
}

void VideoImplQtMultiMedia::setRate(double rate)
{
  if (rate == 0)
  {
    qDebug() << "Cannot set rate to zero, ignoring rate " << rate << endl;
    return;
  }

  // Only update rate if needed.
  if (_rate != rate)
  {
    _rate = rate;

    mediaPlayer.setPlaybackRate(_rate);
  }
}

void VideoImplQtMultiMedia::setVolume(double volume)
{
  // Only update volume if needed.
  if (_volume != volume)
  {
    _volume = volume;

    mediaPlayer.setMuted(_volume <= 0);
    if (_volume > 0)
    {
      // Convert volume using logarithmic scale.
      // qreal linearVolume = QAudio::convertVolume(volume,
      //                                            QAudio::LogarithmicVolumeScale,
      //                                            QAudio::LinearVolumeScale);

      qreal linearVolume = _volume;

      mediaPlayer.setVolume(qRound(linearVolume * 100));
    }
  }
}

void VideoImplQtMultiMedia::build()
{
  qDebug() << "Building video impl";
  if (!loadMovie(_uri))
  {
    qDebug() << "Cannot load movie " << _uri << ".";
  }
}


VideoImplQtMultiMedia::VideoImplQtMultiMedia() :
//_width(-1),
//_height(-1),
//_duration(0),
//_seekEnabled(false),
_bitsChanged(false),
_data(NULL),
//_isSeekable(false),
_rate(1.0),
_movieReady(false),
_playState(false),
_uri("")
{
  _mutexLocker = new QMutexLocker(&_mutex);

  videoSurface = new VideoSurface(this);
  videoProbe = new QVideoProbe(this);
  videoProbe->setSource(&mediaPlayer);
  _freeze = false;

  mediaPlayer.setVideoOutput(videoSurface);

  _bitsChanged = true;
  connect(&mediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
          this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

  connect(videoProbe, SIGNAL(videoFrameProbed(QVideoFrame)), this, SLOT(processFrame(QVideoFrame)));
  //connect(videoSurface, SIGNAL(frameAvailable(QImage)),
  //        this, SLOT(processImage(QImage)));

  // connect(&mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
  //         this, SLOT(mediaStateChanged(QMediaPlayer::State)));
  // connect(&mediaPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
  // connect(&mediaPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(durationChanged(qint64)));
}

VideoImplQtMultiMedia::~VideoImplQtMultiMedia()
{
  // Free all resources.
  //freeResources();

  // Free mutex locker object.
  delete _mutexLocker;
}

// void VideoImplQtMultiMedia::unloadMovie()
// {
//   // // Reset variables.
//   // _terminate = false;
//   // _seekEnabled = false;
//   //
//   // // Un-ready.
//   // _setMovieReady(false);
//   setPlayState(false);
// }

void VideoImplQtMultiMedia::resetMovie()
{
  if (seekIsEnabled())
  {
    if (_rate > 0)
      seekTo((qint64)0);
    else
    {
      // NOTE: Untested.
      seekTo(mediaPlayer.duration());
      //_updateRate();
    }
  }
  else
  {
    qDebug() << "Seeking not enabled: reloading the movie" << endl;
    loadMovie(_uri);
  }
}

void VideoImplQtMultiMedia::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  if (status == QMediaPlayer::EndOfMedia)
  {
//      loadMovie(_uri);
//      _freeze = true;
//      videoSurface->setFreeze(true);

//    resetMovie();
  }
  else if (status == QMediaPlayer::BufferedMedia)
  {
//      _freeze = false;
//    setPlayState(_playState);
//    videoSurface->setFreeze(false);
  }
}

void VideoImplQtMultiMedia::processImage(QImage img)
{
    _img = img;
}

void VideoImplQtMultiMedia::processFrame(QVideoFrame frame)
{
    if (!frame.isValid())
        return;

    if (_freeze)
        return;

    // Copy current frame.
    QVideoFrame currentFrame(frame);
    QImage img;

    // Convert frame into QImage with appropriate format.
    // Source: https://stackoverflow.com/questions/27829830/convert-qvideoframe-to-qimage
    if (currentFrame.map(QAbstractVideoBuffer::ReadOnly))
    {
      QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(currentFrame.pixelFormat());
      if (imageFormat != QImage::Format_Invalid) {
        img = QImage(currentFrame.bits(), currentFrame.width(), currentFrame.height(), currentFrame.bytesPerLine(), imageFormat);
      } else {
          qWarning() << "Strange" << endl;
         // e.g. JPEG
         int nbytes = currentFrame.mappedBytes();
         img = QImage::fromData(currentFrame.bits(), nbytes);
      }


      // Convert to OpenGLformat and apply transforms to straighten.
      img = QGLWidget::convertToGLFormat(img)
              .mirrored(true, false)
              .transformed(QTransform().rotate(180));

//      qWarning() << img.isNull() << " " << img.allGray() << " " << currentFrame.bits() << " " << currentFrame << endl;

      if (!img.isNull())
          _img = img;

      currentFrame.unmap();

    }
}


void VideoImplQtMultiMedia::update()
{
  // // Check for end-of-stream or terminate.
  // if (_eos() || _terminate)
  // {
  //   _setFinished(true);
  //   resetMovie();
  // }
  // else
  // {
  //   _setFinished(false);
  // }
  //
}

 bool VideoImplQtMultiMedia::loadMovie(const QString& filename) {
   qDebug() << "Opening movie: " << filename << ".";

   // Assign URI.
   _uri = filename;

   // Free previously allocated structures
   //unloadMovie();

   // Start playing.
   if (!filename.isEmpty()) {
       mediaPlayer.stop();
       mediaPlayer.setMedia(QUrl::fromLocalFile(filename));
       setPlayState(_playState);
   }

   return true;
 }

bool VideoImplQtMultiMedia::setPlayState(bool play)
{
  _playState = play;
  if (play)
    mediaPlayer.play();
  else
    mediaPlayer.pause();
  return true;
}

bool VideoImplQtMultiMedia::seekTo(double position)
{
  qint64 duration = mediaPlayer.duration();

  // Make sure position is in [0,1].
  position = qBound(0.0, position, 1.0);

  // Seek at position in nanoseconds.
  return seekTo((qint64)(position*duration));
}

bool VideoImplQtMultiMedia::seekTo(qint64 positionMilliSeconds)
{
  // if (!_appsink0 || !_seekEnabled)
  // {
  //   return false;
  // }
  // else
  // {
    lockMutex();

    // Free the current sample and reset.
    //_freeCurrentSample();
//    _bitsChanged = false;

    // Seek to position.
    mediaPlayer.setPosition(positionMilliSeconds);
    unlockMutex();

    return true;
  // }
}

//bool VideoImplQtMultiMedia::_preRun()
//{
//  // Check for end-of-stream or terminate.
//  if (_eos() || _terminate)
//  {
//    _setFinished(true);
//    resetMovie();
//  }
//  else
//  {
//    _setFinished(false);
//  }
//  if (!_movieReady ||
//      !_padHandlerData.videoIsConnected)
//  {
//    return false;
//  }
//  return true;
//}

void VideoImplQtMultiMedia::lockMutex()
{
  _mutexLocker->relock();
}

void VideoImplQtMultiMedia::unlockMutex()
{
  _mutexLocker->unlock();
}

bool VideoImplQtMultiMedia::waitForNextBits(int timeout, const uchar** bits)
{
  QTime time;
  time.start();
  while (time.elapsed() < timeout)
  {
    // Bits available.
    if (hasBits() && bitsHaveChanged())
    {
      if (bits)
        *bits = getBits();
      return true;
    }
  }

  // Timed out.
  return false;
}

}
