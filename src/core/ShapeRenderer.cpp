/*
 * ShapeRenderer.cpp
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

#include "ShapeRenderer.h"

#include "YuvTexture.h"

#include <QDebug>
#include <QHash>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <cstddef>

namespace mmp {

namespace {

/// Attribute locations, fixed before linking so that both programs share them.
const int POSITION_LOCATION  = 0;
const int TEX_COORD_LOCATION = 1;

/// Number of components of a position (x, y) and of a texture coordinate (s, t, q).
const int POSITION_SIZE  = 2;
const int TEX_COORD_SIZE = 3;

/// Texture units of the planes of a video (see YuvTexture::bind()).
const int Y_UNIT = 0;
const int U_UNIT = 1;
const int V_UNIT = 2;

/// Texture unit of an RGBA texture (see Texture::bind()).
const int RGBA_UNIT = 0;

/// Near and far planes of the orthographic projection: shapes are flat, at depth 0.
const float NEAR_PLANE = -1.0f;
const float FAR_PLANE  = 1.0f;

const char* VERTEX_SHADER =
  "#version 120\n"
  "attribute vec2 position;\n"
  "attribute vec3 texCoord;\n"
  "uniform mat4 matrix;\n"
  "varying vec3 vTexCoord;\n"
  "void main() {\n"
  "  gl_Position = matrix * vec4(position, 0.0, 1.0);\n"
  "  vTexCoord = texCoord;\n"
  "}\n";

// The texture is read at (s/q, t/q): dividing per pixel gives an exact perspective.
const char* RGBA_FRAGMENT_SHADER =
  "#version 120\n"
  "uniform sampler2D tex;\n"
  "uniform float opacity;\n"
  "varying vec3 vTexCoord;\n"
  "void main() {\n"
  "  vec4 color = texture2D(tex, vTexCoord.xy / vTexCoord.z);\n"
  "  gl_FragColor = vec4(color.rgb, color.a * opacity);\n"
  "}\n";

// Single-channel planes are GL_LUMINANCE (value in .r); interleaved chroma is
// GL_LUMINANCE_ALPHA (first byte in .r, second in .a). See YuvTexture.
const char* YUV_FRAGMENT_SHADER =
  "#version 120\n"
  "uniform sampler2D texY;\n"
  "uniform sampler2D texU;\n"
  "uniform sampler2D texV;\n"
  "uniform int chromaLayout;\n"
  "uniform mat4 colorMatrix;\n"
  "uniform float opacity;\n"
  "varying vec3 vTexCoord;\n"
  "void main() {\n"
  "  vec2 tc = vTexCoord.xy / vTexCoord.z;\n"
  "  float y = texture2D(texY, tc).r;\n"
  "  vec2 uv;\n"
  "  if (chromaLayout == 0) {\n"
  "    uv = vec2(texture2D(texU, tc).r, texture2D(texV, tc).r);\n"
  "  } else {\n"
  "    vec4 c = texture2D(texU, tc);\n"
  "    uv = (chromaLayout == 1) ? c.ra : c.ar;\n"
  "  }\n"
  "  vec3 rgb = (colorMatrix * vec4(y, uv, 1.0)).rgb;\n"
  "  gl_FragColor = vec4(clamp(rgb, 0.0, 1.0), opacity);\n"
  "}\n";

/// The OpenGL objects of a group of shared contexts.
struct GroupResources
{
  QOpenGLShaderProgram* rgba = nullptr;
  QOpenGLShaderProgram* yuv = nullptr;
  QOpenGLBuffer* vertexBuffer = nullptr;
  bool failed = false;
};

/// Builds a program from the vertex shader and @a fragmentShader, or returns null.
QOpenGLShaderProgram* buildProgram(const char* fragmentShader, const char* name)
{
  QOpenGLShaderProgram* program = new QOpenGLShaderProgram;
  program->bindAttributeLocation("position", POSITION_LOCATION);
  program->bindAttributeLocation("texCoord", TEX_COORD_LOCATION);
  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER) ||
      !program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader) ||
      !program->link())
  {
    qWarning() << "Shapes cannot be drawn: the" << name << "shader does not build:" << program->log();
    delete program;
    return nullptr;
  }
  return program;
}

/// Returns the resources of the current context's group, made on first use, or null.
GroupResources* currentResources()
{
  static QHash<QOpenGLContextGroup*, GroupResources> resources;

  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context)
    return nullptr;
  QOpenGLContextGroup* group = context->shareGroup();

  auto found = resources.find(group);
  if (found != resources.end())
    return found->failed ? nullptr : &found.value();

  GroupResources made;
  made.rgba = buildProgram(RGBA_FRAGMENT_SHADER, "RGBA");
  made.yuv  = buildProgram(YUV_FRAGMENT_SHADER, "YUV");
  made.vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  made.vertexBuffer->setUsagePattern(QOpenGLBuffer::StreamDraw);
  made.failed = !made.rgba || !made.yuv || !made.vertexBuffer->create();

  if (!made.failed)
  {
    made.rgba->bind();
    made.rgba->setUniformValue("tex", RGBA_UNIT);
    made.yuv->bind();
    made.yuv->setUniformValue("texY", Y_UNIT);
    made.yuv->setUniformValue("texU", U_UNIT);
    made.yuv->setUniformValue("texV", V_UNIT);
    made.yuv->release();
  }

  resources.insert(group, made);
  QObject::connect(group, &QObject::destroyed, [group]() {
    // The OpenGL objects die with the group; just forget the wrappers.
    resources.remove(group);
  });
  return made.failed ? nullptr : &resources[group];
}

} // namespace

bool ShapeRenderer::drawTriangles(const QVector<TexturedVertex>& vertices, const QMatrix4x4& matrix,
                                  float opacity, const YuvTexture* yuv)
{
  GroupResources* resources = currentResources();
  if (!resources)
    return false;
  if (vertices.isEmpty())
    return true;

  QOpenGLShaderProgram* program = yuv ? resources->yuv : resources->rgba;
  program->bind();
  program->setUniformValue("matrix", matrix);
  program->setUniformValue("opacity", opacity);
  if (yuv)
  {
    program->setUniformValue("chromaLayout", yuv->chromaLayout());
    program->setUniformValue("colorMatrix", yuv->conversionMatrix());
  }

  QOpenGLBuffer* buffer = resources->vertexBuffer;
  buffer->bind();
  buffer->allocate(vertices.constData(), int(vertices.size() * sizeof(TexturedVertex)));

  program->enableAttributeArray(POSITION_LOCATION);
  program->enableAttributeArray(TEX_COORD_LOCATION);
  program->setAttributeBuffer(POSITION_LOCATION, GL_FLOAT, offsetof(TexturedVertex, x),
                              POSITION_SIZE, sizeof(TexturedVertex));
  program->setAttributeBuffer(TEX_COORD_LOCATION, GL_FLOAT, offsetof(TexturedVertex, s),
                              TEX_COORD_SIZE, sizeof(TexturedVertex));

  QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

  program->disableAttributeArray(TEX_COORD_LOCATION);
  program->disableAttributeArray(POSITION_LOCATION);
  buffer->release();
  program->release();
  return true;
}

QMatrix4x4 ShapeRenderer::viewMatrix(const QTransform& deviceTransform, const QSize& viewportSize)
{
  // Device pixels, y down, to clip coordinates.
  QMatrix4x4 matrix;
  matrix.ortho(0, viewportSize.width(), viewportSize.height(), 0, NEAR_PLANE, FAR_PLANE);
  return matrix * QMatrix4x4(deviceTransform);
}

QSize ShapeRenderer::currentViewportSize()
{
  // x, y, width, height.
  GLint viewport[4] = { 0, 0, 0, 0 };
  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (context)
    context->functions()->glGetIntegerv(GL_VIEWPORT, viewport);
  return QSize(viewport[2], viewport[3]);
}

} // namespace mmp
