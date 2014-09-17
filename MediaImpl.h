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
  MediaImpl(const QString uri);
  ~MediaImpl();

//  void setUri(const QString uri);
  static bool hasVideoSupport();
  void build();
  int getWidth() const;
  int getHeight() const;
  const uchar* getBits() const;

  bool isReady() const { return _padHandlerData.isConnected(); }

  /// Returns true if the image has changed.
  bool runVideo();
//  void runAudio();
  bool loadMovie(QString filename);

  bool setPlayState(bool play);
  void resetMovie();

protected:
  void unloadMovie();
  void freeResources();

  void internalPrePlay();
  void internalPostPlay();

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
    GstElement* audioToConnect;
    GstElement* videoToConnect;
    GstElement* videoSink;
    bool audioIsConnected;
    bool videoIsConnected;
    int width;
    int height;

    GstPadHandlerData() :
      audioToConnect(NULL), videoToConnect(NULL), videoSink(NULL),
      audioIsConnected(false), videoIsConnected(false),
      width(-1), height(-1)
    {}

    bool isConnected() const {
      return (/*audioIsConnected && */videoIsConnected);
    }
  };

//  struct GstNewAudioBufferHandlerData {
//    GstElement* audioSink;
//    GstAdapter* audioBufferAdapter;
//    GstNewAudioBufferHandlerData() : audioSink(NULL), audioBufferAdapter(NULL) {}
//  };

  // GStreamer callback that simply sets the #newSample# flag to point to TRUE.
  static GstFlowReturn gstNewSampleCallback(GstElement*, MediaImpl *p);
  static GstFlowReturn gstNewPreRollCallback (GstAppSink * appsink, gpointer user_data);

//  static void gstNewAudioBufferCallback(GstElement *sink, GstNewAudioBufferHandlerData *data);

  // GStreamer callback that plugs the audio/video pads into the proper elements when they
  // are made available by the source.
  static void gstPadAddedCallback(GstElement *src, GstPad *newPad, MediaImpl::GstPadHandlerData* data);
  AsyncQueue<GstSample*> *get_queue_input_buf() {
    return &this->queue_input_buf;
  }

  AsyncQueue<GstSample*> *get_queue_output_buf() {
    return &this->queue_output_buf;
  }


private:
//  PlugOut<VideoRGBAType> *_VIDEO_OUT;
//  PlugOut<SignalType> *_AUDIO_OUT;
//  PlugOut<ValueType> *_FINISH_OUT;
//  PlugIn<ValueType> *_RESET_IN;
//  PlugIn<StringType> *_MOVIE_IN;
//
//  VideoRGBAType *_imageOut;

  //locals
  QString _currentMovie;

  // gstreamer
  GstBus *_bus;
  GstElement *_pipeline;
  GstElement *_source;
  GstElement *_audioQueue;
  GstElement *_audioConvert;
  GstElement *_audioResample;
  GstElement *_videoQueue;
  GstElement *_videoConvert;
  GstElement *_videoColorSpace;
  GstElement *_audioSink;
  GstElement *_videoSink;
  GstSample  *_frame;

//  GstAdapter *_audioBufferAdapter;

  GstPadHandlerData _padHandlerData;
//  GstNewAudioBufferHandlerData _newAudioBufferHandlerData;

  int _width;
  int _height;
  uchar *_data;

  bool _seekEnabled;

  int _audioNewBufferCounter;

  bool _terminate;
  bool _movieReady;
  AsyncQueue<GstSample*> queue_input_buf;
  AsyncQueue<GstSample*> queue_output_buf;


private:
  QString _uri;
};

//! Fast converts 24-bits color to 32 bits (alpha is set to specified alpha value).
// Based on: http://stackoverflow.com/questions/7069090/convert-rgb-to-rgba-in-c
inline void convert24to32(unsigned char *dst, const unsigned char *src, size_t size, unsigned char alpha=0xff) {
  Q_ASSERT(dst != NULL);
  Q_ASSERT(src != NULL);
  if (size==0) return;
  uint32_t alphaMask = ((uint32_t)alpha) << 24;
  // Copy by 4 byte blocks.
  for (size_t i=size; --i; dst+=4, src+=3)
  {
    *(uint32_t*)(void*)dst = (*(const uint32_t*)(const void*)src) | alphaMask;
  }
  // Copy remaining bytes.
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}

#endif /* ifndef */
