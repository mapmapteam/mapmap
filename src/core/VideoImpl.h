/*
 * VideoImpl.h
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

#ifndef VIDEO_IMPL_H_
#define VIDEO_IMPL_H_

#include "MM.h"
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

namespace mmp {

/**
 * Abstract base class for video playback implementations.
 * Subclasses supply frames via Qt 6 Multimedia (QMediaPlayer, QCamera).
 * The _data pointer always points into _currentFrame.bits() so that
 * callers can pass it directly to glTexImage2D.
 */
class VideoImpl
{
public:
  VideoImpl();
  virtual ~VideoImpl();

  /// Returns whether Qt Multimedia video support is available.
  static bool hasVideoSupport();

  /// Sets up the player (calls loadMovie with the stored URI).
  void build();

  virtual int getWidth() const;
  virtual int getHeight() const;

  QString getUri() const;

  /// Returns raw RGBA bytes of the latest video frame (or NULL if none yet).
  virtual const uchar* getBits();

  /// Returns true once at least one frame has been received.
  virtual bool hasBits() const { return (_data != nullptr); }

  /// Returns true if a new frame arrived since the last getBits() call.
  virtual bool bitsHaveChanged() const { return _bitsChanged; }

  /// Returns true when the pipeline is ready to deliver frames.
  bool isReady() const { return _movieReady && _videoIsConnected; }

  bool videoIsConnected() const { return _videoIsConnected; }
  void videoConnect() { _videoIsConnected = true; }

  bool audioIsConnected() const { return _audioIsConnected; }
  void audioConnect() { _audioIsConnected = true; }

  /// Performs regular updates (handles loop restart on end-of-stream).
  virtual void update();

  /// True for live sources (camera, etc.) that cannot be sought.
  virtual bool isLive() = 0;

  /// Loads a new media source. Subclasses override to set up their player.
  virtual bool loadMovie(const QString& filename);

  bool setPlayState(bool play);
  bool getPlayState() const { return _playState; }

  bool seekIsEnabled() const { return _seekEnabled; }

  /// Seek to a fractional position in [0, 1].
  bool seekTo(double position);
  /// Seek to an absolute position in milliseconds.
  virtual bool seekTo(qint64 positionMs) = 0;

  virtual void setRate(double rate = 1.0);
  double getRate() const { return _rate; }

  virtual void setVolume(double volume = 0.0);
  double getVolume() const { return _volume; }

  void resetMovie();

  /// Unloads the current movie and releases the capture device / player
  /// (reversible via build(), which re-opens the stored URI/device).
  void unloadMovie();

  /// Locks mutex.
  void lockMutex();
  /// Unlocks mutex.
  void unlockMutex();

  /// Blocks until new bits are available (up to timeout ms). Returns false on timeout.
  bool waitForNextBits(int timeout, const uchar** bits = nullptr);

protected:
  virtual void freeResources();

  void _setMovieReady(bool ready) { _movieReady = ready; }

  // Latest decoded frame — always Format_RGBA8888.
  QImage _currentFrame;
  /// Raw pointer into _currentFrame.bits(); kept for ABI with getBits().
  uchar *_data;
  bool   _bitsChanged;

  int _width;
  int _height;

  /// Duration in milliseconds (0 if unknown).
  qint64 _duration;

  bool _videoIsConnected;
  bool _audioIsConnected;
  bool _seekEnabled;

  double _rate;
  double _volume;

  bool _terminate;
  bool _movieReady;
  bool _playState;

  QMutex _mutex;

  bool _playInLoop;

private:
  QString _uri;
};

}

#endif /* VIDEO_IMPL_H_ */
