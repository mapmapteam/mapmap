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
#include <gst/app/gstappsink.h>

// -------- private implementation of VideoImpl -------

bool MediaImpl::hasVideoSupport()
{
  static bool did_print_gst_version = false;
  if (! did_print_gst_version)
  {
    qDebug() << "Using GStreamer version " <<
      GST_VERSION_MAJOR << "." <<
      GST_VERSION_MINOR << "." <<
      GST_VERSION_MICRO << endl;
    did_print_gst_version = true;
  }
  // TODO: actually check if we have it
  return true;
}

int MediaImpl::getWidth() const
{
  return _width;
}

int MediaImpl::getHeight() const
{
  return _height;
}

const uchar* MediaImpl::getBits() const
{
  return _data;
}

void MediaImpl::build()
{
  qDebug() << "Building video impl" << endl;
  if (!loadMovie(_uri))
  {
    qDebug() << "Cannot load movie " << _currentMovie << "." << endl;
  }
}

MediaImpl::~MediaImpl()
{
  freeResources();
  if (_data)
    free(_data);
}

bool MediaImpl::_videoPull()
{
//  qDebug() << "video pull" << endl;

  GstSample *sample = NULL;
  GstStructure *structure = NULL;
  GstCaps* caps = NULL;
  GstBuffer *buffer = NULL;

  // Retrieve the sample
  sample = gst_app_sink_pull_sample(GST_APP_SINK(_videoSink));

  if (sample == NULL)
  {
    // Either means we are not playing or we have reached EOS.
    return false;
  }
  else
  {
    gst_sample_ref(sample);

    caps = gst_sample_get_caps(sample);
    structure = gst_caps_get_structure(caps, 0);
    buffer = gst_sample_get_buffer(sample);

    int width  = 640;
    int height = 480;
    int bpp    = 32;
    int depth  = 24;

    gst_structure_get_int(structure, "width",  &width);
    gst_structure_get_int(structure, "height", &height);
    gst_structure_get_int(structure, "bpp",    &bpp);
    gst_structure_get_int(structure, "depth",  &depth);

    _width = width;
    _height = height;
    int size = _width * _height;

    if (!_data)
      _data = (uchar*) calloc(size, sizeof(uchar*));

//    video->resize(width, height);

//        qDebug() << gst_structure_to_string(capsStruct) << endl;
//        qDebug() << width << "x" << height << "=" << width*height << "(" << width*height*4 << "," << width*height*3 << ")" << endl;
//        qDebug() << "bpp: " << bpp << " depth: " << depth << endl;
//        qDebug() << "Buffer size: " << GST_BUFFER_SIZE(buffer) << endl;

    GstMapInfo map; 
    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    { 
      // For debugging:
      //gst_util_dump_mem(map.data, map.size)
      if (bpp == 32)
        memcpy(_data, map.data, size * 4);
      else
        convert24to32(_data, map.data, size);
      gst_buffer_unmap(buffer, &map); 
    } 

    gst_sample_unref(sample);
    return true;
  }
}

bool MediaImpl::_eos() const
{
  if (_movieReady)
  {
    Q_ASSERT( _videoSink );
//    Q_ASSERT( _audioSink );
    gboolean videoEos;
//    gboolean audioEos;
    g_object_get (G_OBJECT (_videoSink), "eos", &videoEos, NULL);
//    g_object_get (G_OBJECT (_audioSink), "eos", &audioEos, NULL);
    return (bool) (videoEos /*|| audioEos*/);
  }
  else
    return false;
}

//void VideoImpl::_init()
//{
//  _audioHasNewBuffer = false;
//  _videoHasNewBuffer = false;
//
//  _terminate = false;
//  _seekEnabled = false;
//
//  _movieReady=true;
//
//  // Stop sleeping the video output.
//  _VIDEO_OUT->sleeping(false);
//  _AUDIO_OUT->sleeping(false);
//}


void MediaImpl::gstNewSampleCallback(GstElement*, int *newBufferCounter)
{
  (*newBufferCounter)++;
}

MediaImpl::MediaImpl(const QString uri) :
_currentMovie(""),
_bus(NULL),
_pipeline(NULL),
_source(NULL),
//_audioQueue(NULL),
//_audioConvert(NULL),
//_audioResample(NULL),
_videoQueue(NULL),
_videoConvert(NULL),
_videoColorSpace(NULL),
_audioSink(NULL),
_videoSink(NULL),
_width(640),
_height(480),
_data(NULL),
//_audioBufferAdapter(NULL),
_seekEnabled(false),
//_audioNewBufferCounter(0),
_videoNewBufferCounter(0),
_movieReady(false),
_uri(uri)
{
  if (uri != "")
    loadMovie(uri);

//  addPlug(_VIDEO_OUT = new PlugOut<VideoRGBAType>(this, "ImgOut", false));
//  addPlug(_AUDIO_OUT = new PlugOut<SignalType>(this, "AudioOut", false));
//
//  addPlug(_FINISH_OUT = new PlugOut<ValueType>(this, "FinishOut", false));
//
//  QList<AbstractPlug*> atLeastOneOfThem;
//  atLeastOneOfThem.push_back(_VIDEO_OUT);
//  atLeastOneOfThem.push_back(_AUDIO_OUT);
//  setPlugAtLeastOneNeeded(atLeastOneOfThem);
//
//  addPlug(_RESET_IN = new PlugIn<ValueType>(this, "Reset", false, new ValueType(0, 0, 1)));
//  addPlug(_MOVIE_IN = new PlugIn<StringType>(this, "Movie", false));
//
//  //_settings.add(Property::FILENAME, SETTING_FILENAME)->valueStr("");
//
//  _VIDEO_OUT->sleeping(true);
//  _AUDIO_OUT->sleeping(true);
//
//  // Crease audio buffer handler.
//  _audioBufferAdapter = gst_adapter_new();
}

void MediaImpl::unloadMovie()
{
  // Free allocated resources.
  freeResources();

  // Reset flags.
//  _audioNewBufferCounter = 0;
  _videoNewBufferCounter = 0;

  _terminate = false;
  _seekEnabled = false;

  _setReady(false);

  // Unsynch.
  // NOTE: I commented this out, it was in Drone, most probably useless but who knows.
  // unSynch(); // XXX: I'm not sure why we are doing this...
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

  _source = NULL;
//  _audioQueue = NULL;
//  _audioConvert = NULL;
//  _audioResample = NULL;
  _videoQueue = NULL;
  _videoConvert = NULL;
  _videoColorSpace = NULL;
  _audioSink = NULL;
  _videoSink = NULL;
  _padHandlerData = GstPadHandlerData();

  // Flush buffers in adapter.
//  gst_adapter_clear(_audioBufferAdapter);

}

void MediaImpl::resetMovie()
{
  // TODO: Check if we can still seek when we reach EOS. It seems like it's then impossible and we
  // have to reload but it seems weird so we should check.
  if (!_eos() && _seekEnabled)
  {
    qDebug() << "Seeking at position 0." << endl;
    gst_element_seek_simple (_pipeline, GST_FORMAT_TIME,
                             (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);
    _setReady(true);
  }
  else
  {
    // Just reload movie.
    qDebug() << "Reloading the movie" << _seekEnabled << endl;
    _currentMovie = "";
    loadMovie(_uri);
  }
}

bool MediaImpl::loadMovie(QString filename)
{
  _uri = filename;

  qDebug() << "Opening movie: " << filename << ".";

  // Free previously allocated structures
  unloadMovie();

  //_firstFrameTime=_formatContext->start_time;

  // Initialize GStreamer.
  gst_init (NULL, NULL);
  GstElement *capsFilter = NULL;
  GstElement *videoScale = NULL;

  // Create the elements.
  _source =          gst_element_factory_make ("uridecodebin", "source");

//  _audioQueue =      gst_element_factory_make ("queue", "aqueue");
//  _audioConvert =    gst_element_factory_make ("audioconvert", "aconvert");
//  _audioResample =   gst_element_factory_make ("audioresample", "aresample");
//  _audioSink =       gst_element_factory_make ("appsink", "asink");
//
  _videoQueue =      gst_element_factory_make ("queue", "vqueue");
  _videoColorSpace = gst_element_factory_make ("videoconvert", "vcolorspace");
  videoScale = gst_element_factory_make ("videoscale", "videoscale0");
  capsFilter = gst_element_factory_make ("capsfilter", "capsfilter0");
  _videoSink =       gst_element_factory_make ("appsink", "vsink");

  // Prepare handler data.
//  _padHandlerData.audioToConnect   = _audioQueue;
  _padHandlerData.videoToConnect   = _videoQueue;
  _padHandlerData.videoSink        = _videoSink;
  //_padHandlerData.audioIsConnected = false;
  _padHandlerData.videoIsConnected = false;

//  _newAudioBufferHandlerData.audioSink          = _audioSink;
//  _newAudioBufferHandlerData.audioBufferAdapter = _audioBufferAdapter;

  // Create the empty pipeline.
  _pipeline = gst_pipeline_new ( "video-source-pipeline" );

  if (!_pipeline || !_source ||
//      !_audioQueue || !_audioConvert || !_audioResample || !_audioSink ||
      !_videoQueue || !_videoColorSpace || ! videoScale || ! capsFilter || ! _videoSink)
  {
    g_printerr ("Not all elements could be created.\n");
    unloadMovie();
    return -1;
  }

  // Build the pipeline. Note that we are NOT linking the source at this
  // point. We will do it later.
  gst_bin_add_many (GST_BIN (_pipeline), _source,
//                    _audioQueue, _audioConvert, _audioResample, _audioSink,
                    _videoQueue, _videoColorSpace, videoScale, capsFilter, _videoSink, NULL);

//  if (!gst_element_link_many(_audioQueue, _audioConvert, _audioResample, _audioSink, NULL)) {
//    g_printerr ("Audio elements could not be linked.\n");
//    unloadMovie();
//    return false;
//  }

  if (!gst_element_link_many (_videoQueue, _videoColorSpace, capsFilter, videoScale, _videoSink, NULL)) {
    g_printerr ("Video elements could not be linked.\n");
    unloadMovie();
    return false;
  }

  // Process URI.
  gchar* uri = (gchar*) filename.toUtf8().constData();
  if (!gst_uri_is_valid(uri))
  {
    // Try to convert filename to URI.
    GError* error = NULL;
    uri = gst_filename_to_uri(uri, &error);
    if (error) {
      qDebug() << "Filename to URI error: " << error->message << endl;
      g_error_free(error);
      gst_object_unref (uri);
      freeResources();
      return false;
    }
  }

  // Set URI to be played.
  qDebug() << "URI for uridecodebin: " << uri << endl;
  g_object_set (_source, "uri", uri, NULL);

  // Connect to the pad-added signal
  g_signal_connect (_source, "pad-added", G_CALLBACK (MediaImpl::gstPadAddedCallback), &_padHandlerData);

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
//  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw-rgb");
  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA");
  g_object_set (capsFilter, "caps", videoCaps, NULL);
  g_object_set (_videoSink, "emit-signals", TRUE,
                            "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
                            "drop", TRUE,         // ... other buffers are dropped
                            NULL);
  g_signal_connect (_videoSink, "new-sample", G_CALLBACK (MediaImpl::gstNewSampleCallback), &_videoNewBufferCounter);
  gst_caps_unref (videoCaps);

  // Listen to the bus.
  _bus = gst_element_get_bus (_pipeline);

  // Start playing.
  if (!_setPlayState(true))
    return false;

  qDebug() << "Pipeline started." << endl;

  //_movieReady = true;
  return true;
}


bool MediaImpl::runVideo() {

//  if (!_VIDEO_OUT->connected())
//    return;

  if (!_preRun())
    return false;

  bool bitsChanged = false;

  if (_videoNewBufferCounter > 0) {

    // Pull video.
    if (!_videoPull())
    {
      _setFinished(true);
//      _FINISH_OUT->type()->setValue(1.0f);
//      _VIDEO_OUT->sleeping(true);
    }
    else
    {
      bitsChanged = true;
      //      _VIDEO_OUT->sleeping(false);
    }

    _videoNewBufferCounter--;
    //std::cout << "VideoImpl::runVideo: read frame #" << _videoNewBufferCounter << std::endl;
  }

  _postRun();

  return bitsChanged;
}


//void VideoImpl::runAudio() {
//
//  if (!_AUDIO_OUT->connected())
//    return;
//
//  if (!_preRun())
//    return;
//
//  unsigned int blockByteSize = Engine::signalInfo().blockSize()*sizeof(Signal_T);
//  if (gst_adapter_available(_audioBufferAdapter) >= blockByteSize )
//  {
//    // Copy block of data to audio output.
//    gst_adapter_copy(_audioBufferAdapter, (guint8*)_AUDIO_OUT->type()->data(), 0, blockByteSize);
//    gst_adapter_flush (_audioBufferAdapter, blockByteSize);
//
//    _AUDIO_OUT->sleeping(false);
//  }
//  else
//  {
//    _FINISH_OUT->type()->setValue(1.0f);
//    _AUDIO_OUT->sleeping(true);
//  }
//
//  _postRun();
//}

bool MediaImpl::_preRun()
{
  // Check for end-of-stream or terminate.
  if (_eos() || _terminate)
  {
    _setFinished(true);
    resetMovie();

//    _FINISH_OUT->type()->setValue(1.0f);
//    _VIDEO_OUT->sleeping(true);
//    _AUDIO_OUT->sleeping(true);
//
//    if (_audioBufferAdapter != NULL)
//      gst_adapter_clear(_audioBufferAdapter);
  }
  else
    _setFinished(false);
//    _FINISH_OUT->type()->setValue(0.0f);

//  if (_RESET_IN->type()->boolValue())
//    resetMovie();

  if (!_movieReady ||
      !_padHandlerData.isConnected())
    return false;

  return true;
}

void MediaImpl::_postRun()
{
  // Parse message.
  if (_bus != NULL)
  {
    GstMessage *msg = gst_bus_timed_pop_filtered(
                        _bus, 0,
                        (GstMessageType) (GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {

      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr("Debugging information: %s\n",
            debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);

        _terminate = true;
//        _finish();
        break;

      case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
//        _terminate = true;
//        _finish();
        break;

      case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline.
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (_pipeline)) {
          GstState oldState, newState, pendingState;
          gst_message_parse_state_changed(msg, &oldState, &newState,
              &pendingState);
          g_print("Pipeline state for movie %s changed from %s to %s:\n",
              _currentMovie.toUtf8().constData(),
              gst_element_state_get_name(oldState),
              gst_element_state_get_name(newState));

//          if (oldState == GST_STATE_PAUSED && newState == GST_STATE_READY)
//            gst_adapter_clear(_audioBufferAdapter);

          if (newState == GST_STATE_PLAYING) {
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
          }
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


bool MediaImpl::_setPlayState(bool play)
{
  if (_pipeline == NULL)
    return false;

  GstStateChangeReturn ret = gst_element_set_state (_pipeline, (play ? GST_STATE_PLAYING : GST_STATE_PAUSED));
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    qDebug() << "Unable to set the pipeline to the playing state." << endl;
    unloadMovie();
    return false;
  }
  else
  {
    _setReady(play);

    return true;
  }
}

void MediaImpl::_setReady(bool ready)
{
  _movieReady = ready;
//  _VIDEO_OUT->sleeping(!ready);
//  _AUDIO_OUT->sleeping(!ready);
}

void MediaImpl::_setFinished(bool finished) {
//  qDebug() << "Clip " << (finished ? "finished" : "not finished") << endl;
}

void MediaImpl::gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl::GstPadHandlerData* data) {
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (newPad), GST_ELEMENT_NAME (src));
  bool isAudio = false;
  GstPad *sinkPad = NULL;

  // Check the new pad's type.
  GstCaps *newPadCaps = gst_pad_query_caps (newPad, NULL);
  GstStructure *newPadStruct = gst_caps_get_structure (newPadCaps, 0);
  const gchar *newPadType   = gst_structure_get_name (newPadStruct);
  g_print("Structure is %s\n", gst_structure_to_string(newPadStruct));
  if (g_str_has_prefix (newPadType, "audio/x-raw"))
  {
    sinkPad = gst_element_get_static_pad (data->audioToConnect, "sink");
    isAudio = true;
  }
  else if (g_str_has_prefix (newPadType, "video/x-raw"))
  {
    sinkPad = gst_element_get_static_pad (data->videoToConnect, "sink");
    isAudio = false;
  }
  else
  {
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
  if (GST_PAD_LINK_FAILED (gst_pad_link (newPad, sinkPad))) {
    g_print ("  Type is '%s' but link failed.\n", newPadType);
    goto exit;
  } else {
    g_print ("  Link succeeded (type '%s').\n", newPadType);
    if (isAudio)
    {
      data->audioIsConnected = true;
    }
    else
    {
      data->videoIsConnected = true;
    }
  }

exit:
  // Unreference the new pad's caps, if we got them.
  if (newPadCaps != NULL)
    gst_caps_unref (newPadCaps);

  // Unreference the sink pad.
  if (sinkPad != NULL)
    gst_object_unref (sinkPad);
}

//void VideoImpl::gstNewAudioBufferCallback(GstElement *sink, GstNewAudioBufferHandlerData *data) {
//  GstBuffer *buffer = NULL;
//
//  // Retrieve the buffer.
//  // TODO: we should pull ALL buffers and add them to the adapter
//  g_signal_emit_by_name (data->audioSink, "pull-buffer", &buffer);
//
//  if (buffer)
//  {
//    ASSERT_WARNING_MESSAGE( ! GST_BUFFER_IS_DISCONT(buffer), "Discontinuity detected in audio buffer." );
//
////    int blockSize  = 2;
////    int sampleRate = 1;
////    int channels  = 0;
////    int width = 0;
////    GstCaps* caps = GST_BUFFER_CAPS(buffer);
////    GstStructure *capsStruct = gst_caps_get_structure (caps, 0);
////
////    gst_structure_get_int(capsStruct, "rate",  &sampleRate);
////    gst_structure_get_int(capsStruct, "channels", &channels);
////    gst_structure_get_int(capsStruct, "width",  &width);
//
////    qDebug() << "rate = " << sampleRate << " channels = " << channels << " width = " << width << endl;
////    unsigned int blockByteSize = Engine::signalInfo().blockSize() * sizeof(Signal_T);
//
////    qDebug() << "bufsize: "<< GST_BUFFER_SIZE(buffer) <<
////                 " / adaptersize: " << gst_adapter_available(data->audioBufferAdapter) << endl;
//
//    // Add buffer to the adapter.
//    gst_adapter_push(data->audioBufferAdapter, buffer);
// //   qDebug() << " .. after push = : "<< gst_adapter_available(_audioBufferAdapter);
//
//    // NOTE: no need to unref the buffer here because the buffer was given away with the
//    // call to gst_adapter_push()
//    //gst_buffer_unref (buffer);
//  }
//}

void MediaImpl::internalPrePlay()
{
  // Start/resume playback.
  _setPlayState(true);
}

void MediaImpl::internalPostPlay()
{
  // Pause playback.
  _setPlayState(false);
}





