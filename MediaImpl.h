/*
 * MediaImpl.h
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

#ifndef VIDEO_IMPL_H_
#define VIDEO_IMPL_H_

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <QtGlobal>
#include <QtOpenGL>
#include <QMutex>
#include <QWaitCondition>
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <tr1/memory>

/**
 * Private declaration of the video player.
 * This is done this way so that GStreamer header don't need to be 
 * included in the whole project. (just in this file)
 */
class MediaImpl
{
public:
  /**
   * Constructor.
   * This media player works for both video files and shared memory sockets.
   * If live is true, it's a shared memory socket.
   */
  MediaImpl(const QString uri, bool live);
  ~MediaImpl();

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
   * When using the shared memory source, returns whether or not we
   * are attached to a shared memory socket.
   */
  bool getAttached();

  /**
   * Returns the raw image of the last video frame.
   * It is currently unused!
   */
  const uchar* getBits();

  /// Returns true iff bits have changed since last call to getBits().
  bool bitsHaveChanged() const { return _bitsChanged; }

  /**
   * Checks if the pipeline is ready.
   *
   * Returns whether or not the elements in the pipeline are connected,
   * and if we are using shmsrc, if the shared memory socket is being read.
   */
  bool isReady() const { return _movieReady && _padHandlerData.videoIsConnected; }

  bool videoIsConnected() const { return _padHandlerData.videoIsConnected; }

  /**
   * Performs regular updates (checks if movie is ready and checks messages).
   */
  void update();

  // void runAudio();

  /**
   * Loads a new movie file.
   * 
   * Creates a new GStreamer pipeline, opens a movie or a shmsrc socket.
   */
  bool loadMovie(QString filename);

  bool setPlayState(bool play);

  /**
   * Tells the MediaImpl that we are actually reading from a shmsrc.
   * Called from the GStreamer callback of the shmsrc.
   */
  void setAttached(bool attach);

  void setRate(double rate=1.0);
  double getRate() const { return _rate; }

  void resetMovie();

protected:
  void unloadMovie();
  void freeResources();

private:
  /**
   * Checks if we reached the end of the video file.
   *
   * Returns false if the pipeline is not ready yet.
   */
  bool _eos() const;

  // void _finish();
  // void _init();

//  bool _preRun();
  void _checkMessages();
  void _setMovieReady(bool ready);
  bool _isMovieReady() const { return _movieReady; }
  bool getPlayState() const { return _playState; };
  void _setFinished(bool finished);

  // Sends the appropriate seek events to adjust to rate.
  void _updateRate();

  void _freeCurrentSample();

public:
  // GStreamer callbacks.

  struct GstPadHandlerData {
    GstElement* videoToConnect;
    GstElement* videoSink;
    bool videoIsConnected;
    int width;
    int height;

    GstPadHandlerData() :
      videoToConnect(NULL), videoSink(NULL),
      videoIsConnected(false),
      width(-1), height(-1)
    {}
  };

  // GStreamer callback that simply sets the #newSample# flag to point to TRUE.
  static GstFlowReturn gstNewSampleCallback(GstElement*, MediaImpl *p);
  //static GstFlowReturn gstNewPreRollCallback (GstAppSink * appsink, gpointer user_data);

  // GStreamer callback that plugs the audio/video pads into the proper elements when they
  // are made available by the source.
  static void gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl::GstPadHandlerData* data);

  /// Locks mutex (default = no effect).
  void lockMutex();

  /// Unlocks mutex (default = no effect).
  void unlockMutex();

private:
  //locals

  // gstreamer elements
  GstBus *_bus;
  GstElement *_pipeline;
  GstElement *_uridecodebin0;
  GstElement *_shmsrc0;
  GstElement *_gdpdepay0;
  //GstElement *_audioQueue;
  //GstElement *_audioConvert;
  //GstElement *_audioResample;
  GstElement *_queue0;
  GstElement *_videoconvert0;
  //GstElement *_audioSink;
  GstElement *_appsink0;

  /**
   * Temporary contains the image data of the last frame.
   */
  GstSample  *_currentFrameSample;
  GstBuffer  *_currentFrameBuffer;
  GstMapInfo  _mapInfo;
  bool       _bitsChanged;

  /**
   * shmsrc socket poller.
   */
  GSource *_pollSource;

  GstPadHandlerData _padHandlerData;

  // unused
  int _width;
  // unused
  int _height;

  /// Raw image data of the last video frame.
  uchar *_data;

  /// Is seek enabled on the current pipeline?
  bool _seekEnabled;

  /// Playback rate (negative ==> reverse).
  double _rate;

  /// Whether or not we are reading video from a shmsrc.
  bool _isSharedMemorySource;

  /// Whether or not we are attached to a shmsrc, if using a shmsrc.
  bool _attached;

  // unused
  bool _terminate;

  /// Is the movie (or rather pipeline) ready to play.
  bool _movieReady;

  /// Is the movie playing (as opposed to paused).
  bool _playState;

  /// Main mutex.
  QMutex _mutex;

  /// Main mutex locker (for the lockMutex() / unlockMutex() methods).
  QMutexLocker* _mutexLocker;

private:
  /**
   * Path of the movie file being played.
   */
  QString _uri;

  static const int MAX_SAMPLES_IN_BUFFER_QUEUES = 30;
};

#endif /* ifndef */
