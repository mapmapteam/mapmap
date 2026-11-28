/*
 * YuvTexture.h
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

#ifndef YUV_TEXTURE_H_
#define YUV_TEXTURE_H_

#include <QOpenGLFunctions>
#include <QVideoFrame>
#include <QMatrix4x4>
#include <QVector>

namespace mmp {

/**
 * Draws planar / semi-planar YUV video frames without any CPU colour
 * conversion: each plane is uploaded as-is to its own texture and a fragment
 * shader converts to RGB while drawing.
 *
 * Converting on the CPU (QVideoFrame::toImage()) costs ~60 ms per 1080p frame
 * on a Raspberry Pi 4, which is more than a frame period; uploading the raw
 * planes moves 1.5 bytes/pixel instead of 4 and costs no conversion at all.
 *
 * The conversion is done by the YUV shader of ShapeRenderer, which reads the
 * planes that bind() binds. All calls need a current GL context.
 */
class YuvTexture
{
public:
  YuvTexture();
  ~YuvTexture();

  /// True if frames in this pixel format can be drawn by YuvTexture.
  static bool supportsFormat(QVideoFrameFormat::PixelFormat format);

  /// False once the shaders of ShapeRenderer failed to build on this system
  /// (callers should then fall back to RGB conversion).
  static bool isAvailable() { return !_unavailable; }

  /// Uploads the planes of a frame (textures are reallocated only when the
  /// frame geometry changes). Returns false on failure.
  bool upload(const QVideoFrame& frame);

  /// Binds plane textures (units 0-2), for ShapeRenderer::drawTriangles().
  /// Leaves texture unit 0 active.
  void bind();

  /// Unbinds units 1-2. Leaves texture unit 0 active.
  void release();

  /// How the chroma is stored: 0 for separate U and V planes, 1 for interleaved
  /// UV (NV12), 2 for interleaved VU (NV21).
  int chromaLayout() const { return _chromaLayout; }

  /// Matrix mapping (Y, U, V, 1) of the last uploaded frame to RGB (see colorMatrix()).
  const QMatrix4x4& conversionMatrix() const { return _colorMatrix; }

  /// Texture ids owned by this object (0 if not allocated yet), so the owner
  /// can schedule their deletion when no context is current.
  QVector<GLuint> textureIds() const;

  /// Matrix mapping (Y, U, V, 1) in [0,1] to RGB for the given colour space /
  /// range. Public for testing.
  static QMatrix4x4 colorMatrix(const QVideoFrameFormat& format);

private:
  struct Plane {
    GLuint id = 0;
    int width = 0, height = 0;
    GLenum glFormat = 0;
  };

  bool _uploadPlane(QOpenGLFunctions* gl, int index, const uchar* bits, int bytesPerLine,
                    int width, int height, GLenum glFormat, int bytesPerTexel);

  Plane _planes[3];
  int _nPlanes;
  int _chromaLayout; // 0: separate U/V planes, 1: interleaved UV (NV12), 2: VU (NV21)
  QMatrix4x4 _colorMatrix;

  static bool _unavailable;
};

}

#endif /* YUV_TEXTURE_H_ */
