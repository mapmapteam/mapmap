/*
 * SyphonImpl.mm
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
 * Objective-C++ glue between MapMap's C++ Syphon source and Apple's Syphon
 * framework. This file is compiled only on macOS (gated in core.pri) and uses
 * manual reference counting (no ARC) so the rest of the build needs no special
 * compiler flags.
 *
 * Data path: Syphon hands us a GL_TEXTURE_RECTANGLE backed by an IOSurface.
 * We blit it, entirely on the GPU, into the source's own GL_TEXTURE_2D via an
 * FBO. MapMap then draws that 2D texture directly, so there is no GPU->CPU
 * readback and no per-frame re-upload (both of which are painfully slow on the
 * legacy-OpenGL-over-Metal path on Apple Silicon).
 */

#include "Syphon.h"

#ifdef HAVE_SYPHON

#include "MM.h"

#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QDebug>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>

#import <Syphon/SyphonServerDirectory.h>
#import <Syphon/SyphonOpenGLClient.h>
#import <Syphon/SyphonOpenGLImage.h>

// Legacy <OpenGL/gl.h> spells the rectangle target with the _ARB suffix.
#ifndef GL_TEXTURE_RECTANGLE
#  ifdef GL_TEXTURE_RECTANGLE_ARB
#    define GL_TEXTURE_RECTANGLE GL_TEXTURE_RECTANGLE_ARB
#  else
#    define GL_TEXTURE_RECTANGLE 0x84F5
#  endif
#endif

namespace mmp {

/**
 * Owns the Syphon client and blits each new frame into a target GL_TEXTURE_2D.
 *
 * All OpenGL work happens in process(), which is only ever called from the
 * render thread while a GL context is current (Texture::update() is invoked
 * during ShapeGraphicsItem painting). We never touch GL from a Syphon callback.
 */
class SyphonImpl
{
public:
  SyphonImpl()
    : _client(nil),
      _width(0),
      _height(0),
      _texW(0),
      _texH(0),
      _needsConnect(false)
  {}

  ~SyphonImpl()
  {
    teardown();
  }

  /// Ask the client to (re)connect on the next process() call.
  void requestConnect() { _needsConnect = true; }

  /**
   * Per-frame entry point. Must be called with a current GL context.
   * @param targetTex MapMap's GL_TEXTURE_2D name to blit the frame into.
   */
  void process(const QString& uuid, const QString& name, const QString& appName,
               GLuint targetTex, bool forceOpaque)
  {
    CGLContextObj cgl = CGLGetCurrentContext();
    if (cgl == NULL || targetTex == 0)
      return; // No GL context / texture yet; try again next frame.

    @autoreleasepool {
      if (_needsConnect)
      {
        teardown();
        _needsConnect = false;
      }

      if (_client == nil)
        tryConnect(uuid, name, appName, cgl);

      if (_client == nil || ![_client isValid])
        return;

      if ([_client hasNewFrame])
      {
        SyphonOpenGLImage* image = [_client newFrameImage];
        if (image != nil)
        {
          blit(image, targetTex, forceOpaque);
          [image release];
        }
      }
    }
  }

  bool isConnected() const { return (_client != nil && [_client isValid]); }

  int width()  const { return _width; }
  int height() const { return _height; }

  /// Enumerates servers currently advertised on the system.
  static QList<SyphonServerDescription> availableServers()
  {
    QList<SyphonServerDescription> result;
    @autoreleasepool {
      NSArray* servers = [[SyphonServerDirectory sharedDirectory] servers];
      for (NSDictionary* desc in servers)
        result.append(describe(desc));
    }
    return result;
  }

private:
  static QString nsToQString(NSString* s)
  {
    return s ? QString::fromUtf8([s UTF8String]) : QString();
  }

  /// Converts a hosting app's NSImage to a small QImage for the picker UI.
  static QImage nsImageToQImage(NSImage* image)
  {
    if (image == nil)
      return QImage();

    CGImageRef cg = [image CGImageForProposedRect:NULL context:nil hints:nil];
    if (cg == NULL)
      return QImage();

    const int w = (int) CGImageGetWidth(cg);
    const int h = (int) CGImageGetHeight(cg);
    if (w <= 0 || h <= 0)
      return QImage();

    QImage out(w, h, QImage::Format_RGBA8888_Premultiplied);
    out.fill(Qt::transparent);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        out.bits(), w, h, 8, out.bytesPerLine(), cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    if (ctx != NULL)
    {
      CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg);
      CGContextRelease(ctx);
    }
    CGColorSpaceRelease(cs);
    return out;
  }

  static SyphonServerDescription describe(NSDictionary* desc)
  {
    SyphonServerDescription out;
    out.uuid    = nsToQString([desc objectForKey:SyphonServerDescriptionUUIDKey]);
    out.name    = nsToQString([desc objectForKey:SyphonServerDescriptionNameKey]);
    out.appName = nsToQString([desc objectForKey:SyphonServerDescriptionAppNameKey]);
    out.icon    = nsImageToQImage([desc objectForKey:SyphonServerDescriptionIconKey]);
    return out;
  }

  /// Finds the server matching our stored identity, preferring the UUID.
  NSDictionary* findServer(const QString& uuid, const QString& name, const QString& appName)
  {
    if (uuid.isEmpty() && name.isEmpty() && appName.isEmpty())
      return nil; // Unbound source: nothing to connect to (yet).

    NSArray* servers = [[SyphonServerDirectory sharedDirectory] servers];

    // First pass: exact UUID match (the only truly unique key).
    if (!uuid.isEmpty())
    {
      for (NSDictionary* desc in servers)
        if (nsToQString([desc objectForKey:SyphonServerDescriptionUUIDKey]) == uuid)
          return desc;
    }

    // Second pass: match by (appName, name). Survives a server restart, which
    // assigns a fresh UUID but keeps the same human-readable identity.
    for (NSDictionary* desc in servers)
    {
      const bool nameOk = name.isEmpty()
          || nsToQString([desc objectForKey:SyphonServerDescriptionNameKey]) == name;
      const bool appOk  = appName.isEmpty()
          || nsToQString([desc objectForKey:SyphonServerDescriptionAppNameKey]) == appName;
      if (nameOk && appOk)
        return desc;
    }

    return nil;
  }

  void tryConnect(const QString& uuid, const QString& name, const QString& appName, CGLContextObj cgl)
  {
    NSDictionary* desc = findServer(uuid, name, appName);
    if (desc == nil)
      return; // Desired server not available right now; retry next frame.

    _client = [[SyphonOpenGLClient alloc] initWithServerDescription:desc
                                                            context:cgl
                                                            options:nil
                                                    newFrameHandler:nil];
  }

  /// Ensures the target GL_TEXTURE_2D has RGBA8 storage at w x h.
  void ensureTargetStorage(GLuint targetTex, int w, int h)
  {
    if (w == _texW && h == _texH)
      return;
    glBindTexture(GL_TEXTURE_2D, targetTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    _texW = w;
    _texH = h;
  }

  /// GPU-only blit of the Syphon rectangle texture into targetTex (a 2D texture).
  void blit(SyphonOpenGLImage* image, GLuint targetTex, bool forceOpaque)
  {
    const GLuint rectTex = [image textureName];
    const NSSize size    = [image textureSize];
    const int w = (int) size.width;
    const int h = (int) size.height;
    if (w <= 0 || h <= 0)
      return;

    // Save the GL state we are about to touch (we run mid-paint).
    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &prevFbo);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    ensureTargetStorage(targetTex, w, h);

    // FBOs are not shared between contexts, so create a throwaway one in the
    // current context each time (cheap; the texture it draws into is shared).
    GLuint fbo = 0;
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, targetTex, 0);

    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
    {
      glViewport(0, 0, w, h);
      glDisable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_TEXTURE_RECTANGLE);
      glBindTexture(GL_TEXTURE_RECTANGLE, rectTex);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

      // Rectangle textures use non-normalized [0..w]x[0..h] coordinates. The
      // V coordinate is flipped relative to the vertices so the 2D texture ends
      // up top-left origin, matching MapMap's sampling (as with the camera path).
      glBegin(GL_QUADS);
        glTexCoord2f(0, h); glVertex2f(0, 0);
        glTexCoord2f(w, h); glVertex2f(w, 0);
        glTexCoord2f(w, 0); glVertex2f(w, h);
        glTexCoord2f(0, 0); glVertex2f(0, h);
      glEnd();

      // Force the destination alpha to 1: Syphon servers frequently publish a
      // zero/garbage alpha, which would make the (GL_SRC_ALPHA-blended) source
      // invisible. RGB is left untouched. Skipped when the user opts to keep the
      // server's own alpha (respectAlpha).
      if (forceOpaque)
      {
        glDisable(GL_TEXTURE_RECTANGLE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
          glVertex2f(0, 0); glVertex2f(w, 0); glVertex2f(w, h); glVertex2f(0, h);
        glEnd();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      _width = w;
      _height = h;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 0, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint) prevFbo);
    glDeleteFramebuffersEXT(1, &fbo);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glPopAttrib();

    static bool logged = false;
    if (!logged)
    {
      logged = true;
      qDebug() << "[Syphon] first frame blitted:" << w << "x" << h
               << "fboStatus=" << status;
    }
  }

  void teardown()
  {
    if (_client != nil)
    {
      [_client stop];
      [_client release];
      _client = nil;
    }
  }

  SyphonOpenGLClient* _client;
  int  _width;        // Last frame size (drives texture coordinates).
  int  _height;
  int  _texW;         // Currently allocated size of the target 2D texture.
  int  _texH;
  bool _needsConnect;
};

// ---------------------------------------------------------------------------
// Syphon source (C++ side). Method bodies live here so the whole source type
// shares a single Objective-C++ translation unit with its implementation.
// ---------------------------------------------------------------------------

Syphon::Syphon(int id)
  : Texture(id),
    _impl(new SyphonImpl())
{
  _impl->requestConnect();
}

Syphon::~Syphon()
{
  delete _impl;
}

void Syphon::build()
{
  if (_impl)
    _impl->requestConnect();
}

void Syphon::update()
{
  Texture::update(); // Allocates our GL_TEXTURE_2D name on first call.
  if (!_impl)
    return;

  _impl->process(_serverUUID, _serverName, _appName, textureId, !_respectAlpha);

  // Announce the real resolution once, so the UI can fit input shapes that were
  // created (at the default size) before any frame had arrived.
  if (!_frameAnnounced && _impl->width() > 0 && _impl->height() > 0)
  {
    _frameAnnounced = true;
    emit frameSizeKnown(getId(), _impl->width(), _impl->height());
  }
}

int Syphon::getWidth() const
{
  const int w = _impl ? _impl->width() : 0;
  return w > 0 ? w : DEFAULT_WIDTH;
}

int Syphon::getHeight() const
{
  const int h = _impl ? _impl->height() : 0;
  return h > 0 ? h : DEFAULT_HEIGHT;
}

// The frame is blitted straight into our GL_TEXTURE_2D, so there is no CPU-side
// bitmap and nothing for MapMap's pipeline to re-upload.
const uchar* Syphon::getBits()        { return nullptr; }
bool         Syphon::bitsHaveChanged() const { return false; }

void Syphon::connectToServer(const QString& uuid, const QString& name, const QString& appName)
{
  _serverUUID = uuid;
  _serverName = name;
  _appName    = appName;
  _reconnect();
}

bool Syphon::isConnected() const
{
  return _impl ? _impl->isConnected() : false;
}

void Syphon::_reconnect()
{
  if (_impl)
    _impl->requestConnect();
}

QList<SyphonServerDescription> Syphon::availableServers()
{
  return SyphonImpl::availableServers();
}

QIcon Syphon::defaultIcon(int size, const QColor& color)
{
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter p(&pixmap);
  p.setRenderHint(QPainter::Antialiasing, true);

  // Rounded square outline.
  QPen pen(color);
  pen.setWidthF(qMax(1.0, size / 16.0));
  p.setPen(pen);
  const qreal m = size * 0.12;
  const qreal r = size * 0.18;
  p.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), r, r);

  // "S" glyph for Syphon.
  QFont font = p.font();
  font.setBold(true);
  font.setPixelSize(int(size * 0.6));
  p.setFont(font);
  p.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("S"));
  p.end();

  return QIcon(pixmap);
}

QIcon Syphon::getIcon() const
{
  return defaultIcon(MM::MAPPING_LIST_ICON_SIZE, QColor(220, 220, 220));
}

}

#endif // HAVE_SYPHON
