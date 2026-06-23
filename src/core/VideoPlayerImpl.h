/*
 * VideoPlayerImpl.h
 *
 * (c) 2024 MapMap contributors
 *
 * Qt 6 Multimedia video playback using QMediaPlayer + QVideoSink + QAudioOutput.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef VIDEO_PLAYER_IMPL_H_
#define VIDEO_PLAYER_IMPL_H_

#include "VideoImpl.h"

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>

namespace mmp {

/**
 * File/URI video playback using Qt 6 QMediaPlayer.
 * Delivers RGBA frames via QVideoSink into VideoImpl::_data so that
 * callers (ShapeGraphicsItem etc.) can upload them to OpenGL unchanged.
 */
class VideoPlayerImpl : public QObject, public VideoImpl
{
  Q_OBJECT

public:
  VideoPlayerImpl();
  ~VideoPlayerImpl() override;

  bool loadMovie(const QString& path) override;
  bool isLive() override { return false; }

  bool setPlayState(bool play);
  bool seekTo(qint64 positionMs) override;
  void setRate(double rate) override;
  void setVolume(double volume) override;

  void update() override;

protected:
  // Releases the player/audio/sink (called by unloadMovie() and the destructor).
  void freeResources() override;

private slots:
  void onVideoFrameChanged(const QVideoFrame& frame);
  void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
  QMediaPlayer *_player;
  QAudioOutput *_audioOutput;
  QVideoSink   *_videoSink;
  bool          _eos;
};

}

#endif // VIDEO_PLAYER_IMPL_H_
