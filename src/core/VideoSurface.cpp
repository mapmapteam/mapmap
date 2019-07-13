/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "VideoSurface.h"

#include <QVideoSurfaceFormat>
#include <QGLWidget>

namespace mmp {

VideoSurface::VideoSurface(QObject *parent)
    : QAbstractVideoSurface(parent)
    // , imageFormat(QImage::Format_Invalid)
    // , framePainted(false)
{
}

VideoSurface::~VideoSurface()
{
}

QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_RGB24
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_BGR32
                << QVideoFrame::Format_BGR24
                << QVideoFrame::Format_BGR565
                << QVideoFrame::Format_BGR555
                ;
    } else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool VideoSurface::isFormatSupported(const QVideoSurfaceFormat &format, QVideoSurfaceFormat *similar) const
{
  Q_UNUSED(similar)

  const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
  const QSize size = format.frameSize();

  return imageFormat != QImage::Format_Invalid
          && !size.isEmpty()
          && format.handleType() == QAbstractVideoBuffer::NoHandle;
}

bool VideoSurface::present(const QVideoFrame &frame)
{
  // Copy current frame.
  QVideoFrame currentFrame(frame);

  // Convert frame into QImage with appropriate format.
  // Source: https://stackoverflow.com/questions/27829830/convert-qvideoframe-to-qimage
  if (currentFrame.map(QAbstractVideoBuffer::ReadOnly))
  {
    QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(currentFrame.pixelFormat());
    if (imageFormat != QImage::Format_Invalid) {
      _img = QImage(currentFrame.bits(), currentFrame.width(), currentFrame.height(), imageFormat);
    } else {
       // e.g. JPEG
       int nbytes = currentFrame.mappedBytes();
       _img = QImage::fromData(currentFrame.bits(), nbytes);
    }

    currentFrame.unmap();

    // Convert to OpenGLformat and apply transforms to straighten.
    _img = QGLWidget::convertToGLFormat(_img)
            .mirrored(true, false)
            .transformed(QTransform().rotate(180));

    return true;
  }
  else
    return false;
}

const uchar* VideoSurface::bits()
{
  return _img.bits();
}

}
