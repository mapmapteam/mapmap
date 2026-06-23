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

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
  // Qt 6 delivers upright, top-left-origin frames here — the same as the
  // video-file path (VideoPlayerImpl), which shares the OpenGL upload — so no
  // flip is needed. The old flip made macOS camera sources appear upside-down.
  _temporaryImage = img;
#else
  // Linux: historical orientation fix (kept; adjust if cameras look flipped).
  QT_WARNING_PUSH
  QT_WARNING_DISABLE_DEPRECATED
  _temporaryImage = img.mirrored(true, false).transformed(QTransform().rotate(180));
  QT_WARNING_POP
#endif
}

}
