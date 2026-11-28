/*
 * IntermediateImage.h
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


#ifndef INTERMEDIATE_IMAGE_H_
#define INTERMEDIATE_IMAGE_H_

#include <QSize>
#include <QVector>
#include <QtGui/qopengl.h>
#include <functional>

namespace mmp {

/**
 * An image drawn on the graphics card, for the sources that compute their image
 * there, such as generators. The shapes then read it like the texture of an image
 * or a video.
 *
 * It is drawn in one OpenGL context and read in others (the views of MapMap share
 * their textures). So that a view never reads an image being drawn:
 * - two textures take turns: one is drawn into while the views read the other;
 * - a fence marks the end of each drawing, and bind() makes the reading context
 *   wait for it on the graphics card. Without fences (OpenGL before 3.2 without
 *   GL_ARB_sync), a flush is all there is.
 *
 * The framebuffer objects that draw are not shared between contexts: one is made
 * in the current context for each drawing. The textures have no multisampling,
 * and have mipmaps, so that thin lines stay smooth when the image is shown smaller.
 */
class IntermediateImage
{
public:
  IntermediateImage();

  /**
   * Draws a new image: (re)allocates the storage of the texture drawn into when the
   * size changed, then calls draw with that texture as the target, the viewport
   * covering it, the target cleared to transparent black, and blending, scissor,
   * depth and stencil tests off. Then makes its mipmaps, marks its end with a fence,
   * and gives back the framebuffer, the viewport and the tests of the caller. The
   * new image is the one bind() binds from then on.
   *
   * @param size the size of the image, in pixels
   * @param draw draws the image
   * @return false, with a warning, when no context is current or the target cannot be drawn into
   */
  bool render(const QSize& size, const std::function<void()>& draw);

  /**
   * Binds the last image drawn to texture unit 0, after making the current context
   * wait, on the graphics card, for the end of its drawing. Binds nothing before
   * the first image is drawn.
   */
  void bind();

  /// Returns the texture of the last image drawn, 0 before the first one.
  GLuint texture() const { return _drawn ? _textures[_front] : 0; }

  /// Returns the size of the last image drawn.
  QSize size() const { return _drawn ? _sizes[_front] : QSize(); }

  /// Returns the textures made so far, so that their owner can free them.
  QVector<GLuint> textureIds() const;

private:
  /// Number of textures taking turns.
  static const int N_TEXTURES = 2;

  GLuint _textures[N_TEXTURES];
  QSize _sizes[N_TEXTURES];

  /// The texture of the last image drawn, and whether one was drawn.
  int _front;
  bool _drawn;

  /// Marks the end of the last drawing; null without fences. The fence left when the
  /// image is destroyed is not deleted: no context is current then.
  GLsync _fence;
};

} // namespace mmp

#endif /* INTERMEDIATE_IMAGE_H_ */
