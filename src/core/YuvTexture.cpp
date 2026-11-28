/*
 * YuvTexture.cpp
 *
 * (c) 2026 MapMap contributors
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

#include "YuvTexture.h"

#include "ShapeRenderer.h"

#include <QOpenGLContext>

namespace mmp {

bool YuvTexture::_unavailable = false;

YuvTexture::YuvTexture()
  : _nPlanes(0),
    _chromaLayout(0)
{
}

YuvTexture::~YuvTexture()
{
  // Texture ids are released by the owner (see textureIds()).
}

bool YuvTexture::supportsFormat(QVideoFrameFormat::PixelFormat format)
{
  switch (format) {
  case QVideoFrameFormat::Format_YUV420P:
  case QVideoFrameFormat::Format_YUV422P:
  case QVideoFrameFormat::Format_YV12:
  case QVideoFrameFormat::Format_NV12:
  case QVideoFrameFormat::Format_NV21:
    return true;
  default:
    return false;
  }
}

QVector<GLuint> YuvTexture::textureIds() const
{
  QVector<GLuint> ids;
  for (const Plane& p : _planes)
    if (p.id != 0)
      ids.append(p.id);
  return ids;
}

QMatrix4x4 YuvTexture::colorMatrix(const QVideoFrameFormat& format)
{
  // Luma coefficients.
  float kr, kb;
  QVideoFrameFormat::ColorSpace space = format.colorSpace();
  if (space == QVideoFrameFormat::ColorSpace_Undefined)
    // Same guess as most players: HD and up is BT.709.
    space = (format.frameHeight() >= 720) ? QVideoFrameFormat::ColorSpace_BT709
                                          : QVideoFrameFormat::ColorSpace_BT601;
  switch (space) {
  case QVideoFrameFormat::ColorSpace_BT709:  kr = 0.2126f; kb = 0.0722f; break;
  case QVideoFrameFormat::ColorSpace_BT2020: kr = 0.2627f; kb = 0.0593f; break;
  default:                                   kr = 0.299f;  kb = 0.114f;  break; // BT.601, AdobeRGB
  }
  const float kg = 1.0f - kr - kb;

  // Range expansion (video range unless stated otherwise).
  float yOffset, yScale, cScale;
  if (format.colorRange() == QVideoFrameFormat::ColorRange_Full) {
    yOffset = 0.0f;
    yScale  = 1.0f;
    cScale  = 1.0f;
  } else {
    yOffset = 16.0f / 255.0f;
    yScale  = 255.0f / 219.0f;
    cScale  = 255.0f / 224.0f;
  }

  // R = Y' + rv*Cr ; G = Y' + gu*Cb + gv*Cr ; B = Y' + bu*Cb
  const float rv = 2.0f * (1.0f - kr) * cScale;
  const float gu = -2.0f * kb * (1.0f - kb) / kg * cScale;
  const float gv = -2.0f * kr * (1.0f - kr) / kg * cScale;
  const float bu = 2.0f * (1.0f - kb) * cScale;

  // Y' = (Y - yOffset) * yScale ; Cb = U - 0.5 ; Cr = V - 0.5
  return QMatrix4x4(
    yScale, 0.0f, rv,   -yOffset * yScale - 0.5f * rv,
    yScale, gu,   gv,   -yOffset * yScale - 0.5f * (gu + gv),
    yScale, bu,   0.0f, -yOffset * yScale - 0.5f * bu,
    0.0f,   0.0f, 0.0f, 1.0f);
}

bool YuvTexture::_uploadPlane(QOpenGLFunctions* gl, int index, const uchar* bits, int bytesPerLine,
                              int width, int height, GLenum glFormat, int bytesPerTexel)
{
  if (!bits || width <= 0 || height <= 0 || bytesPerLine < width * bytesPerTexel)
    return false;

  Plane& plane = _planes[index];
  gl->glActiveTexture(GL_TEXTURE0 + index);

  if (plane.id == 0)
  {
    glGenTextures(1, &plane.id);
    glBindTexture(GL_TEXTURE_2D, plane.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else
    glBindTexture(GL_TEXTURE_2D, plane.id);

  // Rows may be padded: read them in place rather than repacking.
  glPixelStorei(GL_UNPACK_ROW_LENGTH, bytesPerLine / bytesPerTexel);

  if (plane.width != width || plane.height != height || plane.glFormat != glFormat)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glFormat, GL_UNSIGNED_BYTE, bits);
    plane.width = width;
    plane.height = height;
    plane.glFormat = glFormat;
  }
  else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, glFormat, GL_UNSIGNED_BYTE, bits);

  return true;
}

bool YuvTexture::upload(const QVideoFrame& constFrame)
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context || !supportsFormat(constFrame.pixelFormat()))
    return false;
  if (!ShapeRenderer::isAvailable()) {
    _unavailable = true;
    return false;
  }

  QVideoFrame frame = constFrame; // shallow copy, needed to map
  if (!frame.map(QVideoFrame::ReadOnly))
    return false;
  const uchar* planeBits[3] = { frame.bits(0), frame.bits(1), frame.bits(2) };

  QOpenGLFunctions* gl = context->functions();
  const int w = frame.width();
  const int h = frame.height();
  const int cw = (w + 1) / 2;
  const int ch = (frame.pixelFormat() == QVideoFrameFormat::Format_YUV422P) ? h : (h + 1) / 2;

  GLint oldAlignment = 4, oldRowLength = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  bool ok = _uploadPlane(gl, 0, planeBits[0], frame.bytesPerLine(0), w, h, GL_LUMINANCE, 1);

  switch (frame.pixelFormat()) {
  case QVideoFrameFormat::Format_NV12:
  case QVideoFrameFormat::Format_NV21:
    _nPlanes = 2;
    _chromaLayout = (frame.pixelFormat() == QVideoFrameFormat::Format_NV12) ? 1 : 2;
    ok = ok && _uploadPlane(gl, 1, planeBits[1], frame.bytesPerLine(1), cw, ch, GL_LUMINANCE_ALPHA, 2);
    break;
  default: {
    // YV12 stores V before U.
    const bool swap = (frame.pixelFormat() == QVideoFrameFormat::Format_YV12);
    _nPlanes = 3;
    _chromaLayout = 0;
    ok = ok && _uploadPlane(gl, swap ? 2 : 1, planeBits[1], frame.bytesPerLine(1), cw, ch, GL_LUMINANCE, 1);
    ok = ok && _uploadPlane(gl, swap ? 1 : 2, planeBits[2], frame.bytesPerLine(2), cw, ch, GL_LUMINANCE, 1);
    break;
  }
  }

  glPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);
  glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment);
  gl->glActiveTexture(GL_TEXTURE0);

  frame.unmap();

  _colorMatrix = colorMatrix(frame.surfaceFormat());
  return ok;
}

void YuvTexture::bind()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context)
    return;

  QOpenGLFunctions* gl = context->functions();
  for (int i = _nPlanes - 1; i >= 0; i--) {
    gl->glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, _planes[i].id);
  }
}

void YuvTexture::release()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context)
    return;

  QOpenGLFunctions* gl = context->functions();
  for (int i = _nPlanes - 1; i >= 1; i--) {
    gl->glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  gl->glActiveTexture(GL_TEXTURE0);
}

}
