/*
 * Paint.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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


#ifndef PAINT_H_
#define PAINT_H_

#include <QtGlobal>
#include <QtOpenGL>
#include <string>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <tr1/memory>

/**
 * A Paint is a style that can be applied when drawing potentially any shape.
 * 
 * Defines the way to draw any shape.
 * There must be a Mapper that implements this paint for every shape 
 * so that this shape might be drawn with it.
 */
class Paint
{
protected:
  Paint() {}
public:
  typedef std::tr1::shared_ptr<Paint> ptr;
  virtual ~Paint() {}
  virtual void build() {}
};

/**
 * Paint that uses an OpenGL texture to paint on potentially any Mapper.
 * 
 * This video texture is actually an OpenGL texture.
 */
class Texture : public Paint
{
protected:
  GLuint textureId;
  GLfloat x;
  GLfloat y;

  Texture(GLuint textureId_=0) : textureId(textureId_), x(-1), y(-1) {}
  virtual ~Texture() {}

public:
  GLuint getTextureId() const { return textureId; }
  virtual void loadTexture() = 0;
  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;
  virtual const uchar* getBits() const = 0;

  virtual void setPosition(GLfloat xPos, GLfloat yPos) {
    x = xPos;
    y = yPos;
  }
  virtual GLfloat getX() const { return x; }
  virtual GLfloat getY() const { return y; }
};

/**
 * Paint that is a Texture loaded from an image file.
 */
class Image : public Texture
{
protected:
  QString imagePath;
  QImage image;

public:
  Image(const QString imagePath_) : Texture(), imagePath(imagePath_) {
    image = QGLWidget::convertToGLFormat(QImage(imagePath));
  }

  virtual ~Image() {
  }

  virtual void loadTexture() {
    if (textureId == 0)
    {
      glGenTextures(1, &textureId);
    }
    // TODO: free data?
  }

  const QString getImagePath() const { return imagePath; }

  virtual void build() {
  }

  virtual int getWidth() const { return image.width(); }
  virtual int getHeight() const { return image.height(); }
  virtual const uchar* getBits() const { return image.bits(); }
};

#endif /* PAINT_H_ */
