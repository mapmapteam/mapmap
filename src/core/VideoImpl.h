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
#include "FrameStats.h"
#include <QImage>
#include <QVideoFrame>
#include <QMutex>
#include <QRecursiveMutex>
#include <atomic>
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
  /// Resets bitsHaveChanged(). If the latest frame is a YUV frame (see
  /// hasVideoFrame()) it is converted to RGB on the CPU first, which is slow:
  /// renderers should use takeVideoFrame() for those.
  virtual const uchar* getBits();

  /// Returns true once at least one frame has been received.
  virtual bool hasBits() const;

  /// Returns true if the latest frame is kept unconverted in a YUV format that
  /// YuvTexture can draw directly (see takeVideoFrame()).
  virtual bool hasVideoFrame() const;

  /// Returns the latest unconverted YUV frame (invalid if hasVideoFrame() is
  /// false) and resets bitsHaveChanged().
  virtual QVideoFrame takeVideoFrame();

  /// Returns a (shallow) copy of the latest frame, without resetting
  /// bitsHaveChanged(). Null image if no frame yet. May convert (slow).
  virtual QImage getFrameImage() const;

  /// Returns the QImage format of the latest frame (Format_Invalid if none).
  virtual QImage::Format getFrameFormat() const { return _currentFrame.format(); }

  /// Converts a video frame to a QImage ready for direct OpenGL upload (see
  /// Util::glUploadFormat()): Format_RGB32, Format_ARGB32, Format_RGBX8888 or
  /// Format_RGBA8888, straight alpha. Opaque frames are never copied or
  /// converted beyond what QVideoFrame::toImage() does.
  static QImage frameToImage(const QVideoFrame& frame);

  /// Sends YUV frames through frameToImage(), like before they were converted
  /// on the GPU, for every video. Gives the times of the old path, to compare.
  static void setForceCpuConversion(bool force);
  /// Returns true if YUV frames are always converted on the CPU.
  static bool forceCpuConversion() { return _forceCpuConversion; }

  /// Returns the timing of the frames of this video (see FrameStats).
  FrameStats* frameStats() { return &_frameStats; }

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

  virtual bool setPlayState(bool play);
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

protected:
  virtual void freeResources();

  void _setMovieReady(bool ready) { _movieReady = ready; }

  /// Stores a newly decoded frame: YUV frames that YuvTexture can draw are
  /// kept unconverted (copied to system memory, see copyToSystemMemory()),
  /// others are converted with frameToImage(). The heavy work is done before
  /// taking the mutex, so this can (and should) run on the decoder's thread.
  void _setFrame(const QVideoFrame& frame);

  /// Tells _frameStats how frames of this pixel format are handled.
  void _setFramePath(QVideoFrameFormat::PixelFormat format, bool onGpu);

public:
  /// Returns a copy of a frame in ordinary (cached) system memory. Hardware
  /// decoders (e.g. V4L2 on Raspberry Pi) hand out frames in uncached memory,
  /// which is several times slower to read: glTexSubImage2D straight from it
  /// costs ~23 ms per 1080p frame on a Pi 4, vs ~9 ms copy + ~5 ms upload.
  /// Copying on the decoder thread also returns its buffers right away.
  /// Returns an invalid frame on failure.
  static QVideoFrame copyToSystemMemory(const QVideoFrame& frame);

protected:

  // Latest decoded YUV frame, unconverted (invalid if the frame was converted
  // to _currentFrame instead).
  QVideoFrame _videoFrame;
  // Latest decoded frame, ready for upload (see frameToImage()). Null for
  // YUV frames until getBits() converts them.
  QImage _currentFrame;
  /// Raw pointer into _currentFrame.bits(); kept for ABI with getBits().
  uchar *_data;
  std::atomic<bool> _bitsChanged;

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

  // Recursive: renderers hold it across calls to the accessors, which lock it
  // too (frames are delivered on the decoder's thread).
  mutable QRecursiveMutex _mutex;

  bool _playInLoop;

  FrameStats _frameStats;
  // Path last given to _frameStats (see _setFramePath()).
  QVideoFrameFormat::PixelFormat _framePathFormat;
  bool _framePathOnGpu;

private:
  QString _uri;

  static std::atomic<bool> _forceCpuConversion;
};

}

#endif /* VIDEO_IMPL_H_ */
