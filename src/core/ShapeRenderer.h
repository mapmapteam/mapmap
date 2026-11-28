/*
 * ShapeRenderer.h
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

#ifndef SHAPE_RENDERER_H_
#define SHAPE_RENDERER_H_

#include <QMatrix4x4>
#include <QSize>
#include <QTransform>
#include <QVector>

#include "ShapeGeometry.h"

namespace mmp {

class YuvTexture;

/**
 * Draws textured shapes with shaders.
 *
 * The shaders are in GLSL 1.20, with attributes and parameters only: no
 * ftransform(), gl_Color or matrix stack, so that they move to OpenGL 3.3 and
 * OpenGL ES by changing their header. The programs and the vertex buffer are
 * made once per group of shared OpenGL contexts (all MapMap contexts share one
 * group, see Qt::AA_ShareOpenGLContexts in main.cpp).
 *
 * All calls but viewMatrix() need a current OpenGL context.
 */
class ShapeRenderer
{
public:
  /**
   * Draws triangles (three vertices each) with the texture that Texture::bind()
   * bound, blended over what is drawn by the alpha of the texture times the opacity.
   * Blending stays on afterwards.
   *
   * @param vertices the triangles, in the coordinates that @a matrix transforms
   * @param matrix from those coordinates to OpenGL's clip coordinates (see viewMatrix())
   * @param opacity multiplies the alpha of the texture
   * @param yuv the planes of a video bound on units 0 to 2, or null for an RGBA texture on unit 0
   * @return false if the shaders could not be built (the reason is in the log)
   */
  static bool drawTriangles(const QVector<TexturedVertex>& vertices, const QMatrix4x4& matrix,
                            float opacity, const YuvTexture* yuv);

  /**
   * Returns the matrix from the logical coordinates of a painter to OpenGL's clip
   * coordinates, the same as the one QPainter's OpenGL engine uses.
   *
   * @param deviceTransform the painter's device transform (QPainter::deviceTransform())
   * @param viewportSize the size of the OpenGL viewport, in pixels of the device
   */
  static QMatrix4x4 viewMatrix(const QTransform& deviceTransform, const QSize& viewportSize);

  /// Whether the shaders, YUV included, build in the current OpenGL context.
  static bool isAvailable();

  /// Returns the size of the current OpenGL viewport, in pixels of the device.
  static QSize currentViewportSize();
};

} // namespace mmp

#endif /* SHAPE_RENDERER_H_ */
