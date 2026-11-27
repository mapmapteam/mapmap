/*
 * VideoPlayerImpl.cpp
 *
 * (c) 2024 MapMap contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "VideoPlayerImpl.h"
#include <QDebug>
#include <QUrl>

namespace mmp {

VideoPlayerImpl::VideoPlayerImpl()
  : _player(nullptr),
    _audioOutput(nullptr),
    _videoSink(nullptr),
    _eos(false)
{
}

VideoPlayerImpl::~VideoPlayerImpl()
{
  freeResources();
}

void VideoPlayerImpl::freeResources()
{
  if (_player) {
    _player->stop();
    delete _player;
    _player = nullptr;
  }
  delete _audioOutput;
  _audioOutput = nullptr;
  delete _videoSink;
  _videoSink = nullptr;
  _eos = false;
  VideoImpl::freeResources();
}

bool VideoPlayerImpl::loadMovie(const QString& path)
{
  // Store URI in base class.
  VideoImpl::loadMovie(path);

  // Tear down any previous player and reset frame state.
  freeResources();

  _player      = new QMediaPlayer(this);
  _audioOutput = new QAudioOutput(this);
  _videoSink   = new QVideoSink(this);

  _player->setAudioOutput(_audioOutput);
  _player->setVideoSink(_videoSink);

  // Direct connection: frames are copied / converted on the decoder's thread
  // (see _setFrame()), keeping that work off the GUI thread.
  connect(_videoSink, &QVideoSink::videoFrameChanged,
          this, &VideoPlayerImpl::onVideoFrameChanged, Qt::DirectConnection);
  connect(_player, &QMediaPlayer::mediaStatusChanged,
          this, &VideoPlayerImpl::onMediaStatusChanged);

  _player->setSource(QUrl::fromLocalFile(path));

  if (_playInLoop)
    _player->setLoops(QMediaPlayer::Infinite);

  // Seek is always enabled for file sources in Qt 6.
  _seekEnabled = true;
  _videoIsConnected = true;
  _audioIsConnected = true;

  _audioOutput->setVolume(_volume);
  _playState = true;
  _applyRateAndPlayState();

  return true;
}

void VideoPlayerImpl::onVideoFrameChanged(const QVideoFrame& frame)
{
  if (!frame.isValid())
    return;

  // Runs on the decoder's thread; locks the mutex only to swap the frame.
  _setFrame(frame);
  _videoIsConnected = true;
}

void VideoPlayerImpl::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  switch (status) {
  case QMediaPlayer::LoadedMedia:
    _duration = _player->duration();
    _setMovieReady(true);
    break;
  case QMediaPlayer::EndOfMedia:
    _eos = true;
    break;
  case QMediaPlayer::InvalidMedia:
    qWarning() << "Invalid media:" << _player->errorString();
    break;
  default:
    break;
  }
}

bool VideoPlayerImpl::setPlayState(bool play)
{
  if (!_player)
    return false;
  _playState = play;
  _applyRateAndPlayState();
  return true;
}

bool VideoPlayerImpl::seekTo(qint64 positionMs)
{
  if (!_player || !_seekEnabled)
    return false;
  _player->setPosition(positionMs);
  return true;
}

void VideoPlayerImpl::setRate(double rate)
{
  _rate = rate;
  _applyRateAndPlayState();
}

void VideoPlayerImpl::_applyRateAndPlayState()
{
  if (!_player)
    return;
  // QMediaPlayer races through the video at a speed of zero and cannot play backwards: at a
  // speed of zero or less, the video is paused instead, and plays again at a positive
  // speed if it was playing. A negative speed is kept, for when the video can play backwards
  // (#667).
  if (_rate <= 0.0) {
    _player->pause();
    return;
  }
  _player->setPlaybackRate(_rate);
  if (_playState)
    _player->play();
  else
    _player->pause();
}

void VideoPlayerImpl::setVolume(double volume)
{
  _volume = volume;
  if (_audioOutput)
    _audioOutput->setVolume(volume);
}

void VideoPlayerImpl::update()
{
  if (_eos) {
    _eos = false;
    if (_playInLoop)
      resetMovie();
  }
}

}
