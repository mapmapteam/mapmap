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

#include <QColor>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <tr1/memory>

#include "UidAllocator.h"

/**
 * A Paint is a style that can be applied when drawing potentially any shape.
 * 
 * Defines the way to draw any shape.
 * There must be a Mapper that implements this paint for every shape 
 * so that this shape might be drawn with it.
 */
class Paint
{
private:
  static UidAllocator allocator;

  uid _id;

protected:
  Paint(uid id=NULL_UID);

public:
  typedef std::tr1::shared_ptr<Paint> ptr;

  virtual ~Paint();

  static const UidAllocator& getUidAllocator() { return allocator; }

  virtual void build() {}

  /// This method should be called at each call of draw().
  virtual void update() {}

  void setName(const QString& name) { _name = name; }
  QString getName() const { return _name; }
  uid getId() const { return _id; }

  virtual QString getType() const = 0;

private:
  QString _name;
};

class Color : public Paint
{
protected:
  QColor color;

public:
  Color(uid id=NULL_UID) : Paint(id) {}
  Color(const QColor& color_, uid id=NULL_UID) : Paint(id), color(color_) {}

  QColor getColor() const { return color; }
  virtual void setColor(const QColor& color_) { color = color_; }

  virtual QString getType() const { return "color"; }
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

  Texture(uid id=NULL_UID) :
    Paint(id),
    textureId(0),
    x(0),
    y(0)
  {}
  virtual ~Texture() {
    if (textureId != 0)
      glDeleteTextures(1, &textureId);
  }

public:
  GLuint getTextureId() const { return textureId; }
  virtual void loadTexture() {
    if (textureId == 0)
      glGenTextures(1, &textureId);
  }
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
  QString uri;
  QImage image;

public:
  Image(const QString uri_, uid id=NULL_UID) :
    Texture(id),
    uri(uri_)
  {
    image = QGLWidget::convertToGLFormat(QImage(uri));
  }

  virtual ~Image() {}

  const QString getUri() const { return uri; }

  virtual void build() {
  }

  virtual QString getType() const { return "image"; }

  virtual int getWidth() const { return image.width(); }
  virtual int getHeight() const { return image.height(); }
  virtual const uchar* getBits() const { return image.bits(); }
};

class VideoImpl; // forware declaration

/**
 * Paint that is a Texture retrieved via a video file.
 */
class Video : public Texture
{
protected:
  QString uri;
public:
  Video(const QString uri_, uid id=NULL_UID);
  virtual ~Video();
  const QString getUri() const
  {
    return uri;
  }
  virtual void build();
  virtual void update();
  virtual QString getType() const
  {
    return "video";
  }
  virtual int getWidth() const;
  virtual int getHeight() const;
  virtual const uchar* getBits() const;
  /**
   * Checks whether or not video is supported on this platform.
   */
  static bool hasVideoSupport();
private:
  /**
   * Private implementation, so that GStreamer headers don't need
   * to be included from every file in the project.
   */
  VideoImpl * impl_; // PIMPL opaque pointer
};

#endif /* PAINT_H_ */
