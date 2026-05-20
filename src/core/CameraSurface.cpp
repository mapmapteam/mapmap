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

#ifdef Q_OS_WIN
  _temporaryImage = img;
#else
  // Straighten the image for OpenGL (bottom-left origin convention).
  _temporaryImage = img.mirrored(true, false).transformed(QTransform().rotate(180));
#endif
}

}
