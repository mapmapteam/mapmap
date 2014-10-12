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
  return _padHandlerData.width;
}

int MediaImpl::getHeight() const
{
  return _padHandlerData.height;
}

const uchar* MediaImpl::getBits() const
{
  return _data;
}

void MediaImpl::build()
{
  qDebug() << "Building video impl";
  if (!loadMovie(_uri))
  {
    qDebug() << "Cannot load movie " << _currentMovie << ".";
  }
}

MediaImpl::~MediaImpl()
{
  freeResources();
  /* _data points to gstreamer-allocated data, we don't manage it ourselves */
  //if (_data)
  //  free(_data);
}

bool MediaImpl::_videoPull()
{
  GstSample *sample = _queueInputBuffer.get();

  if (sample == NULL)
  {
    // Either means we are not playing or we have reached EOS.
    return false;
  }
  else
  {
    // Pull current frame buffer.
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    GstMapInfo map; 
    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    { 
      // For debugging:
      //gst_util_dump_mem(map.data, map.size)
      _data = map.data;
      gst_buffer_unmap(buffer, &map); 
      if(this->_frame != NULL)
        _queueOutputBuffer.put(this->_frame);
      _frame = sample;
    } 

    return true;
  }
}

bool MediaImpl::_eos() const
{
  if (_movieReady)
  {
    Q_ASSERT( _videoSink );
    gboolean videoEos;
    g_object_get (G_OBJECT (_videoSink), "eos", &videoEos, NULL);
    return (bool) (videoEos);
  }
  else
    return false;
}

GstFlowReturn MediaImpl::gstNewSampleCallback(GstElement*, MediaImpl *p)
{
  GstSample *sample;
  sample = gst_app_sink_pull_sample(GST_APP_SINK(p->_videoSink));
  //g_signal_emit_by_name (p->_videoSink, "pull-sample", &sample);
  p->getQueueInputBuffer()->put(sample);
  if (p->getQueueOutputBuffer()->size() > 1) {
    sample = p->getQueueOutputBuffer()->get();
    gst_sample_unref(sample);
  }
  return GST_FLOW_OK;
}

MediaImpl::MediaImpl(const QString uri) :
_currentMovie(""),
_bus(NULL),
_pipeline(NULL),
_source(NULL),
_videoQueue(NULL),
_videoConvert(NULL),
_videoColorSpace(NULL),
_audioSink(NULL),
_videoSink(NULL),
_frame(NULL),
_width(640),
_height(480),
_data(NULL),
_seekEnabled(false),
_movieReady(false),
_uri(uri)
{
  if (uri != "")
    loadMovie(uri);
}

void MediaImpl::unloadMovie()
{
  // Free allocated resources.
  freeResources();

  // Reset variables.
  _terminate = false;
  _seekEnabled = false;

  // Un-ready.
  _setReady(false);
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
  _videoQueue = NULL;
  _videoConvert = NULL;
  _videoColorSpace = NULL;
  _audioSink = NULL;
  _videoSink = NULL;
  _frame = NULL;
  _padHandlerData = GstPadHandlerData();
}

void MediaImpl::resetMovie()
{
  // TODO: Check if we can still seek when we reach EOS. It seems like it's then impossible and we
  // have to reload but it seems weird so we should check.
  if (!_eos() && _seekEnabled)
  {
    qDebug() << "Seeking at position 0.";
    gst_element_seek_simple (_pipeline, GST_FORMAT_TIME,
                             (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);
    this->_frame = NULL;
    _setReady(true);
  }
  else
  {
    // Just reload movie.
    qDebug() << "Reloading the movie" << _seekEnabled;
    _currentMovie = "";
    loadMovie(_uri);
  }
}

bool MediaImpl::loadMovie(QString filename)
{
  _uri = filename;

  qDebug() << "Opening movie: " << filename << ".";
  this->_frame = NULL;

  // Free previously allocated structures
  unloadMovie();

  // Initialize GStreamer.
  gst_init (NULL, NULL);
  GstElement *capsFilter = NULL;
  GstElement *videoScale = NULL;

  // Create the elements.
  _source =          gst_element_factory_make ("uridecodebin", "source");
  _videoQueue =      gst_element_factory_make ("queue", "vqueue");
  _videoColorSpace = gst_element_factory_make ("videoconvert", "vcolorspace");
  videoScale = gst_element_factory_make ("videoscale", "videoscale0");
  capsFilter = gst_element_factory_make ("capsfilter", "capsfilter0");
  _videoSink =       gst_element_factory_make ("appsink", "vsink");

  // Prepare handler data.
  _padHandlerData.videoToConnect   = _videoQueue;
  _padHandlerData.videoSink        = _videoSink;
  _padHandlerData.videoIsConnected = false;

  // Create the empty pipeline.
  _pipeline = gst_pipeline_new ( "video-source-pipeline" );

  if (!_pipeline || !_source ||
      !_videoQueue || !_videoColorSpace || ! videoScale || ! capsFilter || ! _videoSink)
  {
    g_printerr ("Not all elements could be created.\n");
    unloadMovie();
    return -1;
  }

  // Build the pipeline. Note that we are NOT linking the source at this
  // point. We will do it later.
  gst_bin_add_many (GST_BIN (_pipeline), _source,
                    _videoQueue, _videoColorSpace, videoScale, capsFilter, _videoSink, NULL);

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
      qDebug() << "Filename to URI error: " << error->message;
      g_error_free(error);
      gst_object_unref (uri);
      freeResources();
      return false;
    }
  }

  // Set URI to be played.
  qDebug() << "URI for uridecodebin: " << uri;
  // FIXME: sometimes it's just the path to the directory that is given, not the file itself.
  g_object_set (_source, "uri", uri, NULL);
  // Connect to the pad-added signal
  g_signal_connect (_source, "pad-added", G_CALLBACK (MediaImpl::gstPadAddedCallback), &_padHandlerData);

  // Configure video appsink.
  GstCaps *videoCaps = gst_caps_from_string ("video/x-raw,format=RGBA");
  g_object_set (capsFilter, "caps", videoCaps, NULL);
  g_object_set (_videoSink, "emit-signals", TRUE,
                            "max-buffers", 1,     // only one buffer (the last) is maintained in the queue
                            "drop", TRUE,         // ... other buffers are dropped
                            NULL);
  g_signal_connect (_videoSink, "new-sample", G_CALLBACK (MediaImpl::gstNewSampleCallback), this);
  gst_caps_unref (videoCaps);

  // Listen to the bus.
  _bus = gst_element_get_bus (_pipeline);

  // Start playing.
  if (!setPlayState(true))
    return false;

  qDebug() << "Pipeline started.";

  //_movieReady = true;
  return true;
}

bool MediaImpl::runVideo() {

  if (!_preRun())
    return false;

  bool bitsChanged = false;

  // Check if we have some frames in the input buffer.
  if (_queueInputBuffer.size() > 0) {

    // Pull video.
    if (_videoPull())
      bitsChanged = true;

    //std::cout << "VideoImpl::runVideo: read frame #" << _videoNewBufferCounter << std::endl;
  }

  /* TODO: This causes the texture to be loaded always in Mapper.cpp . The
 * problem if this is not set is: When we have more than one shape, a
 * shape that has a new buffer coming in will overdraw the old buffer of the
 * shape on top. This implementation seems to be fast enough that
 * _videoNewBufferCounter is often 1 or 0. If bitsChanged is often switching
 * between true and false (as in the case described above), than the shape
 * textures will appear to be flickering/alternating. Maybe a better solution is
 * needed (in the GL layer or here?)*/
  else
      bitsChanged = true;

  _postRun();

  return bitsChanged;
}

bool MediaImpl::setPlayState(bool play)
{
  if (_pipeline == NULL)
    return false;

  GstStateChangeReturn ret = gst_element_set_state (_pipeline, (play ? GST_STATE_PLAYING : GST_STATE_PAUSED));
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    qDebug() << "Unable to set the pipeline to the playing state.";
    unloadMovie();
    return false;
  }
  else
  {
    _setReady(play);

    return true;
  }
}

bool MediaImpl::_preRun()
{
  // Check for end-of-stream or terminate.
  if (_eos() || _terminate)
    resetMovie();

  if (!_movieReady ||
      !_padHandlerData.videoIsConnected)
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
        break;

      case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        break;

      case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline.
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (_pipeline))
        {
          GstState oldState, newState, pendingState;
          gst_message_parse_state_changed(msg, &oldState, &newState,
              &pendingState);
          g_print("Pipeline state for movie %s changed from %s to %s:\n",
              _currentMovie.toUtf8().constData(),
              gst_element_state_get_name(oldState),
              gst_element_state_get_name(newState));

          if (newState == GST_STATE_PLAYING)
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

void MediaImpl::_setReady(bool ready)
{
  _movieReady = ready;
}

void MediaImpl::gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl::GstPadHandlerData* data) {
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (newPad), GST_ELEMENT_NAME (src));
  GstPad *sinkPad = NULL;

  // Check the new pad's type.
  GstCaps *newPadCaps = gst_pad_query_caps (newPad, NULL);
  GstStructure *newPadStruct = gst_caps_get_structure (newPadCaps, 0);
  const gchar *newPadType   = gst_structure_get_name (newPadStruct);
  g_print("Structure is %s\n", gst_structure_to_string(newPadStruct));
  if (g_str_has_prefix (newPadType, "video/x-raw"))
  {
    sinkPad = gst_element_get_static_pad (data->videoToConnect, "sink");
    gst_structure_get_int(newPadStruct, "width",  &data->width);
    gst_structure_get_int(newPadStruct, "height", &data->height);
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
    data->videoIsConnected = true;
    g_print ("  Link succeeded (type '%s').\n", newPadType);
  }

exit:
  // Unreference the new pad's caps, if we got them.
  if (newPadCaps != NULL)
    gst_caps_unref (newPadCaps);

  // Unreference the sink pad.
  if (sinkPad != NULL)
    gst_object_unref (sinkPad);
}
