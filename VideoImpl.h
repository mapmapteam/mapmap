/*
 * VideoImpl.h
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

// GStreamer includes.
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>

// Other includes.
#include "MM.h"
#include <QtOpenGL>
#include <QMutex>
#include <QWaitCondition>

#include <glib.h>
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

MM_BEGIN_NAMESPACE

/**
 * Private declaration of the video player.
 * This is to prevent the GStreamer header to be included in the whole project.
 * (it just needs to be included in this file).
 */
class VideoImpl
{
public:
  /**
   * Constructor.
   * This media player works for both video files and shared memory sockets.
   * If live is true, it's a shared memory socket.
   */
  VideoImpl();
  virtual ~VideoImpl();



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
  bool hasBits() const { return (_currentFrameSample != NULL); }

  /// Returns true iff bits have changed since last call to getBits().
  bool bitsHaveChanged() const { return _bitsChanged; }

  /**
   * Checks if the pipeline is ready.
   *
   * Returns whether or not the elements in the pipeline are connected,
   * and if we are using shmsrc, if the shared memory socket is being read.
   */
  bool isReady() const { return _isMovieReady() && videoIsConnected(); }

  bool videoIsConnected() const { return _videoIsConnected; }
  void videoConnect() { _videoIsConnected = true; }

  /**
   * Performs regular updates (checks if movie is ready and checks messages).
   */
  void update();
  virtual bool isLive() = 0;

  /**
   * Loads a new video stream
   * 
   * Creates a new GStreamer pipeline, opens a movie, webcam or shmsrc socket,
   * depending on subclass.
   */
  virtual bool loadMovie(const QString& filename) {
    // Verify if file exists.
    const gchar* filetestpath = (const gchar*) filename.toUtf8().constData();
    if (FALSE == g_file_test(filetestpath, G_FILE_TEST_EXISTS))
    {
      qDebug() << "File " << filename << " does not exist" << endl;
      return false;
    }

    qDebug() << "Opening movie: " << filename << ".";

    // Assign URI.
    _uri = filename;

    // Free previously allocated structures
    unloadMovie();

    // Initialize GStreamer.

    GstElement *videoscale0 = NULL;

    // Create the elements.
    _queue0 = gst_element_factory_make ("queue", "queue0");
    _videoconvert0 = gst_element_factory_make ("videoconvert", "videoconvert0");
    videoscale0 = gst_element_factory_make ("videoscale", "videoscale0");
    capsfilter0 = gst_element_factory_make ("capsfilter", "capsfilter0");
    _appsink0 = gst_element_factory_make ("appsink", "appsink0");

    // Prepare handler data.
    _videoIsConnected = false;

    _audioqueue0 = gst_element_factory_make ("queue", "audioqueue0");
    _audioconvert0 = gst_element_factory_make ("audioconvert", "audioconvert0");
    _audioresample0 = gst_element_factory_make ("audioresample", "audioresample0");
    _audiovolume0 = gst_element_factory_make ("volume", "audiovolume0");
    _audiosink0 = gst_element_factory_make ("autoaudiosink", "audiosink0");

    // Create the empty pipeline.
    _pipeline = gst_pipeline_new ( "video-source-pipeline" );
    if (!_pipeline ||
        !_queue0 || !_videoconvert0 || ! videoscale0 || ! capsfilter0 ||
        !_appsink0 || !_audioqueue0 || !_audioconvert0 || !_audioresample0 ||
        !_audiovolume0 || !_audiosink0)
    {
      g_printerr ("Not all elements could be created.\n");
      if (! _pipeline) g_printerr("_pipeline");
      if (! _queue0) g_printerr("_queue0");
      if (! _videoconvert0) g_printerr("_videoconvert0");
      if (! videoscale0) g_printerr("videoscale0");
      if (! capsfilter0) g_printerr("capsfilter0");
      if (! _appsink0) g_printerr("_appsink0");
      if (! _audioqueue0) g_printerr("_audioqueue0");
      if (! _audioconvert0) g_printerr("_audioconvert0");
      if (! _audioresample0) g_printerr("_audioresample0");
      if (! _audiovolume0) g_printerr("_audiovolume0");
      if (! _audiosink0) g_printerr("_audiosink0");
      unloadMovie();
      return -1;
    }

    // Build the pipeline. Note that we are NOT linking the source at this
    // point. We will do it later.
    gst_bin_add_many (GST_BIN (_pipeline),
        _queue0, _videoconvert0, videoscale0, capsfilter0, _appsink0,
        _audioqueue0, _audioconvert0, _audioresample0, _audiovolume0, _audiosink0,
        NULL);
    // special case for shmsrc
    // link uridecodebin -> queue will be performed by callback

    if (! gst_element_link_many (_queue0, _videoconvert0, capsfilter0, videoscale0, _appsink0, NULL))
    {
      qDebug() << "Could not link video queue, colorspace converter, caps filter, scaler and app sink." << endl;
      unloadMovie();
      return false;
    }

    if (! gst_element_link_many (_audioqueue0, _audioconvert0, _audioresample0,
                                 _audiovolume0, _audiosink0, NULL))
    {
      g_printerr ("Could not link audio queue, converter, resampler and audio sink.\n");
      unloadMovie();
      return false;
    }
    
    // Configure audio appsink.
    // TODO: change from mono to stereo
    //  gchar* audioCapsText = g_strdup_printf ("audio/x-raw-float,channels=1,rate=%d,signed=(boolean)true,width=%d,depth=%d,endianness=BYTE_ORDER",
    //                                          Engine::signalInfo().sampleRate(), (int)(sizeof(Signal_T)*8), (int)(sizeof(Signal_T)*8) );
    //GstCaps* audioCaps = gst_caps_from_string (audioCapsText);
    /*
    GstCaps* audioCaps = gst_caps_from_string ("audio/xraw-float");
    g_object_set (_audioSink, "emit-signals", TRUE,
                              "caps", audioCaps,
                              "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
                              "drop", TRUE,         // ... other buffers are dropped
                              "sync", TRUE,
                              NULL);
    g_signal_connect (_audioSink, "new-buffer", G_CALLBACK (VideoImpl::gstNewAudioBufferCallback), this);
    gst_caps_unref (audioCaps);
    */
    //  g_free (audioCapsText);


    // Configure video appsink.
    GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA");
    g_object_set (capsfilter0, "caps", videoCaps, NULL);

    g_object_set (_appsink0, "emit-signals", TRUE,
        "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
        "drop", TRUE,         // ... other buffers are dropped
        "sync", TRUE,
        NULL);

    g_signal_connect (_appsink0, "new-sample", G_CALLBACK (VideoImpl::gstNewSampleCallback), this);
    gst_caps_unref (videoCaps);

    setVolume(0);
    
    // Listen to the bus.
    _bus = gst_element_get_bus (_pipeline);

    // Start playing.

    return true;
  }
  //virtual GstElement *buildPipeline(Element sink) = 0;
  //virtual void loadMovie(const QString& filename) = 0;

  bool setPlayState(bool play);
  bool getPlayState() const { return _playState; }

  bool seekIsEnabled() const { return _seekEnabled; }

  bool seekTo(double position);
  bool seekTo(guint64 positionNanoSeconds);

  void setRate(double rate=1.0);
  double getRate() const { return _rate; }

  void setVolume(double rate=0.0);
  double getVolume() const { return _volume; }

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
  void _setFinished(bool finished);

  // Sends the appropriate seek events to adjust to rate.
  void _updateRate();

  void _freeCurrentSample();

public:
  // GStreamer callback that simply sets the #newSample# flag to point to TRUE.
  static GstFlowReturn gstNewSampleCallback(GstElement*, VideoImpl *p);
  //static GstFlowReturn gstNewPreRollCallback (GstAppSink * appsink, gpointer user_data);

  // GStreamer callback that plugs the audio/video pads into the proper elements when they
  // are made available by the source.
  //static void gstPadAddedCallback(GstElement *src, GstPad *newPad, VideoImpl* p);

  /// Locks mutex (default = no effect).
  void lockMutex();

  /// Unlocks mutex (default = no effect).
  void unlockMutex();

  /// Wait until first data samples are available (blocking).
  bool waitForNextBits(int timeout, const uchar** bits=0);

  // FIXME these should be private, accessed my subclasses
  int _width;
  int _height;
//  bool _isSeekable;
  guint64 _duration; // duration (in nanoseconds) (unused for now)
  bool _videoIsConnected;
  GstElement *_queue0;
  GstElement *_audioqueue0;
  GstElement *_pipeline;
  GstElement *capsfilter0;
  bool _seekEnabled;

private:
  //locals

  // gstreamer elements
  GstBus *_bus;

  GstElement *_videoconvert0;
  GstElement *_appsink0;

  GstElement *_audioconvert0;
  GstElement *_audioresample0;
  GstElement *_audiovolume0;
  GstElement *_audiosink0;

  /**
   * Temporary contains the image data of the last frame.
   */
  GstSample  *_currentFrameSample;
  GstBuffer  *_currentFrameBuffer;
  GstMapInfo  _mapInfo;
  bool       _bitsChanged;

  /**
   * Contains meta informations about current file.
   */




  /// Raw image data of the last video frame.
  uchar *_data;

  /// Is seek enabled on the current pipeline?


  /// Playback rate (negative ==> reverse).
  double _rate;
  /// Audio playback volume (0.0 ==> 1.0).
  double _volume;

  /// Whether or not we are reading video from a shmsrc.
  bool _isSharedMemorySource;



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

MM_END_NAMESPACE

#endif /* ifndef */
