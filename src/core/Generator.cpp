/*
 * Generator.cpp
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


#include "Generator.h"

#include "FrameCounter.h"

#include <QDebug>
#include <QHash>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

namespace mmp {

namespace {

/// Milliseconds in a second.
const double MS_PER_SECOND = 1000.0;

/// Attribute location of the position of a vertex.
const int POSITION_LOCATION = 0;

/// Number of components of a position (x, y).
const int POSITION_SIZE = 2;

/// Two triangles covering the whole target, in clip coordinates.
const GLfloat FULL_TARGET[] = {
  -1, -1,   1, -1,   1, 1,
  -1, -1,   1,  1,  -1, 1,
};

/// Number of vertices of FULL_TARGET.
const int N_FULL_TARGET_VERTICES = int(sizeof(FULL_TARGET) / (POSITION_SIZE * sizeof(GLfloat)));

const char* VERTEX_SHADER =
  "#version 120\n"
  "attribute vec2 position;\n"
  "varying vec2 isf_FragNormCoord;\n"
  "void main() {\n"
  "  isf_FragNormCoord = position * 0.5 + 0.5;\n"
  "  gl_Position = vec4(position, 0.0, 1.0);\n"
  "}\n";

const char* FRAGMENT_HEADER =
  "#version 120\n"
  "uniform float TIME;\n"
  "uniform vec2 RENDERSIZE;\n"
  "varying vec2 isf_FragNormCoord;\n";

/// The OpenGL objects of the generators in a group of shared contexts.
struct GroupResources
{
  /// Programs by kind of generator; null when the shader did not build.
  QHash<QString, QOpenGLShaderProgram*> programs;
  QOpenGLBuffer* fullTarget = nullptr;
};

/// Returns the resources of the current context's group, made on first use.
GroupResources* currentResources()
{
  static QHash<QOpenGLContextGroup*, GroupResources> resources;

  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context)
    return nullptr;
  QOpenGLContextGroup* group = context->shareGroup();

  auto found = resources.find(group);
  if (found != resources.end())
    return &found.value();

  GroupResources& made = resources[group];
  made.fullTarget = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  if (made.fullTarget->create())
  {
    made.fullTarget->bind();
    made.fullTarget->allocate(FULL_TARGET, int(sizeof(FULL_TARGET)));
    made.fullTarget->release();
  }
  QObject::connect(group, &QObject::destroyed, [group]() {
    // The OpenGL objects die with the group; just forget the wrappers.
    resources.remove(group);
  });
  return &made;
}

/// Returns the GLSL type of a setting passed to a shader, or an empty text when it cannot be.
QString glslType(SettingInfo::Type type)
{
  switch (type) {
  case SettingInfo::Number:  return QStringLiteral("float");
  case SettingInfo::Integer: return QStringLiteral("int");
  case SettingInfo::Boolean: return QStringLiteral("bool");
  case SettingInfo::Color:   return QStringLiteral("vec4");
  default:                   return QString();
  }
}

} // namespace

Generator::Generator(uid id, int defaultSize)
  : Texture(id),
    _width(defaultSize),
    _height(defaultSize),
    _changed(true),
    _renderedFrame(0),
    _renderCount(0),
    _playedSeconds(0)
{
  SettingGroup* root = settings();
  root->addSetting(SettingInfo::integer("width", tr("Width"))
                   .bounds(1, MAX_SIZE)
                   .defaultValue(defaultSize)
                   .unit(SettingInfo::Pixels)
                   .displayRow("resolution")
                   .description(tr("The width of the generated image.")))
      ->bind([this]() { return QVariant(_width); },
             [this](const QVariant& value) { setSize(value.toInt(), _height); return true; });
  root->addSetting(SettingInfo::integer("height", tr("Height"))
                   .bounds(1, MAX_SIZE)
                   .defaultValue(defaultSize)
                   .unit(SettingInfo::Pixels)
                   .displayRow("resolution")
                   .description(tr("The height of the generated image.")))
      ->bind([this]() { return QVariant(_height); },
             [this](const QVariant& value) { setSize(_width, value.toInt()); return true; });

  // Any change of setting may change the image.
  connect(this, &Element::settingChanged, this, [this](uid, const QString& path) {
    _changed = true;
    if (path == QLatin1String("name"))
      _frameStats.setName(getName());
  });
}

Generator::~Generator()
{
  // No context is current here: the textures are freed later.
  for (GLuint id : _image.textureIds())
    orphanTexture(id);
}

void Generator::bind()
{
  _image.bind();
}

void Generator::setSize(int width, int height)
{
  width  = qBound(1, width, int(MAX_SIZE));
  height = qBound(1, height, int(MAX_SIZE));
  if (width == _width && height == _height)
    return;

  const int oldWidth = _width;
  const int oldHeight = _height;
  _width = width;
  _height = height;
  _changed = true;
  if (width != oldWidth)
    _emitPropertyChanged("width");
  if (height != oldHeight)
    _emitPropertyChanged("height");
  emit sizeChanged(int(getId()), oldWidth, oldHeight, width, height);
}

void Generator::update()
{
  // Drawn by the first view that paints a frame; the others read it.
  const quint64 frame = FrameCounter::current();
  const bool due = _changed || _renderCount == 0 ||
                   (isAnimated() && isPlaying() && frame != _renderedFrame);
  if (!due)
    return;

  if (_renderCount == 0)
  {
    _frameStats.setName(getName());
    _frameStats.setPath(tr("%1 generator, drawn on the GPU").arg(generatorName()));
  }

  const bool measuring = FrameStats::isEnabled();
  const qint64 startNs = measuring ? FrameStats::now() : 0;

  _changed = false;
  _renderedFrame = frame;
  if (_image.render(QSize(_width, _height), [this]() { _render(); }))
    _renderCount++;

  if (measuring)
  {
    // OpenGL works asynchronously: without glFinish() this only times the
    // sending of the commands, not the drawing itself.
    if (FrameStats::isSynchronous())
      QOpenGLContext::currentContext()->functions()->glFinish();
    const qint64 endNs = FrameStats::now();
    _frameStats.record(FrameStats::RENDER, endNs - startNs, endNs);
  }
}

void Generator::rewind()
{
  _playedSeconds = 0;
  if (_playTimer.isValid())
    _playTimer.restart();
  _changed = true;
}

double Generator::time() const
{
  return _playedSeconds + (_playTimer.isValid() ? _playTimer.elapsed() / MS_PER_SECOND : 0);
}

void Generator::_doPlay()
{
  if (!_playTimer.isValid())
    _playTimer.start();
}

void Generator::_doPause()
{
  if (_playTimer.isValid())
  {
    _playedSeconds += _playTimer.elapsed() / MS_PER_SECOND;
    _playTimer.invalidate();
  }
}

ShaderGenerator::ShaderGenerator(uid id, int defaultSize)
  : Generator(id, defaultSize)
{
}

void ShaderGenerator::_addShaderInput(const QString& id)
{
  Setting* setting = settings()->find(id);
  if (!setting || glslType(setting->info().type()).isEmpty())
  {
    qWarning() << "The setting" << id << "cannot be an input of a shader.";
    return;
  }
  _shaderInputs.append(id);
}

QOpenGLShaderProgram* ShaderGenerator::_program()
{
  GroupResources* resources = currentResources();
  if (!resources)
    return nullptr;

  // One program per kind of generator.
  const QString kind = QString::fromLatin1(metaObject()->className());
  auto found = resources->programs.constFind(kind);
  if (found != resources->programs.constEnd())
    return found.value();

  QString fragment = QString::fromLatin1(FRAGMENT_HEADER);
  for (const QString& id : std::as_const(_shaderInputs))
    fragment += QStringLiteral("uniform %1 %2;\n").arg(glslType(settings()->find(id)->info().type()), id);
  fragment += _extraUniforms();
  fragment += _fragmentShader();

  QOpenGLShaderProgram* program = new QOpenGLShaderProgram;
  program->bindAttributeLocation("position", POSITION_LOCATION);
  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER) ||
      !program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment) ||
      !program->link())
  {
    qWarning() << "The generator" << generatorName() << "cannot be drawn: its shader does not build:"
               << program->log();
    delete program;
    program = nullptr;
  }
  resources->programs.insert(kind, program);
  return program;
}

void ShaderGenerator::_render()
{
  QOpenGLShaderProgram* program = _program();
  GroupResources* resources = currentResources();
  if (!program || !resources || !resources->fullTarget->isCreated())
    return;

  program->bind();
  program->setUniformValue("TIME", GLfloat(time()));
  program->setUniformValue("RENDERSIZE", QSizeF(getWidth(), getHeight()));
  for (const QString& id : std::as_const(_shaderInputs))
  {
    const Setting* setting = settings()->find(id);
    const QVariant value = setting->effectiveValue();
    const QByteArray name = id.toLatin1();
    switch (setting->info().type()) {
    case SettingInfo::Number:  program->setUniformValue(name.constData(), GLfloat(value.toDouble())); break;
    case SettingInfo::Integer: program->setUniformValue(name.constData(), GLint(value.toInt())); break;
    case SettingInfo::Boolean: program->setUniformValue(name.constData(), GLint(value.toBool())); break;
    case SettingInfo::Color:   program->setUniformValue(name.constData(), value.value<QColor>()); break;
    default: break;
    }
  }
  _setExtraUniforms(program);

  resources->fullTarget->bind();
  program->enableAttributeArray(POSITION_LOCATION);
  program->setAttributeBuffer(POSITION_LOCATION, GL_FLOAT, 0, POSITION_SIZE);
  QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, N_FULL_TARGET_VERTICES);
  program->disableAttributeArray(POSITION_LOCATION);
  resources->fullTarget->release();
  program->release();
}

} // namespace mmp
