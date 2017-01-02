/*
 * VideoUriDecodeBinImpl.cpp
 *
 * (c) 2016 Vasilis Liaskovitis -- vliaskov@gmail.com
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
#include "VideoUriDecodeBinImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoUriDecodeBinImpl::VideoUriDecodeBinImpl() :
_uridecodebin0(NULL)
{
}

void VideoUriDecodeBinImpl::gstPadAddedCallback(GstElement *src, GstPad *newPad, VideoUriDecodeBinImpl* p)
{
  Q_UNUSED(src);
#ifdef VIDEO_IMPL_VERBOSE
#ifndef Q_OS_OSX
  // NOTE: This line was causing a problem on Mac OSX: it caused the software to freeze when loading a new movie.
  qDebug() << "Received new pad '" << GST_PAD_NAME(newPad) << "' from '" << GST_ELEMENT_NAME (src) << "'." << endl;
#endif
#endif

  GstPad *sinkPad = NULL;

  // Check the new pad's type.
  GstCaps *newPadCaps = gst_pad_query_caps (newPad, NULL);
  GstStructure *newPadStruct = gst_caps_get_structure (newPadCaps, 0);
  const gchar *newPadType   = gst_structure_get_name (newPadStruct);
  gchar *newPadStructStr = gst_structure_to_string(newPadStruct);
#ifdef VIDEO_IMPL_VERBOSE
  qDebug() << "Structure is " << newPadStructStr << "." << endl;
#endif
  g_free(newPadStructStr);

  bool isVideoPad = g_str_has_prefix (newPadType, "video/x-raw");
  bool isAudioPad = g_str_has_prefix (newPadType, "audio/x-raw");

  // Check for video pads.
  if (isVideoPad)
  {
    sinkPad = gst_element_get_static_pad (p->_queue0, "sink");
    gst_structure_get_int(newPadStruct, "width",  &p->_width);
    gst_structure_get_int(newPadStruct, "height", &p->_height);
  }

  // Check for audio pads.
  else if (isAudioPad)
  {
    if (!p->createAudioComponents())
    {
      qWarning() << "Problem creating audio components." << endl;
      goto exit;
    }
    sinkPad = gst_element_get_static_pad (p->_audioqueue0, "sink");
  }

  // Other types: ignore.
  else {
    qDebug() << "  It has type '" << newPadType << "' which is not raw audio/video: ignored." << endl;
    goto exit;
  }


  // If our converter is already linked, we have nothing to do here.
  if (gst_pad_is_linked (sinkPad))
  {
    // Best prefixes.
    if (isVideoPad || isAudioPad)
    {
      qDebug() << "  Found a better pad." << endl;
      GstPad* oldPad = gst_pad_get_peer(sinkPad);
      gst_pad_unlink(oldPad, sinkPad);
      g_object_unref(oldPad);
    }
    else
    {
#ifdef VIDEO_IMPL_VERBOSE
      qDebug() << "  We are already linked: ignoring." << endl;
#endif
      goto exit;
    }
  }

  // Attempt the link.
  if (GST_PAD_LINK_FAILED (gst_pad_link (newPad, sinkPad)))
  {
#ifdef VIDEO_IMPL_VERBOSE
    qDebug() << "  Type is '" << newPadType << "' but link failed." << endl;
#endif // ifdef
    goto exit;
  }
  else
  {
    if (isVideoPad)
      p->videoConnect();
    else if (isAudioPad)
      p->audioConnect();
    else
      qWarning() << "Error: this pad is neither valid audio or video." << endl;
#ifdef VIDEO_IMPL_VERBOSE
    qDebug() << "  Link succeeded (type '" << newPadType << "')." << endl;
#endif // ifdef
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

bool VideoUriDecodeBinImpl::loadMovie(const QString& path) {
  VideoImpl::loadMovie(path);

  _uridecodebin0 = gst_element_factory_make("uridecodebin", NULL);

  if ( !_uridecodebin0)
  {
    qWarning() << "Not all elements could be created." << endl;
    unloadMovie();
    return (-1);
  }

  // Build the pipeline. Note that we are NOT linking the source at this
  // point. We will do it later.
  gst_bin_add_many (GST_BIN (_pipeline),
      _uridecodebin0,
      NULL);

  // Process URI.
  QByteArray ba = path.toLocal8Bit();
  gchar *filename_tmp = g_strdup((gchar*) path.toUtf8().constData());
  gchar* uri = (gchar*) path.toUtf8().constData();
  if (! gst_uri_is_valid(uri))
  {
    // Try to convert filename to URI.
    GError* error = NULL;
    qDebug() << "Calling gst_filename_to_uri : " << uri << endl;
    uri = gst_filename_to_uri(filename_tmp, &error);
    if (error)
    {
      qDebug() << "Filename to URI error: " << error->message << endl;
      g_clear_error(&error);
      gst_object_unref (uri);
      freeResources();
      return false;
    }
  }
  g_free(filename_tmp);

// Connect to the pad-added signal
  // Extract meta info.
  GError* error = NULL;
  GstDiscoverer* discoverer = gst_discoverer_new(5*GST_SECOND, &error);
  if (!discoverer)
  {
    qDebug() << "Error creating discoverer: " << error->message << endl;
    g_clear_error (&error);
    return false;
  }

  GstDiscovererInfo* info = gst_discoverer_discover_uri(discoverer, uri, &error);

  if (!info)
  {
    qDebug() << "Error getting discoverer info: " << error->message << endl;
    g_clear_error (&error);
    return false;
  }

  GstDiscovererResult result = gst_discoverer_info_get_result(info);

  switch (result) {
    case GST_DISCOVERER_URI_INVALID:
      qDebug()<< "Invalid URI '" << uri << "'" << endl;
      break;
    case GST_DISCOVERER_ERROR:
      qDebug()<< "Discoverer error: " << error->message << endl;
      break;
    case GST_DISCOVERER_TIMEOUT:
      qDebug() << "Timeout" << endl;
      break;
    case GST_DISCOVERER_BUSY:
      qDebug() << "Busy" << endl;
      break;
    case GST_DISCOVERER_MISSING_PLUGINS:{
      const GstStructure *s;
      gchar *str;

      s = gst_discoverer_info_get_misc (info);
      str = gst_structure_to_string (s);

      qDebug() << "Missing plugins: " << str << endl;
      g_free (str);
      break;
    }
    case GST_DISCOVERER_OK:
      qDebug() << "Discovered '" << uri << "'" << endl;
      break;
  }

  g_clear_error (&error);

  if (result != GST_DISCOVERER_OK) {
    qDebug() << "This URI cannot be played" << endl;
    return false;
  }

  // Gather info from video.
  GList *videoStreams = gst_discoverer_info_get_video_streams (info);
  if (!videoStreams)
  {
    qDebug() << "This URI does not contain any video streams" << endl;
    return false;
  }

  // Retrieve meta-info.
  _width = gst_discoverer_video_info_get_width((GstDiscovererVideoInfo*)videoStreams->data);
  _height = gst_discoverer_video_info_get_height((GstDiscovererVideoInfo*)videoStreams->data);
  _duration = gst_discoverer_info_get_duration(info);
  _seekEnabled = gst_discoverer_info_get_seekable(info);

  // Free everything.
  g_object_unref(discoverer);
  gst_discoverer_info_unref(info);
  gst_discoverer_stream_info_list_free(videoStreams);

  // Connect pad signal.
  g_signal_connect (_uridecodebin0, "pad-added", G_CALLBACK (VideoUriDecodeBinImpl::gstPadAddedCallback), this);

  // Set uri of decoder.
  g_object_set (_uridecodebin0, "uri", uri, NULL);

  setPlayState(true);

  return true;
}

VideoUriDecodeBinImpl::~VideoUriDecodeBinImpl()
{
}

}
