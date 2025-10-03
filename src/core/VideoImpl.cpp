/*
 * VideoImpl.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
 * (c) 2004 Mathieu Guindon, Julien Keable
 *           Based on code from Drone http://github.com/sofian/drone
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
#include <cstring>
#include <iostream>
#include <QElapsedTimer>
#include <QUrl>
#include <QFile>
#include <QImage>
#include <QSettings>

namespace mmp {

// -------- private implementation of VideoImpl -------

bool VideoImpl::hasVideoSupport()
{
  static bool did_print_qt_version = false;
  if (! did_print_qt_version)
  {
    qDebug() << "Using Qt Multimedia version " << QT_VERSION_STR << Qt::endl;
    did_print_qt_version = true;
  }
  return true;
}

int VideoImpl::getWidth() const
{
  return _width;
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.width;
}

int VideoImpl::getHeight() const
{
  return _height;
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.height;
}

const uchar* VideoImpl::getBits()
{
  // Reset bits changed.
  _bitsChanged = false;

  // Return data.
  return (hasBits() ? _data : NULL);
}

QString VideoImpl::getUri() const
{
  return _uri;
}

void VideoImpl::setRate(double rate)
{
  if (rate == 0.0)
  {
    qDebug() << "Cannot set rate to zero, ignoring rate " << rate << Qt::endl;
    return;
  }

  // Only update rate if needed.
  if (_rate != rate)
  {
    _rate = rate;

    // Send seek events to activate rate.
    if (_seekEnabled)
      _updateRate();
  }
}

void VideoImpl::setVolume(double volume)
{
  // Only update volume if needed.
  if (_volume != volume)
  {
    _volume = volume;

    // Set volume on audio output
    if (audioIsSupported() && _audioOutput)
    {
      _audioOutput->setMuted(_volume <= 0);
      _audioOutput->setVolume(_volume);
    }
    else
      qWarning() << "Cannot change volume cause this video does not support audio." << Qt::endl;
  }
}

void VideoImpl::build()
{
  qDebug() << "Building video impl";
  if (!loadMovie(_uri))
  {
    qDebug() << "Cannot load movie " << _uri << ".";
  }
}

VideoImpl::~VideoImpl()
{
    // Free all resources.
    freeResources();

    // Free mutex locker object only if created (we don't create one anymore).
    if (_mutexLocker) {
        delete _mutexLocker;
        _mutexLocker = nullptr;
    }
}

bool VideoImpl::_eos() const
{
  if (_movieReady && _mediaPlayer)
  {
    QMediaPlayer::PlaybackState state = _mediaPlayer->playbackState();
    qint64 position = _mediaPlayer->position();
    qint64 duration = _mediaPlayer->duration();
    
    if (_rate > 0.0)
    {
      // Check if we've reached the end
      return (state == QMediaPlayer::StoppedState && position >= duration);
    }
    else
    {
      // For reverse playback, check if position is at 0
      return (position == 0);
    }
  }
  else
    return false;
}

void VideoImpl::onVideoFrameChanged(const QVideoFrame &frame)
{
  // Make it thread-safe.
  lockMutex();

  if (!frame.isValid())
  {
    unlockMutex();
    return;
  }

  // Get a copy of the frame
  QVideoFrame clonedFrame(frame);
  if (!clonedFrame.map(QVideoFrame::ReadOnly))
  {
    unlockMutex();
    return;
  }

  // Update dimensions if needed
  if (_width == -1 || _height == -1)
  {
    _width = clonedFrame.width();
    _height = clonedFrame.height();
  }

  // Free previous frame data
  _freeCurrentFrame();

  // Convert frame to RGBA format for OpenGL texture usage
  QImage image = clonedFrame.toImage();
  if (!image.isNull())
  {
    // Convert to RGBA8888 format which matches the old GStreamer RGBA format
    image = image.convertToFormat(QImage::Format_RGBA8888);
    
    // Allocate and copy frame data
    _currentFrameData = new QByteArray(reinterpret_cast<const char*>(image.bits()), image.sizeInBytes());
    _data = reinterpret_cast<uchar*>(_currentFrameData->data());
    _bitsChanged = true;
  }

  clonedFrame.unmap();
  unlockMutex();
}

VideoImpl::VideoImpl() :
    QObject(),
    _width(-1),
    _height(-1),
    _duration(0),
    _seekEnabled(false),
    _mediaPlayer(nullptr),
    _videoSink(nullptr),
    _audioOutput(nullptr),
    _currentFrameData(nullptr),
    _bitsChanged(false),
    _data(nullptr),
    _rate(1.0),
    _movieReady(false),
    _playState(false),
    _uri("")
{
#if QT_VERSION < 0x060000
    // Do NOT create a persistent QMutexLocker here. That would lock the mutex permanently
    // and calling relock() later can cause deadlocks between threads.
    _mutexLocker = nullptr;
#else
    _mutexLocker = nullptr;
#endif

    QSettings settings;
    _playInLoop = settings.value("playInLoop", MM::PLAY_IN_LOOP).toBool();
}

void VideoImpl::unloadMovie()
{
  // Reset variables.
  _terminate = false;
  _seekEnabled = false;

  // Un-ready.
  _setMovieReady(false);
  setPlayState(false);

  // Free allocated resources / reinit.
  freeResources();
}

void VideoImpl::freeResources()
{
  // Stop media player first
  if (_mediaPlayer)
  {
    _mediaPlayer->stop();
    delete _mediaPlayer;
    _mediaPlayer = nullptr;
  }

  // Free video sink
  if (_videoSink)
  {
    delete _videoSink;
    _videoSink = nullptr;
  }

  // Free audio output
  if (_audioOutput)
  {
    delete _audioOutput;
    _audioOutput = nullptr;
  }

  qDebug() << "Freeing remaining frame data" << Qt::endl;

  // Frees current frame data
  _freeCurrentFrame();

  // Reset other informations.
  _bitsChanged = false;
  _width = _height = (-1);
  _duration = 0;
  _videoIsConnected = false;
  _audioIsConnected = false;
}

void VideoImpl::resetMovie()
{
  if (_seekEnabled && _mediaPlayer)
  {
    if (_rate > 0.0)
    {
      seekTo(0LL);
      qWarning() << "update Rate" << Qt::endl;
      _updateRate();
    }
    else
    {
      // NOTE: Untested.
      seekTo(_duration);
      qWarning() << "update Rate" << Qt::endl;
      _updateRate();
    }
  }
  else
  {
    qDebug() << "Seeking not enabled: reloading the movie" << Qt::endl;
    loadMovie(_uri);
  }
}

bool VideoImpl::createVideoComponents()
{
  // Already supported?
  if (videoIsSupported())
    return true;

  // Video components are now managed by QMediaPlayer
  // This method is kept for compatibility with the interface
  return (_mediaPlayer != nullptr);
}

bool VideoImpl::createAudioComponents()
{
  // Already supported?
  if (audioIsSupported())
    return true;

  // Audio components are now managed by QMediaPlayer and QAudioOutput
  // This method is kept for compatibility with the interface
  return (_audioOutput != nullptr);
}

void VideoImpl::update()
{
  // Check for end-of-stream or terminate.
  if (_eos() || _terminate)
  {
    _setFinished(true);
    if (_playInLoop) // Check if repeat mode is on
      resetMovie();
  }
  else
  {
    _setFinished(false);
  }

//  // Check if movie is ready and connected.
//  if (!isReady())
//  {
//    _bitsChanged = false;
//  }
//
  // Check gstreamer messages on bus.
  _checkMessages();
}

 bool VideoImpl::loadMovie(const QString& filename) {
   // Verify if file exists.
   if (!QFile::exists(filename))
   {
     qDebug() << "File " << filename << " does not exist" << Qt::endl;
     return false;
   }

   qDebug() << "Opening movie: " << filename << ".";

   // Assign URI.
   _uri = filename;

   // Free previously allocated structures
   unloadMovie();

   // Prepare handler data.
   _videoIsConnected = false;
   _audioIsConnected = false;

   // Create Qt Multimedia components
   _mediaPlayer = new QMediaPlayer();
   _videoSink = new QVideoSink();
   _audioOutput = new QAudioOutput();

   if (!_mediaPlayer || !_videoSink || !_audioOutput)
   {
     qWarning() << "Media components could not be created." << Qt::endl;
     unloadMovie();
     return false;
   }

   // Connect the video sink to receive frames
   connect(_videoSink, &QVideoSink::videoFrameChanged, this, &VideoImpl::onVideoFrameChanged);

   // Set up the media player
   _mediaPlayer->setVideoSink(_videoSink);
   _mediaPlayer->setAudioOutput(_audioOutput);

   // Connect signals for media status changes
   connect(_mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
     _duration = duration;
     qDebug() << "Duration: " << duration << " ms" << Qt::endl;
   });

   connect(_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
       if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia)
       {
           _seekEnabled = _mediaPlayer->isSeekable();
           qDebug() << "Media loaded. Seekable: " << _seekEnabled << Qt::endl;
           _setMovieReady(true);
           _videoIsConnected = true;
           // Avoid referencing types that require QMediaMetaData to be fully defined
           // (some Qt versions forward-declare media metadata types). Use audio output validity instead.
           _audioIsConnected = (_audioOutput != nullptr);
       }
   });

   connect(_mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
     qWarning() << "Media player error: " << errorString << Qt::endl;
     Q_UNUSED(error);
   });

   // Set the source
   _mediaPlayer->setSource(QUrl::fromLocalFile(filename));

   return true;
 }

bool VideoImpl::setPlayState(bool play)
{
  if (_mediaPlayer == nullptr)
  {
    return false;
  }

  // Change state.
  if (play)
    _mediaPlayer->play();
  else
    _mediaPlayer->pause();

  _playState = play;
  return true;
}

bool VideoImpl::seekTo(double position)
{
  if (!_mediaPlayer)
  {
    qDebug() << "Cannot seek: no media player" << Qt::endl;
    return false;
  }

  qint64 duration = _mediaPlayer->duration();
  if (duration <= 0)
  {
    qDebug() << "Cannot get duration of file" << Qt::endl;
    return false;
  }

  // Make sure position is in [0,1].
  position = qBound(0.0, position, 1.0);

  // Seek at position in milliseconds.
  return seekTo((qint64)(position * duration));
}

bool VideoImpl::seekTo(qint64 positionMilliseconds)
{
  if (!_mediaPlayer || !_seekEnabled)
  {
    return false;
  }
  else
  {
    lockMutex();

    // Free the current frame and reset.
    _freeCurrentFrame();
    _bitsChanged = false;

    // Seek to position.
    _mediaPlayer->setPosition(positionMilliseconds);

    unlockMutex();

    return true;
  }
}

//bool VideoImpl::_preRun()
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

void VideoImpl::_checkMessages()
{
  // Qt Multimedia handles messages internally via signals/slots
  // This method is kept for interface compatibility but is no longer needed
}

void VideoImpl::_setMovieReady(bool ready)
{
  _movieReady = ready;
}

void VideoImpl::_setFinished(bool finished)
{
  Q_UNUSED(finished);
  //  qDebug() << "Clip " << (finished ? "finished" : "not finished");
}

void  VideoImpl::_updateRate()
{
  // Check different things.
  if (_mediaPlayer == nullptr)
  {
    qWarning() << "Cannot set rate: no media player!" << Qt::endl;
    return;
  }

  if (!_seekEnabled)
  {
    qWarning() << "Cannot set rate: seek not working" << Qt::endl;
    return;
  }

  if (!_isMovieReady())
  {
    qWarning() << "Movie is not yet ready to play, cannot set rate yet." << Qt::endl;
    return;
  }

  // Set the playback rate
  _mediaPlayer->setPlaybackRate(_rate);

  qDebug() << "Current rate: " << _rate << "." << Qt::endl;
}

void VideoImpl::_freeCurrentFrame() {
    if (_currentFrameData != nullptr)
    {
        delete _currentFrameData;
        _currentFrameData = nullptr;
    }
    _data = nullptr;
}

void VideoImpl::lockMutex()
{
    // Lock the underlying mutex directly. This is safe across threads.
    _mutex.lock();
}

void VideoImpl::unlockMutex()
{
    _mutex.unlock();
}

bool VideoImpl::waitForNextBits(int timeout, const uchar** bits)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeout)
    {
        // Bits available.
        if (hasBits() && bitsHaveChanged())
        {
            if (bits)
                *bits = getBits();
            return true;
        }
        // Avoid tight busy-loop that hogs the CPU while waiting.
        QThread::msleep(5);
        // or QCoreApplication::processEvents() in some contexts, but msleep is safer here.
    }

    // Timed out.
    return false;
}
}
