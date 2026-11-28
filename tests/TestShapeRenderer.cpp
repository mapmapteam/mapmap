/*
 * TestShapeRenderer.cpp
 *
 * Unit tests for the drawing of shapes with shaders (src/core/ShapeRenderer).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestShapeRenderer.h"
#include "ShapeRenderer.h"

#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <memory>

using namespace mmp;

namespace {

/// Side of the image drawn into, in pixels.
const int SIZE = 64;

/// Largest difference allowed on a color channel, out of 255.
const int TOLERANCE = 2;

/// Largest difference allowed on a coordinate.
const float EPSILON = 1e-5f;

/// The colors of the 2 × 2 texture, and where their middle lands in the drawn image.
struct Texel
{
  const char* name;
  int x;
  int y;
  QColor color;
};
const Texel TEXELS[] = {
  { "top-left (red)",       SIZE / 4,     SIZE / 4,     QColor(255, 0, 0) },
  { "top-right (green)",    3 * SIZE / 4, SIZE / 4,     QColor(0, 255, 0) },
  { "bottom-left (blue)",   SIZE / 4,     3 * SIZE / 4, QColor(0, 0, 255) },
  { "bottom-right (white)", 3 * SIZE / 4, 3 * SIZE / 4, QColor(255, 255, 255) },
};

/// An OpenGL context made current on an offscreen surface for the life of the object.
struct OffscreenContext
{
  QOffscreenSurface surface;
  QOpenGLContext context;
  bool ok = false;

  OffscreenContext()
  {
    surface.create();
    ok = context.create() && context.makeCurrent(&surface) && !context.isOpenGLES();
  }
  ~OffscreenContext() { context.doneCurrent(); }
};

/// Uploads the 2 × 2 texture of TEXELS to texture unit 0 and returns its id.
GLuint makeTexture()
{
  QImage image(2, 2, QImage::Format_RGBA8888);
  for (const Texel& texel : TEXELS)
    image.setPixelColor(texel.x * 2 / SIZE, texel.y * 2 / SIZE, texel.color);

  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               image.constBits());
  return id;
}

/// Draws @a vertices over opaque black, blended as MapMap blends, and returns the image
/// with the values of the framebuffer (premultiplied), or a null image if nothing was drawn.
QImage draw(const QVector<TexturedVertex>& vertices, float opacity)
{
  QOpenGLFramebufferObject fbo(SIZE, SIZE);
  fbo.bind();
  glViewport(0, 0, SIZE, SIZE);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLuint texture = makeTexture();
  const QMatrix4x4 matrix = ShapeRenderer::viewMatrix(QTransform(), QSize(SIZE, SIZE));
  const bool drawn = ShapeRenderer::drawTriangles(vertices, matrix, opacity, nullptr);
  glDeleteTextures(1, &texture);
  glDisable(GL_BLEND);

  QImage image = drawn ? fbo.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied) : QImage();
  fbo.release();
  return image;
}

bool closeChannel(int a, int b)
{
  return qAbs(a - b) <= TOLERANCE;
}

/// Checks that each texel shows at its place, scaled by @a factor (opacity over black).
void checkTexels(const QImage& image, float factor)
{
  QVERIFY2(!image.isNull(), "The shaders do not build");
  for (const Texel& texel : TEXELS)
  {
    const QRgb got = image.pixel(texel.x, texel.y);
    const QColor expected(qRound(texel.color.red() * factor), qRound(texel.color.green() * factor),
                          qRound(texel.color.blue() * factor));
    QVERIFY2(closeChannel(qRed(got), expected.red()) && closeChannel(qGreen(got), expected.green()) &&
             closeChannel(qBlue(got), expected.blue()),
             qPrintable(QString("%1 is (%2, %3, %4), expected (%5, %6, %7)")
                        .arg(texel.name).arg(qRed(got)).arg(qGreen(got)).arg(qBlue(got))
                        .arg(expected.red()).arg(expected.green()).arg(expected.blue())));
  }
}

/// Maps a point with a 4 × 4 matrix and returns its normalized device coordinates.
QPointF toNdc(const QMatrix4x4& matrix, const QPointF& point)
{
  const QVector4D clip = matrix * QVector4D(point.x(), point.y(), 0, 1);
  return QPointF(clip.x() / clip.w(), clip.y() / clip.w());
}

bool near(const QPointF& a, const QPointF& b)
{
  return qAbs(a.x() - b.x()) < EPSILON && qAbs(a.y() - b.y()) < EPSILON;
}

} // namespace

void TestShapeRenderer::viewMatrixMapsViewportCorners()
{
  const QMatrix4x4 matrix = ShapeRenderer::viewMatrix(QTransform(), QSize(800, 600));
  // y goes down in the viewport and up in OpenGL.
  QVERIFY(near(toNdc(matrix, QPointF(0, 0)), QPointF(-1, 1)));
  QVERIFY(near(toNdc(matrix, QPointF(800, 600)), QPointF(1, -1)));
  QVERIFY(near(toNdc(matrix, QPointF(400, 300)), QPointF(0, 0)));
}

void TestShapeRenderer::viewMatrixAppliesDeviceTransform()
{
  // A view zoomed twice and scrolled by (100, 50).
  QTransform transform;
  transform.translate(-100, -50);
  transform.scale(2, 2);
  const QMatrix4x4 matrix = ShapeRenderer::viewMatrix(transform, QSize(800, 600));
  QVERIFY(near(toNdc(matrix, QPointF(50, 25)), QPointF(-1, 1)));
  QVERIFY(near(toNdc(matrix, QPointF(450, 325)), QPointF(1, -1)));
}

void TestShapeRenderer::drawingNeedsAContext()
{
  if (QOpenGLContext::currentContext())
    QSKIP("A context is current");
  QVERIFY(!ShapeRenderer::drawTriangles(ShapeGeometry::rectangle(QRectF(0, 0, SIZE, SIZE)), QMatrix4x4(),
                                        1.0f, nullptr));
}

void TestShapeRenderer::drawsRgbaTexture()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");
  checkTexels(draw(ShapeGeometry::rectangle(QRectF(0, 0, SIZE, SIZE)), 1.0f), 1.0f);
  QCOMPARE(glGetError(), GLenum(GL_NO_ERROR));
}

void TestShapeRenderer::opacityScalesAlpha()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");
  const float opacity = 0.5f;
  checkTexels(draw(ShapeGeometry::rectangle(QRectF(0, 0, SIZE, SIZE)), opacity), opacity);
}

void TestShapeRenderer::textureCoordinateIsDividedByQ()
{
  OffscreenContext gl;
  if (!gl.ok)
    QSKIP("No desktop OpenGL context available");

  // The same rectangle with every texture coordinate multiplied by q: (s/q, t/q) is unchanged.
  const float q = 2.0f;
  QVector<TexturedVertex> vertices = ShapeGeometry::rectangle(QRectF(0, 0, SIZE, SIZE));
  for (TexturedVertex& v : vertices)
  {
    v.s *= q;
    v.t *= q;
    v.q *= q;
  }
  checkTexels(draw(vertices, 1.0f), 1.0f);
}
