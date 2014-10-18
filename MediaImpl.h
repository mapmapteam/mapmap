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
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <tr1/memory>
#include "AsyncQueue.h"

/**
 * Private declaration of the video player.
 * This is done this way so that GStreamer header don't need to be 
 * included in the whole project. (just in this file)
 */
class MediaImpl
{
public:
  MediaImpl(const QString uri, bool live);
  ~MediaImpl();

//  void setUri(const QString uri);
  static bool hasVideoSupport();
  void build();
  int getWidth() const;
  int getHeight() const;
  QString getUri() const;
  bool getAttached();
  const uchar* getBits() const;

  bool isReady() const { return _padHandlerData.videoIsConnected; }

  /// Returns true if the image has changed.
  bool runVideo();
//  void runAudio();
  bool loadMovie(QString filename);
  bool setPlayState(bool play);
  void setAttached(bool attach);

  void resetMovie();

protected:
  void unloadMovie();
  void freeResources();

private:
  bool _videoPull();
  bool _eos() const;
//  void _finish();
//  void _init();

  bool _preRun();
  void _postRun();
  void _setReady(bool ready);
  void _setFinished(bool finished);

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
  static GstFlowReturn gstNewPreRollCallback (GstAppSink * appsink, gpointer user_data);

  // GStreamer callback that plugs the audio/video pads into the proper elements when they
  // are made available by the source.
  static void gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl::GstPadHandlerData* data);
  AsyncQueue<GstSample*> *getQueueInputBuffer() {
    return &this->_queueInputBuffer;
  }

  AsyncQueue<GstSample*> *getQueueOutputBuffer() {
    return &this->_queueOutputBuffer;
  }

private:
  //locals
  QString _currentMovie;

  // gstreamer
  GstBus *_bus;
  GstElement *_pipeline;
  GstElement *_source;
  GstElement *_deserializer;
  GstElement *_audioQueue;
  GstElement *_audioConvert;
  GstElement *_audioResample;
  GstElement *_videoQueue;
  GstElement *_videoConvert;
  GstElement *_videoColorSpace;
  GstElement *_audioSink;
  GstElement *_videoSink;
  GstSample  *_frame;
  GSource *_pollSource;

  GstPadHandlerData _padHandlerData;

  int _width;
  int _height;
  uchar *_data;

  bool _seekEnabled;
  bool _live;
  bool _attached;

  int _audioNewBufferCounter;

  bool _terminate;
  bool _movieReady;
  AsyncQueue<GstSample*> _queueInputBuffer;
  AsyncQueue<GstSample*> _queueOutputBuffer;

private:
  QString _uri;

};

#endif /* ifndef */
