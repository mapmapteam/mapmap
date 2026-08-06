/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** ...
** $QT_END_LICENSE$
**
****************************************************************************/

#include "CameraSurface.h"
#include <QDebug>

namespace mmp {

CameraSurface::CameraSurface(QObject *parent)
  : QObject(parent),
    _videoSink(new QVideoSink(this))
{
  connect(_videoSink, &QVideoSink::videoFrameChanged,
          this, &CameraSurface::onVideoFrameChanged);
}

CameraSurface::~CameraSurface()
{
}

void CameraSurface::onVideoFrameChanged(const QVideoFrame& frame)
{
  if (!frame.isValid())
    return;

  QImage img = frame.toImage().convertToFormat(QImage::Format_RGBA8888);
  if (img.isNull())
    return;

  // Qt 6 delivers upright, top-left-origin frames here on every platform —
  // the same as the video-file path (VideoPlayerImpl), which shares the
  // OpenGL upload — so no flip is needed. The old flip (a historical OpenGL
  // orientation workaround, mirror + 180-degree rotation, net a vertical
  // flip) made camera sources appear upside-down; already removed on
  // macOS/Windows in f1e51b4, confirmed to be the same bug on Linux by
  // injecting a synthetic top-red/bottom-blue QVideoFrame directly into this
  // class and observing the flip.
  _temporaryImage = img;
}

}
