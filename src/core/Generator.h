/*
 * Generator.h
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


#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <QElapsedTimer>
#include <QStringList>

#include "FrameStats.h"
#include "IntermediateImage.h"
#include "Source.h"

class QOpenGLShaderProgram;

namespace mmp {

/**
 * A source that computes its image on the graphics card, in an intermediate image,
 * instead of reading a file.
 *
 * The image is drawn by the first view that paints a frame and only read by the
 * others (see FrameCounter): when a setting changes, and once per frame while an
 * animated generator plays. Its size is set by the settings width and height.
 */
class Generator : public Texture
{
  Q_OBJECT

public:
  /// Size of a new generator's image, in pixels.
  static const int DEFAULT_SIZE = 1024;

  /// Largest side of the image, in pixels: the texture size every graphics card accepts.
  static const int MAX_SIZE = 8192;

  /// Frees the textures of the image once a context is current (see orphanTexture()).
  virtual ~Generator();

  /// Returns the translated name of the kind of generator, such as "Checker".
  virtual QString generatorName() const = 0;

  /// Whether the image changes with time; a still generator draws only when a setting changes.
  virtual bool isAnimated() const { return false; }

  virtual SourceType getSourceType() const { return SourceType::Generator; }
  virtual int getWidth() const { return _width; }
  virtual int getHeight() const { return _height; }

  /// Sets the size of the image, kept within 1 and MAX_SIZE; notifies width and height.
  void setSize(int width, int height);

  /// A generator has no image on the processor.
  virtual const uchar* getBits() { return nullptr; }
  virtual bool bitsHaveChanged() const { return false; }

  /// The image is drawn by OpenGL, from the bottom row up.
  virtual bool isBottomUp() const { return true; }

  /// True when the image must be drawn again: a setting changed, or it is animated and plays.
  virtual bool needsRedraw() const { return _changed || (isAnimated() && isPlaying()); }

  /// Draws the image, if it is due, in the current OpenGL context.
  virtual void update();

  /// Binds the last image drawn (see IntermediateImage::bind()).
  virtual void bind();

  /// Sets the time back to 0.
  virtual void rewind();

  /// Returns the time the generator has played, in seconds.
  double time() const;

  /// Returns how many times the image was drawn.
  int renderCount() const { return _renderCount; }

  /// Returns the timing of the drawings of the image (see FrameStats).
  FrameStats* frameStats() { return &_frameStats; }

signals:
  /// Emitted when the size of the image changed, so that shapes fitting the old size can follow.
  void sizeChanged(int sourceId, int oldWidth, int oldHeight, int newWidth, int newHeight);

protected:
  /// Declares the settings width and height, of defaultSize by default.
  Generator(uid id, int defaultSize = DEFAULT_SIZE);

  /// Draws the image, with the intermediate image as the target (see IntermediateImage::render()).
  virtual void _render() = 0;

  virtual void _doPlay();
  virtual void _doPause();

private:
  int _width;
  int _height;

  /// True when a setting changed since the image was drawn.
  bool _changed;

  /// The frame in which the image was drawn last.
  quint64 _renderedFrame;
  int _renderCount;

  IntermediateImage _image;
  FrameStats _frameStats;

  /// The time played before the last pause, and the time since the last play.
  double _playedSeconds;
  QElapsedTimer _playTimer;
};

/**
 * A generator drawn by a fragment shader written as in the ISF format: the shader reads
 * isf_FragNormCoord (from (0, 0) at the bottom left to (1, 1) at the top right), TIME
 * (seconds) and RENDERSIZE (pixels), and each setting named as a shader input as a
 * uniform of the same name: a float for a Number, an int for an Integer, a bool for a
 * Boolean, a vec4 for a Color. A shader written this way becomes an ISF file by giving
 * it the ISF header that describes its inputs.
 */
class ShaderGenerator : public Generator
{
  Q_OBJECT

protected:
  /// Creates a generator whose image is of defaultSize by default.
  ShaderGenerator(uid id, int defaultSize = DEFAULT_SIZE);

  /// Returns the body of the fragment shader, in GLSL 1.20, without the uniforms of its inputs.
  virtual QString _fragmentShader() const = 0;

  /// Makes the setting at id an input of the shader; call once the setting is declared.
  void _addShaderInput(const QString& id);

  /// Sets uniforms of the shader other than its inputs; called before each drawing.
  virtual void _setExtraUniforms(QOpenGLShaderProgram* program) { Q_UNUSED(program); }

  /// Returns the declarations of uniforms other than its inputs, added to the shader.
  virtual QString _extraUniforms() const { return QString(); }

  virtual void _render();

private:
  /// Returns the program of this kind of generator for the current context, built on first use.
  QOpenGLShaderProgram* _program();

  QStringList _shaderInputs;
};

} // namespace mmp

#endif /* GENERATOR_H_ */
