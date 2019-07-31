/*
 * VideoImplQtMultiMedia.h
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

#ifndef VIDEO_IMPL_QT_MULTI_MEDIA_H_
#define VIDEO_IMPL_QT_MULTI_MEDIA_H_

// Other includes.
#include "MM.h"
#include "VideoSurface.h"

#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QtOpenGL>
#include <QMutex>
#include <QWaitCondition>

#include <glib.h>
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace mmp {

/**
 * Private declaration of the video player.
 * This is to prevent the GStreamer header to be included in the whole project.
 * (it just needs to be included in this file).
 */
class VideoImplQtMultiMedia : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor.
   * This media player works for both video files and shared memory sockets.
   * If live is true, it's a shared memory socket.
   */
  VideoImplQtMultiMedia();
  virtual ~VideoImplQtMultiMedia();

  //  void setUri(const QString uri);
  /**
   * Returns whether or not GStreamer video support is ok.
   */
  static bool hasVideoSupport();

  /**
   * Sets up the player.
   * Basically calls loadMovie().
   */
  void build();

  /**
   * Returns the width of the video image.
   */
  int getWidth() const;

  /**
   * Returns the height of the video image.
   */
  int getHeight() const;

  /**
   * Returns the path to the media file being played.
   */
  QString getUri() const;

  /**
   * Returns the raw image of the last video frame.
   * It is currently unused!
   */
  const uchar* getBits();

  /// Returns true iff bits have started flowing (ie. if there is at least a first sample available).
  //  bool hasBits() const { return videoSurface->bits() != NULL; }
  bool hasBits() const { return _videoSurface->isActive(); }

  /// Returns true iff bits have changed since last call to getBits().
  //  bool bitsHaveChanged() const { return _bitsChanged; }
  bool bitsHaveChanged() const { return true; }

  /**
   * Performs regular updates (checks if movie is ready and checks messages).
   */
  void update();
  virtual bool isLive() { return false; }

  /**
   * Loads a new video stream
   *
   * Creates a new GStreamer pipeline, opens a movie, webcam or shmsrc socket,
   * depending on subclass.
   */
  virtual bool loadMovie(const QString& filename);

  bool setPlayState(bool play);
  bool getPlayState() const { return _playState; }

  bool seekIsEnabled() const { return _mediaPlayer->isSeekable(); }

  bool seekTo(double position);
  bool seekTo(qint64 positionMilliSeconds);

  void setRate(int rate=1.0);
  int getRate() const { return _rate; }

  void setVolume(int rate=0.0);
  int getVolume() const { return _volume; }

  void resetMovie();

protected:

public:
  /// Locks mutex (default = no effect).
  void lockMutex();

  /// Unlocks mutex (default = no effect).
  void unlockMutex();

  /// Wait until first data samples are available (blocking).
  bool waitForNextBits(int timeout, const uchar** bits=nullptr);

protected:
  int _width;
  int _height;

  bool _bitsChanged;

  /**
   * Contains meta informations about current file.
   */

  /// Raw image data of the last video frame.
  uchar *_data;

  /// Is seek enabled on the current pipeline?


  /// Playback rate (negative ==> reverse).
  int _rate;
  /// Audio playback volume (0.0 ==> 1.0).
  int _volume;


  /// Is the movie (or rather pipeline) ready to play.
  bool _movieReady;

  /// Is the movie playing (as opposed to paused).
  bool _playState;

  /// Main mutex.
  QMutex _mutex;

  /// Main mutex locker (for the lockMutex() / unlockMutex() methods).
  QMutexLocker* _mutexLocker;
private slots:
  void watchEndOfMedia(qint64 position);

private:
  /**
   * Path of the movie file being played.
   */
  QString _uri;

  QMediaPlayer *_mediaPlayer;
  QMediaPlaylist *_mediaPlaylist;

  VideoSurface* _videoSurface;

};

}

#endif /* ifndef */
