/*
 * SyphonServerImpl.mm
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
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
 *
 * Objective-C++ glue publishing MapMap's output composition as a Syphon
 * server. Compiled only on macOS (gated in core.pri); manual reference
 * counting (no ARC).
 *
 * Each frame we MSAA-resolve/blit the output canvas's framebuffer into the
 * server's own FBO (bindToDrawFrameOfSize/unbindAndPublish), so the work is
 * entirely on the GPU.
 */

#include "SyphonOutput.h"

#ifdef HAVE_SYPHON

#include <QDebug>

#import <Foundation/Foundation.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>

#import <Syphon/SyphonOpenGLServer.h>

namespace mmp {

static NSString* qStringToNS(const QString& s)
{
  return [NSString stringWithUTF8String:(s.isEmpty() ? "MapMap" : s.toUtf8().constData())];
}

class SyphonServerImpl
{
public:
  SyphonServerImpl() : _server(nil), _serverCtx(NULL) {}

  ~SyphonServerImpl() { teardown(); }

  void teardown()
  {
    if (_server != nil)
    {
      [_server stop];
      [_server release];
      _server = nil;
    }
    _serverCtx = NULL;
  }

  void setName(const QString& name)
  {
    @autoreleasepool {
      if (_server != nil)
        _server.name = qStringToNS(name);
    }
  }

  void publish(GLuint sourceFbo, int w, int h, const QString& name)
  {
    if (w <= 0 || h <= 0)
      return;

    CGLContextObj cgl = CGLGetCurrentContext();
    if (cgl == NULL)
      return;

    @autoreleasepool {
      // (Re)create the server if needed, or if the GL context changed.
      if (_server != nil && _serverCtx != cgl)
        teardown();

      if (_server == nil)
      {
        _server = [[SyphonOpenGLServer alloc] initWithName:qStringToNS(name)
                                                   context:cgl
                                                   options:nil];
        if (_server == nil)
          return;
        _serverCtx = cgl;
      }

      // Bind the server's FBO and blit our composition into it. A same-size
      // blit from the (multisampled) widget FBO resolves MSAA in one step.
      if ([_server bindToDrawFrameOfSize:NSMakeSize(w, h)])
      {
        GLint prevRead = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &prevRead);

        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, sourceFbo);
        glBlitFramebufferEXT(0, 0, w, h, 0, 0, w, h,
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, (GLuint) prevRead);

        [_server unbindAndPublish]; // restores the previously-bound FBO + flushes
      }
    }
  }

private:
  SyphonOpenGLServer* _server;
  CGLContextObj       _serverCtx; // context the server was created with
};

// ---------------------------------------------------------------------------
// SyphonOutput (C++ side).
// ---------------------------------------------------------------------------

SyphonOutput::SyphonOutput()
  : _enabled(false),
    _serverName(QStringLiteral("MapMap")),
    _impl(new SyphonServerImpl())
{
}

SyphonOutput::~SyphonOutput()
{
  delete _impl;
}

void SyphonOutput::setEnabled(bool on)
{
  if (on == _enabled)
    return;
  _enabled = on;
  // The server is created lazily on the next publish (needs a GL context); when
  // disabling, tear it down so it disappears from the Syphon directory.
  if (!on && _impl)
    _impl->teardown();
}

void SyphonOutput::setServerName(const QString& name)
{
  _serverName = name.isEmpty() ? QStringLiteral("MapMap") : name;
  if (_impl)
    _impl->setName(_serverName);
}

void SyphonOutput::publishCurrentFramebuffer()
{
  if (!_enabled || !_impl)
    return;

  GLint fbo = 0;
  GLint viewport[4] = { 0, 0, 0, 0 };
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo);
  glGetIntegerv(GL_VIEWPORT, viewport);

  _impl->publish((GLuint) fbo, viewport[2], viewport[3], _serverName);
}

}

#endif // HAVE_SYPHON
