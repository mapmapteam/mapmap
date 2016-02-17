/*
 * MediaImpl.cpp
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
#include "MediaImpl.h"
#include <cstring>
#include <iostream>

// -------- private implementation of VideoImpl -------

bool MediaImpl::hasVideoSupport()
{
  static bool did_print_gst_version = false;
  if (! did_print_gst_version)
  {
    qDebug() << "Using GStreamer version " <<
      GST_VERSION_MAJOR << "." <<
      GST_VERSION_MINOR << "." <<
      GST_VERSION_MICRO;
    did_print_gst_version = true;
  }
  // TODO: actually check if we have it
  return true;
}

int MediaImpl::getWidth() const
{
  return _width;
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.width;
}

int MediaImpl::getHeight() const
{
  return _height;
//  Q_ASSERT(videoIsConnected());
//  return _padHandlerData.height;
}

const uchar* MediaImpl::getBits()
{
  // Reset bits changed.
  _bitsChanged = false;

  // Return data.
  return (hasBits() ? _data : NULL);
}

QString MediaImpl::getUri() const
{
  return _uri;
}

bool MediaImpl::getAttached()
{
  return _attached;
}

void MediaImpl::setAttached(bool attach)
{
  _attached = attach;
}


void MediaImpl::setRate(double rate)
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

    // Send seek events to activate rate.
    if (_seekEnabled)
      _updateRate();
  }
}

void MediaImpl::setVolume(double volume)
{
  // Only update volume if needed.
  if (_volume != volume)
  {
    _volume = volume;

    // Set volume element property.
    g_object_set (_audiovolume0, "volume", _volume, NULL);
  }
}

void MediaImpl::build()
{
  qDebug() << "Building video impl";
  if (!loadMovie(_uri))
  {
    qDebug() << "Cannot load movie " << _uri << ".";
  }
}

MediaImpl::~MediaImpl()
{
  // Free all resources.
  freeResources();

  // Free mutex locker object.
  delete _mutexLocker;
}

bool MediaImpl::_eos() const
{
  if (_movieReady)
  {
    Q_ASSERT( _appsink0 );
    if (_rate > 0)
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

GstFlowReturn MediaImpl::gstNewSampleCallback(GstElement*, MediaImpl *p)
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
  if (p->_isSharedMemorySource &&
      ( p->_width  == -1 ||
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

MediaImpl::MediaImpl(const QString uri, bool live) :
_bus(NULL),
_pipeline(NULL),
_uridecodebin0(NULL),
_queue0(NULL),
_videoconvert0(NULL),
_appsink0(NULL),
_audioqueue0(NULL),
_audioconvert0(NULL),
_audioresample0(NULL),
_audiovolume0(NULL),
_audiosink0(NULL),
_currentFrameSample(NULL),
_currentFrameBuffer(NULL),
_bitsChanged(false),
_width(-1),
_height(-1),
_duration(0),
//_isSeekable(false),
_data(NULL),
_seekEnabled(false),
_rate(1.0),
_isSharedMemorySource(live),
_attached(false),
_movieReady(false),
_playState(false),
_uri(uri)
{
  _pollSource = NULL;
  if (uri != "")
  {
    loadMovie(uri);
  }
  _mutexLocker = new QMutexLocker(&_mutex);
}

void MediaImpl::unloadMovie()
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

void MediaImpl::freeResources()
{
  // Free resources.
  if (_bus)
  {
    gst_object_unref (_bus);
    _bus = NULL;
  }

  if (_pipeline)
  {
    gst_element_set_state (_pipeline, GST_STATE_NULL);
    gst_object_unref (_pipeline);
    _pipeline = NULL;
  }

  // Reset pipeline elements.
  _uridecodebin0 = NULL;
  _queue0 = NULL;
  _videoconvert0 = NULL;
  _appsink0 = NULL;

  // Unref the shmsrc poller.
  if (_pollSource)
  {
     g_source_unref(_pollSource);
     _pollSource = NULL;
  }

  qDebug() << "Freeing remaining samples/buffers" << endl;

  // Frees current sample and buffer.
  _freeCurrentSample();

  // Reset other informations.
  _bitsChanged = false;
  _width = _height = (-1);
  _duration = 0;
  _videoIsConnected = false;
}

void MediaImpl::resetMovie()
{
  // XXX: There used to be an issue that when we reached EOS (_eos() == true) we could not seek anymore.
  if (_seekEnabled)
  {
    qDebug() << "Seeking at position 0.";
    GstEvent* seek_event;

    if (_rate > 0) {
      seek_event = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
          GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, 0);
    } else {
      seek_event = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
          GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_END, 0);
    }
    /* Send the event */
    gst_element_send_event (_appsink0, seek_event);
//    gst_element_seek_simple (_pipeline, GST_FORMAT_TIME,
//                             (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);
//    this->_currentFrameSample = NULL;
    _setMovieReady(true);
  }
  else
  {
    // Just reload movie.
    qDebug() << "Reloading the movie" << _seekEnabled;
    loadMovie(_uri);
  }
}

gboolean 
gstPollShmsrc (void *user_data)
{
  MediaImpl *p = (MediaImpl*) user_data;
  if (g_file_test(p->getUri().toUtf8().constData(), G_FILE_TEST_EXISTS) &&
    ! p->getAttached())
  {
    if (! p->setPlayState(true))
    {
      qDebug() << "tried to attach, but starting pipeline failed!" << endl;
      return false;
    }
    //qDebug() << "attached, started pipeline!" << endl;
    p->setAttached(true);
  }
  return true;
}

bool MediaImpl::loadMovie(const QString& filename)
{
  // Verify if file exists.
  const gchar* filetestpath = (const gchar*) filename.toUtf8().constData();
  if (FALSE == g_file_test(filetestpath, G_FILE_TEST_EXISTS))
  {
      std::cout << "File " << filetestpath << " does not exist" << std::endl;
      return false;
  }
  qDebug() << "Opening movie: " << filename << ".";

  // Assign URI.
  _uri = filename;

  // Free previously allocated structures
  unloadMovie();

  // Initialize GStreamer.
  GstElement *capsfilter0 = NULL;
  GstElement *videoscale0 = NULL;

  // Create the elements.
  if (_isSharedMemorySource)
  {
    _shmsrc0 = gst_element_factory_make ("shmsrc", "shmsrc0");
    _gdpdepay0 = gst_element_factory_make ("gdpdepay", "gdpdepay0");
    _pollSource = g_timeout_source_new (500);
    g_source_set_callback (_pollSource, 
        gstPollShmsrc, 
        this, 
        NULL);
    g_source_attach (_pollSource, g_main_context_default());
    g_source_unref (_pollSource);
  }
  else {
    _uridecodebin0 = gst_element_factory_make ("uridecodebin", "uridecodebin0");
  }
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
  
  if (_isSharedMemorySource)
  {
    if (! _shmsrc0 || ! _gdpdepay0)
    {
      g_printerr ("Not all elements could be created.\n");
      if (! _shmsrc0) g_printerr("_shmsrc0");
      if (! _gdpdepay0) g_printerr("_gdpdepay0");
      unloadMovie();
      return -1;
    }
  }
  else
  {
    if (! _uridecodebin0)
    {
      g_printerr ("Not all elements could be created.\n");
      if (! _uridecodebin0) g_printerr("_uridecodebin0");
      unloadMovie();
      return -1;
    }
  }

  // Build the pipeline. Note that we are NOT linking the source at this
  // point. We will do it later.
  gst_bin_add_many (GST_BIN (_pipeline),
    _isSharedMemorySource ? _shmsrc0 : _uridecodebin0, _queue0,
    _videoconvert0, videoscale0, capsfilter0, _appsink0,
//    _audioqueue0, _audioconvert0, _audioresample0, _audiovolume0, _audiosink0,
    NULL);

  // special case for shmsrc
  if (_isSharedMemorySource)
  {
    gst_bin_add (GST_BIN(_pipeline), _gdpdepay0);
    if (! gst_element_link_many (_shmsrc0, _gdpdepay0, _queue0, NULL))
    {
      g_printerr ("Could not link shmsrc, deserializer and video queue.\n");
    }
  }
  // link uridecodebin -> queue will be performed by callback

  if (! gst_element_link_many (_queue0, _videoconvert0, capsfilter0, videoscale0, _appsink0, NULL))
  {
    g_printerr ("Could not link video queue, colorspace converter, caps filter, scaler and app sink.\n");
    unloadMovie();
    return false;
  }

//  if (! gst_element_link_many (_audioqueue0, _audioconvert0, _audioresample0,
//        _audiovolume0, _audiosink0, NULL))
//  {
//    g_printerr ("Could not link audio queue, converter, resampler and audio sink.\n");
//    unloadMovie();
//    return false;
//  }

  // Process URI.
  QByteArray ba = filename.toLocal8Bit();
  gchar *filename_tmp = g_strdup((gchar*) filename.toUtf8().constData());
  gchar* uri = NULL; //  (gchar*) filename.toUtf8().constData();
  if (! _isSharedMemorySource && ! gst_uri_is_valid(uri))
  {
    // Try to convert filename to URI.
    GError* error = NULL;
    qDebug() << "Calling gst_filename_to_uri : " << uri;
    uri = gst_filename_to_uri(filename_tmp, &error);
    if (error)
    {
      qDebug() << "Filename to URI error: " << error->message;
      g_error_free(error);
      gst_object_unref (uri);
      freeResources();
      return false;
    }
  }
  g_free(filename_tmp);

  if (_isSharedMemorySource)
  {
    uri =  (gchar*) ba.data();
  }

  // Set URI to be played.
  qDebug() << "URI for uridecodebin: " << uri;
  // FIXME: sometimes it's just the path to the directory that is given, not the file itself.

  // Connect to the pad-added signal
  if (! _isSharedMemorySource)
  {
    // Extract meta info.
    GError* error = NULL;
    GstDiscoverer* discoverer = gst_discoverer_new(5*GST_SECOND, &error);
    if (!discoverer)
    {
      std::cout << "Error creating discoverer: " << error->message << std::endl;
      g_clear_error (&error);
      return false;
    }

    GstDiscovererInfo* info = gst_discoverer_discover_uri(discoverer, uri, &error);

    if (!info)
    {
      std::cout << "Error getting discoverer info: " << error->message << std::endl;
      g_clear_error (&error);
      return false;
    }

    GstDiscovererResult result = gst_discoverer_info_get_result(info);

    switch (result) {
      case GST_DISCOVERER_URI_INVALID:
        std::cout<< "Invalid URI '" << uri << "'" << std::endl;
        break;
      case GST_DISCOVERER_ERROR:
        std::cout<< "Discoverer error: " << error->message << std::endl;
        break;
      case GST_DISCOVERER_TIMEOUT:
        std::cout << "Timeout" << std::endl;
        break;
      case GST_DISCOVERER_BUSY:
        std::cout << "Busy" << std::endl;
        break;
      case GST_DISCOVERER_MISSING_PLUGINS:{
        const GstStructure *s;
        gchar *str;

        s = gst_discoverer_info_get_misc (info);
        str = gst_structure_to_string (s);

        std::cout << "Missing plugins: " << str << std::endl;
        g_free (str);
        break;
      }
      case GST_DISCOVERER_OK:
        std::cout << "Discovered '" << uri << "'" << std::endl;
        break;
    }

    g_clear_error (&error);

    if (result != GST_DISCOVERER_OK) {
      std::cout << "This URI cannot be played" << std::endl;
      return false;
    }

    // Gather info from video.
    GList *videoStreams = gst_discoverer_info_get_video_streams (info);
    if (!videoStreams)
    {
      std::cout << "This URI does not contain any video streams" << std::endl;
      return false;
    }

    // Retrieve meta-info.
    _width = gst_discoverer_video_info_get_width((GstDiscovererVideoInfo*)videoStreams->data);
    _height = gst_discoverer_video_info_get_height((GstDiscovererVideoInfo*)videoStreams->data);
//    _isSeekable = gst_discoverer_info_get_seekable(info);
    _duration = gst_discoverer_info_get_duration(info);

    // Free everything.
    g_object_unref(discoverer);
    gst_discoverer_info_unref(info);
    gst_discoverer_stream_info_list_free(videoStreams);

    // Connect pad signal.
    g_signal_connect (_uridecodebin0, "pad-added", G_CALLBACK (MediaImpl::gstPadAddedCallback), this);

    // Set uri of decoder.
    g_object_set (_uridecodebin0, "uri", uri, NULL);
  }
  else
  {
    //qDebug() << "LIVE mode" << uri;
    g_object_set (_shmsrc0, "socket-path", uri, NULL);
    g_object_set (_shmsrc0, "is-live", TRUE, NULL);
    _videoIsConnected = true;
  }
  g_free(uri);

  // Configure audio appsink.
  // TODO: change from mono to stereo
//  gchar* audioCapsText = g_strdup_printf ("audio/x-raw-float,channels=1,rate=%d,signed=(boolean)true,width=%d,depth=%d,endianness=BYTE_ORDER",
//                                          Engine::signalInfo().sampleRate(), (int)(sizeof(Signal_T)*8), (int)(sizeof(Signal_T)*8) );
//  GstCaps* audioCaps = gst_caps_from_string (audioCapsText);
//  g_object_set (_audioSink, "emit-signals", TRUE,
//                            "caps", audioCaps,
////                            "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
////                            "drop", TRUE,         // ... other buffers are dropped
//                            NULL);
//  g_signal_connect (_audioSink, "new-buffer", G_CALLBACK (VideoImpl::gstNewAudioBufferCallback), &_newAudioBufferHandlerData);
//  gst_caps_unref (audioCaps);
//  g_free (audioCapsText);


  // Configure video appsink.
  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA");
  g_object_set (capsfilter0, "caps", videoCaps, NULL);
  g_object_set (_appsink0, "emit-signals", TRUE,
                            "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
                            "drop", TRUE,         // ... other buffers are dropped
                            "sync", TRUE,
                            NULL);
  g_signal_connect (_appsink0, "new-sample", G_CALLBACK (MediaImpl::gstNewSampleCallback), this);
  gst_caps_unref (videoCaps);

  g_object_set (_audiovolume0, "mute", false, NULL);
  g_object_set (_audiovolume0, "volume", 0.0, NULL);

  // Listen to the bus.
  _bus = gst_element_get_bus (_pipeline);

  // Start playing.
  if (! _isSharedMemorySource && ! setPlayState(true))
  {
    return false;
  }

  return true;
}

void MediaImpl::update()
{
  // Check for end-of-stream or terminate.
  if (_eos() || _terminate)
  {
    _setFinished(true);
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

bool MediaImpl::setPlayState(bool play)
{
  if (_pipeline == NULL)
  {
    return false;
  }

  GstStateChangeReturn ret = gst_element_set_state (_pipeline, (play ? GST_STATE_PLAYING : GST_STATE_PAUSED));
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    qDebug() << "Unable to set the pipeline to the playing state." << endl;
    unloadMovie();
    return false;
  }
  else
  {
    _playState = play;
    return true;
  }
}

bool MediaImpl::seekTo(double position)
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
  return seekTo((gint64)(position*duration));
}

bool MediaImpl::seekTo(gint64 positionNanoSeconds)
{
  if (!_appsink0)
    return false;
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

//bool MediaImpl::_preRun()
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

void MediaImpl::_checkMessages()
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
        g_printerr("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr("Debugging information: %s\n",
            debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);

        if (!_isSharedMemorySource)
        {
          _terminate = true;
        }
        else
        {
          _attached = false;
          gst_element_set_state (_pipeline, GST_STATE_PAUSED);
          gst_element_set_state (_pipeline, GST_STATE_NULL);
          gst_element_set_state (_pipeline, GST_STATE_READY);
        }
//        _finish();
        break;

      // End-of-stream ////////////////////////////////////////
      case GST_MESSAGE_EOS:
        // Automatically loop back.
        g_print("End-Of-Stream reached.\n");
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
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
                       GST_TIME_ARGS (start), GST_TIME_ARGS (end));

            }
            else
            {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          }
          else
          {
            g_printerr ("Seeking query failed.");
          }

          gst_query_unref (query);

          // Movie is ready!
          _setMovieReady(true);
        }

        break;

      case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline.
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (_pipeline))
        {
          GstState oldState, newState, pendingState;
          gst_message_parse_state_changed(msg, &oldState, &newState,
              &pendingState);
          g_print("Pipeline state for movie %s changed from %s to %s:\n",
              _uri.toUtf8().constData(),
              gst_element_state_get_name(oldState),
              gst_element_state_get_name(newState));
        }
        break;

      default:
        // We should not reach here.
        g_printerr("Unexpected message received.\n");
        break;
      }
      gst_message_unref(msg);
    }
  }
}

void MediaImpl::_setMovieReady(bool ready)
{
  _movieReady = ready;
}

void MediaImpl::_setFinished(bool finished)
{
  Q_UNUSED(finished);
  //  qDebug() << "Clip " << (finished ? "finished" : "not finished");
}

void  MediaImpl::_updateRate()
{
  if (_pipeline == NULL)
  {
    qDebug() << "Cannot set rate: no pipeline!" << endl;
    return;
  }

  if (!_seekEnabled)
  {
    qDebug() << "Cannot set rate: seek not working" << endl;
    return;
  }

  if (!_isMovieReady())
  {
    qDebug() << "Movie is not yet ready to play, cannot seek yet." << endl;
  }

  gint64 position;
  GstEvent *seekEvent;

  /* Obtain the current position, needed for the seek event */
  if (!gst_element_query_position (_pipeline, GST_FORMAT_TIME, &position)) {
    g_printerr ("Unable to retrieve current position.\n");
    return;
  }

  /* Create the seek event */
  if (_rate > 0) {
    seekEvent = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
        GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, 0);
  } else {
    seekEvent = gst_event_new_seek (_rate, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE ),
        GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
  }

  if (_appsink0 == NULL) {
    /* If we have not done so, obtain the sink through which we will send the seek events */
    g_object_get (_pipeline, "video-sink", &_appsink0, NULL);
  }

  /* Send the event */
  gst_element_send_event (_appsink0, seekEvent);

  g_print ("Current rate: %g\n", _rate);
}

void MediaImpl::_freeCurrentSample() {
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

/**
 * FIXME: remove GOTO
 */
void MediaImpl::gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl* p)
{
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (newPad), GST_ELEMENT_NAME (src));
  GstPad *sinkPad = NULL;

  // Check the new pad's type.
  GstCaps *newPadCaps = gst_pad_query_caps (newPad, NULL);
  GstStructure *newPadStruct = gst_caps_get_structure (newPadCaps, 0);
  const gchar *newPadType   = gst_structure_get_name (newPadStruct);
  gchar *newPadStructStr = gst_structure_to_string(newPadStruct);
  g_print("Structure is %s\n", newPadStructStr);
  g_free(newPadStructStr);
  if (g_str_has_prefix (newPadType, "video/x-raw"))
  {
    sinkPad = gst_element_get_static_pad (p->_queue0, "sink");
    gst_structure_get_int(newPadStruct, "width",  &p->_width);
    gst_structure_get_int(newPadStruct, "height", &p->_height);
  }
  else if (g_str_has_prefix (newPadType, "audio/x-raw"))
  {
    sinkPad = gst_element_get_static_pad (p->_audioqueue0, "sink");
  }
  else {
    g_print ("  It has type '%s' which is not raw audio/video. Ignoring.\n", newPadType);
    goto exit;
  }

  // If our converter is already linked, we have nothing to do here.
  if (gst_pad_is_linked (sinkPad))
  {
    // Best prefixes.
    if (g_str_has_prefix (newPadType, "audio/x-raw-float") ||
        g_str_has_prefix (newPadType, "video/x-raw-int") )
    {
      g_print ("  Found a better pad.\n");
      GstPad* oldPad = gst_pad_get_peer(sinkPad);
      gst_pad_unlink(oldPad, sinkPad);
      g_object_unref(oldPad);
    }
    else
    {
      g_print ("  We are already linked. Ignoring.\n");
      goto exit;
    }
  }

  // Attempt the link
  if (GST_PAD_LINK_FAILED (gst_pad_link (newPad, sinkPad)))
  {
    g_print ("  Type is '%s' but link failed.\n", newPadType);
    goto exit;
  } else {
    p->_videoIsConnected = true;
    g_print ("  Link succeeded (type '%s').\n", newPadType);
  }

exit:
  // Unreference the new pad's caps, if we got them.
  if (newPadCaps != NULL)
  {
    gst_caps_unref (newPadCaps);
  }

  // Unreference the sink pad.
  if (sinkPad != NULL)
  {
    gst_object_unref (sinkPad);
  }
}

void MediaImpl::lockMutex()
{
  _mutexLocker->relock();
}

void MediaImpl::unlockMutex()
{
  _mutexLocker->unlock();
}

bool MediaImpl::waitForNextBits(int timeout, const uchar** bits)
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


