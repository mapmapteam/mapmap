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
#include <QMutex>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "Element.h"

MM_BEGIN_NAMESPACE

/**
 * A Paint is a style that can be applied when drawing potentially any shape.
 * 
 * Defines the way to draw any shape.
 * There must be a MappingGui that implements this paint for every shape 
 * so that this shape might be drawn with it.
 */
class Paint : public Element
{
  Q_OBJECT

private:
  static UidAllocator allocator;

  uid _id;

protected:
  Paint(uid id=NULL_UID);

public:
  typedef QSharedPointer<Paint> ptr;

  virtual ~Paint();

  static const UidAllocator& getUidAllocator() { return allocator; }

  /// This method should be called at each call of draw().
  virtual void update() {}

  /// Starts playback.
  virtual void play() {}

  /// Pauses playback.
  virtual void pause() {}

  /// Rewinds.
  virtual void rewind() {}

  /// Locks mutex (default = no effect).
  virtual void lockMutex() {}

  /// Unlocks mutex (default = no effect).
  virtual void unlockMutex() {}

  virtual QString getType() const = 0;
};

class Color : public Paint
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ getColor WRITE setColor)

protected:
  QColor color;

public:
  Q_INVOKABLE Color(int id=NULL_UID) : Paint(id) {}
  Color(const QColor& color_, uid id=NULL_UID) : Paint(id), color(color_) {}

  QColor getColor() const { return color; }
  void setColor(const QColor& color_) { color = color_; }

  virtual QString getType() const { return "color"; }

  virtual QIcon getIcon() const {
    QPixmap pixmap(100,100);
    pixmap.fill(color);
    return QIcon(pixmap);
  }
};

/**
 * Paint that uses an OpenGL texture to paint on potentially any MappingGui.
 * 
 * This video texture is actually an OpenGL texture.
 */
class Texture : public Paint
{
  Q_OBJECT

  Q_PROPERTY(float x READ getX)
  Q_PROPERTY(float y READ getY)

protected:
  GLuint textureId;
  GLfloat x;
  GLfloat y;
  mutable bool bitsChanged;

  Texture(uid id=NULL_UID) :
    Paint(id),
    textureId(0),
    x(0),
    y(0)
  {
    glGenTextures(1, &textureId);
  }

public:
  virtual ~Texture() {
    if (textureId != 0)
      glDeleteTextures(1, &textureId);
  }

public:
  GLuint getTextureId() const { return textureId; }
  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;

  /// Returns image bits data. Next call to bitsHaveChanged() will be false.
  virtual const uchar* getBits() = 0;

  /// Returns true iff bits have changed since last call to getBits().
  virtual bool bitsHaveChanged() const = 0;

  virtual GLfloat getX() const { return x; }
  virtual GLfloat getY() const { return y; }

  virtual void setX(GLfloat xPos) {
      x = xPos;
    }

  virtual void setY(GLfloat yPos) {
      y = yPos;
    }

  virtual void setPosition(GLfloat xPos, GLfloat yPos) {
    setX(xPos);
    setY(yPos);
  }

  virtual QRectF getRect() const { return QRectF(getX(), getY(), getWidth(), getHeight()); }

  virtual void read(const QDomElement& obj);
  virtual void write(QDomElement& obj);

protected:
  // Lists QProperties that should NOT be parsed automatically.
  virtual QList<QString> _propertiesSpecial() const { return Paint::_propertiesSpecial() << "x" << "y"; }
};

/**
 * Paint that is a Texture loaded from an image file.
 */
class Image : public Texture
{
  Q_OBJECT

  Q_PROPERTY(QString uri READ getUri WRITE setUri)

protected:
  QString _uri;
  QImage _image;

public:
  Q_INVOKABLE Image(int id=NULL_UID) : Texture(id) {}
  Image(const QString uri_, uid id=NULL_UID) :
    Texture(id)
  {
    setUri(uri_);
  }

  virtual ~Image() {}

  virtual void build() {
    _image = QGLWidget::convertToGLFormat(QImage(_uri)).mirrored(true, false).transformed(QTransform().rotate(180));
    bitsChanged = true;
  }

  const QString getUri() const { return _uri; }
  bool setUri(const QString &uri);

  virtual QString getType() const { return "image"; }

  virtual int getWidth() const { return _image.width(); }
  virtual int getHeight() const { return _image.height(); }

  virtual const uchar* getBits() {
    bitsChanged = false;
    return _image.bits();
  }

  virtual bool bitsHaveChanged() const { return bitsChanged; }

  virtual QIcon getIcon() const { return QIcon(QPixmap::fromImage(_image)); }
};

class VideoImpl; // forward declaration

/**
 * Paint that is a Texture retrieved via a video file.
 */
class Video : public Texture
{
  Q_OBJECT

  Q_PROPERTY(QString uri READ getUri WRITE setUri)

  Q_PROPERTY(double volume READ getVolume WRITE setVolume)
  Q_PROPERTY(double rate READ getRate WRITE setRate)

public:
  // Thumbnail generation timeout (in ms).
  static const int ICON_TIMEOUT = 1000;

public:
  Q_INVOKABLE Video(int id=NULL_UID);
  Video(const QString uri_, bool live, double rate, uid id=NULL_UID);
  virtual ~Video();
  const QString getUri() const
  {
    return _uri;
  }
  bool setUri(const QString &uri);
  virtual void build();
  virtual void update();

  /// Starts playback.
  virtual void play();
  /// Pauses playback.
  virtual void pause();
  /// Rewinds.
  virtual void rewind();

  /// Locks mutex (default = no effect).
  virtual void lockMutex();

  /// Unlocks mutex (default = no effect).
  virtual void unlockMutex();

  virtual QString getType() const
  {
    return "media";
  }
  virtual int getWidth() const;
  virtual int getHeight() const;

  virtual const uchar* getBits();

  virtual bool bitsHaveChanged() const;

  /// Sets playback rate (in %). Negative values mean reverse playback.
  virtual void setRate(double rate);

  /// Returns playback rate.
  double getRate() const;

  /// Sets audio playback volume (in %).
  virtual void setVolume(double volume);

  /// Returns audio playback volume.
  double getVolume() const;

  /**
   * Checks whether or not video is supported on this platform.
   */
  static bool hasVideoSupport();

  virtual QIcon getIcon() const { return _icon; }

protected:

  // Try to generate a thumbnail from currently loaded movie.
  bool _generateThumbnail();

  QString _uri;
  QIcon _icon;

  /**
   * Private implementation, so that GStreamer headers don't need
   * to be included from every file in the project.
   */
  VideoImpl *_impl;
};

MM_END_NAMESPACE

#endif /* PAINT_H_ */
