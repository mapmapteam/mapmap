/*
 * VideoImpl.cpp
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
#include "VideoImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

// -------- private implementation of VideoImpl -------

bool VideoImpl::hasVideoSupport()
{
  static bool did_print_gst_version = false;
  if (! did_print_gst_version)
  {
    qDebug() << "Using GStreamer version " <<
      GST_VERSION_MAJOR << "." << GST_VERSION_MINOR << "." << GST_VERSION_MICRO << endl;
    did_print_gst_version = true;
  }
  // TODO: actually check if we have it
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
    qDebug() << "Cannot set rate to zero, ignoring rate " << rate << endl;
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

    // Set volume element property
    if (audioIsSupported())
    {
      g_object_set (_audiovolume0, "mute", (_volume <= 0), NULL);
      g_object_set (_audiovolume0, "volume", _volume, NULL);
    }
    else
      qWarning() << "Cannot change volume cause this video does not support audio." << endl;
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

  // Free mutex locker object.
  delete _mutexLocker;
}

bool VideoImpl::_eos() const
{
  if (_movieReady)
  {
    Q_ASSERT( _appsink0 );
    if (_rate > 0.0)
    {
      gboolean videoEos;
      g_object_get (G_OBJECT (_appsink0), "eos", &videoEos, NULL);
      return (bool) (videoEos);
    }
    else
    {
      /* Obtain the current position, needed for the seek event */
      gint64 position;
      if (!gst_element_query_position (_pipeline, GST_FORMAT_TIME, &position)) {
        g_printerr ("Unable to retrieve current position.\n");
        return false;
      }
      return (position == 0);
    }
  }
  else
    return false;
}

GstFlowReturn VideoImpl::gstNewSampleCallback(GstElement*, VideoImpl *p)
{
  // Make it thread-safe.
  p->lockMutex();

  // Get next frame.
  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(p->_appsink0));

  // Unref last frame.
  p->_freeCurrentSample();

  // Set current frame.
  p->_currentFrameSample = sample;

  // For live sources, video dimensions have not been set, because
  // gstPadAddedCallback is never called. Fix dimensions from first sample /
  // caps we receive.
  if (( p->_width  == -1 ||
        p->_height == -1)) {
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure;
    structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(structure, "width",  &p->_width);
    gst_structure_get_int(structure, "height", &p->_height);
  }

  // Try to retrieve data bits of frame.
  GstMapInfo& map = p->_mapInfo;
  GstBuffer *buffer = gst_sample_get_buffer( sample );
  if (gst_buffer_map(buffer, &map, GST_MAP_READ))
  {
    p->_currentFrameBuffer = buffer;
    // For debugging:
    //gst_util_dump_mem(map.data, map.size)

    // Retrieve data from map info.
    p->_data = map.data;

    // Bits have changed.
    p->_bitsChanged = true;
  }

  p->unlockMutex();

  return GST_FLOW_OK;
}

VideoImpl::VideoImpl() :
_width(-1),
_height(-1),
_duration(0),
_seekEnabled(false),
_pipeline(NULL),
_queue0(NULL),
_capsfilter0(NULL),
_videoscale0(NULL),
_videoconvert0(NULL),
_appsink0(NULL),
_audioqueue0(NULL),
_audioconvert0(NULL),
_audioresample0(NULL),
_audiovolume0(NULL),
_audiosink0(NULL),
_bus(NULL),
_currentFrameSample(NULL),
_currentFrameBuffer(NULL),
_bitsChanged(false),
_data(NULL),
//_isSeekable(false),
_rate(1.0),
_movieReady(false),
_playState(false),
_uri("")
{
  _mutexLocker = new QMutexLocker(&_mutex);

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
  // Free resources.
  if (_bus)
  {
    gst_object_unref (GST_OBJECT(_bus));
    _bus = NULL;
  }

  if (_pipeline)
  {
    gst_element_set_state (_pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT(_pipeline));
    _pipeline = NULL;
  }

  // Free all components.
  _freeElement(&_queue0);
  _freeElement(&_capsfilter0);
  _freeElement(&_videoscale0);
  _freeElement(&_videoconvert0);
  _freeElement(&_appsink0);

  _freeElement(&_audioqueue0);
  _freeElement(&_audioconvert0);
  _freeElement(&_audioresample0);
  _freeElement(&_audiovolume0);
  _freeElement(&_audiosink0);

  qDebug() << "Freeing remaining samples/buffers" << endl;

  // Frees current sample and buffer.
  _freeCurrentSample();

  // Reset other informations.
  _bitsChanged = false;
  _width = _height = (-1);
  _duration = 0;
  _videoIsConnected = false;
  _audioIsConnected = false;
}

void VideoImpl::resetMovie()
{
  if (_seekEnabled)
  {
    if (_rate > 0.0)
    {
      seekTo((guint64) 0);
      qWarning() << "update Rate" << endl;
      _updateRate();
    }
    else
    {
      // NOTE: Untested.
      seekTo(_duration);
      qWarning() << "update Rate" << endl;
      _updateRate();
    }
  }
  else
  {
    qDebug() << "Seeking not enabled: reloading the movie" << endl;
    loadMovie(_uri);
  }
}

bool VideoImpl::createVideoComponents()
{
  // Already supported?
  if (videoIsSupported())
    return true;

  // Create the video elements.
  _queue0 = gst_element_factory_make ("queue", "queue0");
  _videoconvert0 = gst_element_factory_make ("videoconvert", "videoconvert0");
  _videoscale0 = gst_element_factory_make ("videoscale", "videoscale0");
  _capsfilter0 = gst_element_factory_make ("capsfilter", "capsfilter0");
  _appsink0 = gst_element_factory_make ("appsink", "appsink0");

  // Verify that they were created.
  if (!_queue0 || !_videoconvert0 || ! _videoscale0 || ! _capsfilter0 || !_appsink0)
  {
    qWarning() << "Not all video elements could be created." << endl;
    if (! _pipeline) g_printerr("_pipeline");
    if (! _queue0) g_printerr("_queue0");
    if (! _videoconvert0) g_printerr("_videoconvert0");
    if (! _videoscale0) g_printerr("videoscale0");
    if (! _capsfilter0) g_printerr("capsfilter0");
    if (! _appsink0) g_printerr("_appsink0");
    return false;
  }

  // Add them to pipeline.
  gst_bin_add_many (GST_BIN (_pipeline),
                    _queue0, _videoconvert0, _videoscale0, _capsfilter0, _appsink0,
                    NULL);

  // Link.
  if (! gst_element_link_many (_queue0, _videoconvert0, _capsfilter0, _videoscale0, _appsink0, NULL))
  {
    qWarning() << "Could not link video queue, colorspace converter, caps filter, scaler and app sink." << endl;
    return false;
  }

  // Configure video appsink.
  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA");
  g_object_set (_capsfilter0, "caps", videoCaps, NULL);

  g_object_set (_appsink0, "emit-signals", TRUE,
                           "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
                           "drop", TRUE,         // ... other buffers are dropped
                           "sync", TRUE,
                           NULL);

  g_signal_connect (_appsink0, "new-sample", G_CALLBACK (VideoImpl::gstNewSampleCallback), this);
  gst_caps_unref (videoCaps);

  return true;
}

bool VideoImpl::createAudioComponents()
{
  // Already supported?
  if (audioIsSupported())
    return true;

  // Create the audio elements.
  _audioqueue0 = gst_element_factory_make ("queue", "audioqueue0");
  _audioconvert0 = gst_element_factory_make ("audioconvert", "audioconvert0");
  _audioresample0 = gst_element_factory_make ("audioresample", "audioresample0");
  _audiovolume0 = gst_element_factory_make ("volume", "audiovolume0");
  _audiosink0 = gst_element_factory_make ("autoaudiosink", "audiosink0");

  // Verify that they were created.
  if (!_audioqueue0 || !_audioconvert0 || !_audioresample0 || !_audiovolume0 || !_audiosink0)
  {
    qDebug() << "Not all audio elements could be created." << endl;
    if (! _audioqueue0) g_printerr("_audioqueue0");
    if (! _audioconvert0) g_printerr("_audioconvert0");
    if (! _audioresample0) g_printerr("_audioresample0");
    if (! _audiovolume0) g_printerr("_audiovolume0");
    if (! _audiosink0) g_printerr("_audiosink0");
    return false;
  }

  // Add them to pipeline.
  gst_bin_add_many (GST_BIN (_pipeline),
                    _audioqueue0, _audioconvert0, _audioresample0, _audiovolume0, _audiosink0,
                    NULL);

  // Link.
  if (! gst_element_link_many (_audioqueue0, _audioconvert0, _audioresample0,
                               _audiovolume0, _audiosink0, NULL))
  {
    qDebug() << "Could not link audio queue, converter, resampler and audio sink." << endl;
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

  return true;
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

   // Prepare handler data.
   _videoIsConnected = false;
   _audioIsConnected = false;

   // Create the empty pipeline.
   _pipeline = gst_pipeline_new ( "video-source-pipeline" );
   if (!_pipeline)
   {
     qWarning() << "Pipeline could not be created." << endl;
     unloadMovie();
     return (-1);
   }

   // Create and link video components.
   if (!createVideoComponents())
   {
     qWarning() << "Video components could not be initialized." << endl;
     unloadMovie();
     return (-1);
   }

   //setVolume(0);

   // Listen to the bus.
   _bus = gst_element_get_bus (_pipeline);

   // Start playing.

   return true;
 }

bool VideoImpl::setPlayState(bool play)
{
  if (_pipeline == NULL)
  {
    return false;
  }

  // Change state.
  GstStateChangeReturn ret = gst_element_set_state (_pipeline, (play ? GST_STATE_PLAYING : GST_STATE_PAUSED));

//  // Wait until its done.
//  GstStateChangeReturn ret = gst_element_get_state (_pipeline, NULL, NULL, -1);
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    qDebug() << "Unable to set the pipeline to the playing state." << endl;
    //unloadMovie(); // <-- calling this created an infinite recursion
    return false;
  }
  else
  {
    _playState = play;
    return true;
  }
}

bool VideoImpl::seekTo(double position)
{
  gint64 duration;
  if (!gst_element_query_duration (_pipeline, GST_FORMAT_TIME, &duration))
  {
    qDebug() << "Cannot get duration of file" << endl;
    return false;
  }

  // Make sure position is in [0,1].
  position = qBound(0.0, position, 1.0);

  // Seek at position in nanoseconds.
  return seekTo((guint64)(position*duration));
}

bool VideoImpl::seekTo(guint64 positionNanoSeconds)
{
  if (!_appsink0 || !_seekEnabled)
  {
    return false;
  }
  else
  {
    lockMutex();

    // Free the current sample and reset.
    _freeCurrentSample();
    _bitsChanged = false;

    // Seek to position.
    bool result = gst_element_seek_simple(
                    _appsink0, GST_FORMAT_TIME,
                    GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
                    positionNanoSeconds);

    unlockMutex();

    return result;
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
  if (_bus != NULL)
  {
    // Get message.
    GstMessage *msg = gst_bus_timed_pop_filtered(
                        _bus, 0,
                        (GstMessageType) (GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_ASYNC_DONE));

    if (msg != NULL)
    {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg))
      {
        // Error ////////////////////////////////////////////////
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          qWarning() << "Error received from element " << GST_OBJECT_NAME (msg->src) << ": " << err->message << endl;
          qDebug() << "Debugging information: " << (debug_info ? debug_info : "none") << "." << endl;
          g_clear_error(&err);
          g_free(debug_info);

          if (!isLive())
          {
            _terminate = true;
          }
          else
          {
            gst_element_set_state (_pipeline, GST_STATE_PAUSED);
            gst_element_set_state (_pipeline, GST_STATE_NULL);
            gst_element_set_state (_pipeline, GST_STATE_READY);
          }
          //        _finish();
          break;

          // End-of-stream ////////////////////////////////////////
        case GST_MESSAGE_EOS:
          // Automatically loop back.
          if (_playInLoop) // Check if repeat mode is on
            resetMovie();
          //        _terminate = true;
          //        _finish();
          break;

          // Pipeline has prerolled/ready to play ///////////////
        case GST_MESSAGE_ASYNC_DONE:
          if (!_isMovieReady())
        {
          // Check if seeking is allowed.
          gint64 start, end;
          GstQuery *query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (_pipeline, query))
          {
            gst_query_parse_seeking (query, NULL, (gboolean*)&_seekEnabled, &start, &end);
            if (_seekEnabled)
            {
#ifdef VIDEO_IMPL_VERBOSE
              qDebug() << "Seeking is ENABLED from " << start << " to " << end << "." << endl;
#endif
            }
            else
            {
              qDebug() << "Seeking is DISABLED for this stream." << endl;
            }
          }
          else
          {
            qWarning() << "Seeking query failed." << endl;
          }

          gst_query_unref (query);

          // Movie is ready!
#ifdef VIDEO_IMPL_VERBOSE
          qDebug() << "Preroll done: movie is ready." << endl;
#endif // ifdef
          _setMovieReady(true);
        }

        break;

      case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline.
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (_pipeline))
        {
          GstState oldState, newState, pendingState;
          gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
#ifdef VIDEO_IMPL_VERBOSE
          qDebug() << "Pipeline state for movie " << _uri
                   << " changed from " << gst_element_state_get_name(oldState)
                   << " to " << gst_element_state_get_name(newState) << endl;
#endif
        }
        break;

      default:
        // We should not reach here.
        qWarning() << "Unexpected message received." << endl;
        break;
      }
      gst_message_unref(msg);
    }
  }
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
  if (_pipeline == NULL)
  {
    qWarning() << "Cannot set rate: no pipeline!" << endl;
    return;
  }

  if (!_seekEnabled)
  {
    qWarning() << "Cannot set rate: seek not working" << endl;
    return;
  }

  if (!_isMovieReady())
  {
    qWarning() << "Movie is not yet ready to play, cannot seek yet." << endl;
  }

  // Obtain the current position, needed for the seek event.
  gint64 position;
  if (!gst_element_query_position (_pipeline, GST_FORMAT_TIME, &position)) {
    qWarning() << "Unable to retrieve current position." << endl;
    return;
  }

  // Create the seek event.
  GstEvent *seekEvent;
  if (_rate > 0.0) {
    // Rate is positive (playing the video in normal direction)
    // Set new rate as a first argument. Provide position 0 so that we go to 0:00
    seekEvent = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
        GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, 0); // Go to 0:00
  } else {
    // Rate is negative
    // Set new rate as a first arguemnt. Provide the position we were already at.
    seekEvent = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
        GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
  }

  // If we have not done so, obtain the sink through which we will send the seek events.
  if (_appsink0 == NULL) {
    g_object_get (_pipeline, "video-sink", &_appsink0, NULL);
  }

  // Send the event.
  if (!gst_element_send_event (_appsink0, seekEvent)) {
    qWarning() << "Cannot perform seek event" << endl;
  }

  qDebug() << "Current rate: " << _rate << "." << endl;
}

void VideoImpl::_freeCurrentSample() {
  if (_currentFrameBuffer != NULL)
  {
    gst_buffer_unmap(_currentFrameBuffer, &_mapInfo);
  }

  if (_currentFrameSample != NULL)
  {
    gst_sample_unref(_currentFrameSample);
  }

  _currentFrameSample = NULL;
  _currentFrameBuffer = NULL;
  _data = NULL;
}

void VideoImpl::_freeElement(GstElement** element)
{
  if (*element)
  {
    *element = NULL;
  }
}

void VideoImpl::lockMutex()
{
  _mutexLocker->relock();
}

void VideoImpl::unlockMutex()
{
  _mutexLocker->unlock();
}

bool VideoImpl::waitForNextBits(int timeout, const uchar** bits)
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
