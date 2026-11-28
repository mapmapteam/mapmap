/*
 * IntermediateImage.cpp
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


#include "IntermediateImage.h"

#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>

namespace mmp {

// Anisotropic filtering (GL_EXT_texture_filter_anisotropic), missing from some headers.
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace {

/// Most samples taken along a line seen in perspective; more costs time for little.
const GLfloat MAX_ANISOTROPY = 8.0f;

/// OpenGL version from which fences are part of desktop OpenGL, and of OpenGL ES.
const int FENCE_DESKTOP_MAJOR = 3;
const int FENCE_DESKTOP_MINOR = 2;
const int FENCE_ES_MAJOR = 3;

/// Returns true when the current context has fences (glFenceSync, glWaitSync).
bool hasFences(QOpenGLContext* context)
{
  const QPair<int, int> version = context->format().version();
  if (context->isOpenGLES())
    return version.first >= FENCE_ES_MAJOR;
  return version >= qMakePair(FENCE_DESKTOP_MAJOR, FENCE_DESKTOP_MINOR) ||
         context->hasExtension(QByteArrayLiteral("GL_ARB_sync"));
}

/// Saves the OpenGL state that render() changes, and gives it back when destroyed.
class SavedState
{
public:
  explicit SavedState(QOpenGLFunctions* gl) : _gl(gl)
  {
    _gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_framebuffer);
    _gl->glGetIntegerv(GL_VIEWPORT, _viewport);
    _gl->glGetFloatv(GL_COLOR_CLEAR_VALUE, _clearColor);
    for (int i = 0; i < N_CAPABILITIES; i++)
      _enabled[i] = _gl->glIsEnabled(CAPABILITIES[i]);
  }

  ~SavedState()
  {
    _gl->glBindFramebuffer(GL_FRAMEBUFFER, GLuint(_framebuffer));
    _gl->glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
    _gl->glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    for (int i = 0; i < N_CAPABILITIES; i++)
      setEnabled(CAPABILITIES[i], _enabled[i]);
  }

  /// Turns off the tests and the blending that would change what is drawn.
  void disableCapabilities()
  {
    for (int i = 0; i < N_CAPABILITIES; i++)
      setEnabled(CAPABILITIES[i], false);
  }

private:
  /// The tests and the blending of the caller, turned off while drawing.
  static constexpr int N_CAPABILITIES = 4;
  static constexpr GLenum CAPABILITIES[N_CAPABILITIES] = {
    GL_BLEND, GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_STENCIL_TEST
  };

  /// Number of values of a viewport (x, y, width, height) and of a colour (r, g, b, a).
  static constexpr int N_VIEWPORT_VALUES = 4;
  static constexpr int N_COLOR_VALUES = 4;

  void setEnabled(GLenum capability, bool enabled)
  {
    if (enabled)
      _gl->glEnable(capability);
    else
      _gl->glDisable(capability);
  }

  QOpenGLFunctions* _gl;
  GLint _framebuffer = 0;
  GLint _viewport[N_VIEWPORT_VALUES] = { 0, 0, 0, 0 };
  GLfloat _clearColor[N_COLOR_VALUES] = { 0, 0, 0, 0 };
  GLboolean _enabled[N_CAPABILITIES] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
};

} // namespace

IntermediateImage::IntermediateImage()
  : _textures { 0, 0 },
    _front(0),
    _drawn(false),
    _fence(nullptr)
{
}

QVector<GLuint> IntermediateImage::textureIds() const
{
  QVector<GLuint> ids;
  for (GLuint id : _textures)
    if (id != 0)
      ids.append(id);
  return ids;
}

bool IntermediateImage::render(const QSize& size, const std::function<void()>& draw)
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context || size.isEmpty())
  {
    qWarning() << "An intermediate image cannot be drawn without a context and a size.";
    return false;
  }
  QOpenGLFunctions* gl = context->functions();

  // Drawn into the texture the views are not reading.
  const int target = _drawn ? (_front + 1) % N_TEXTURES : _front;
  GLuint& texture = _textures[target];
  if (texture == 0)
    gl->glGenTextures(1, &texture);

  // Storage, without multisampling, made again only when the size changes.
  gl->glBindTexture(GL_TEXTURE_2D, texture);
  if (size != _sizes[target])
  {
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    _sizes[target] = size;
  }
  // Not bound while it is the target: reading it would be undefined.
  gl->glBindTexture(GL_TEXTURE_2D, 0);

  bool complete = false;
  {
    SavedState saved(gl);

    // Framebuffer objects are not shared between contexts: one for this drawing.
    GLuint framebuffer = 0;
    gl->glGenFramebuffers(1, &framebuffer);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    complete = (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if (complete)
    {
      gl->glViewport(0, 0, size.width(), size.height());
      saved.disableCapabilities();
      gl->glClearColor(0, 0, 0, 0);
      gl->glClear(GL_COLOR_BUFFER_BIT);
      draw();
    }
    else
      qWarning() << "An intermediate image of" << size << "cannot be drawn into on this graphics card.";

    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glDeleteFramebuffers(1, &framebuffer);
  }
  if (!complete)
    return false;

  // Smaller copies, read when the image is shown smaller.
  gl->glBindTexture(GL_TEXTURE_2D, texture);
  gl->glGenerateMipmap(GL_TEXTURE_2D);
  gl->glBindTexture(GL_TEXTURE_2D, 0);

  // Marks the end of the drawing, for the other contexts (see bind()).
  if (hasFences(context))
  {
    QOpenGLExtraFunctions* extra = context->extraFunctions();
    if (_fence)
      extra->glDeleteSync(_fence);
    _fence = extra->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }
  // Sends the commands now, so that the fence can be reached.
  gl->glFlush();

  _front = target;
  _drawn = true;
  return true;
}

void IntermediateImage::bind()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context || !_drawn)
    return;
  QOpenGLFunctions* gl = context->functions();

  // Waits on the graphics card, not on the processor, for the end of the drawing.
  if (_fence && hasFences(context))
    context->extraFunctions()->glWaitSync(_fence, 0, GL_TIMEOUT_IGNORED);

  gl->glBindTexture(GL_TEXTURE_2D, _textures[_front]);
  gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Keeps lines sharp in shapes seen in perspective, where the image is shrunk more
  // in one direction than in the other.
  if (context->hasExtension(QByteArrayLiteral("GL_EXT_texture_filter_anisotropic")) ||
      context->hasExtension(QByteArrayLiteral("GL_ARB_texture_filter_anisotropic")))
  {
    GLfloat supported = 1.0f;
    gl->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &supported);
    gl->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, qMin(supported, MAX_ANISOTROPY));
  }
}

} // namespace mmp
