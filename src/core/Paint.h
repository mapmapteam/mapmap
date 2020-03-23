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
#include "Maths.h"

#include <QCameraInfo>

namespace mmp {

typedef enum {
  VIDEO_URI,
  VIDEO_WEBCAM,
  VIDEO_SHMSRC
} VideoType;

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

  enum SourceType {
    Video, Image, Color
  };

  typedef QSharedPointer<Paint> ptr;

  virtual ~Paint();

  static const UidAllocator& getUidAllocator() { return allocator; }

  /// This method should be called at each call of draw().
  virtual void update() {}

  /// Is the paint currently playing?
  virtual bool isPlaying() const { return _isPlaying; }

  /// Starts playback.
  virtual void play() {
    _doPlay();
    _isPlaying = true;
  }

  /// Pauses playback.
  virtual void pause() {
    _doPause();
    _isPlaying = false;
  }

  /// Rewinds.
  virtual void rewind() {}

  /// Locks mutex (default = no effect).
  virtual void lockMutex() {}

  /// Unlocks mutex (default = no effect).
  virtual void unlockMutex() {}

  virtual SourceType getSourceType() const = 0;

protected:
  virtual void _doPlay() {}
  virtual void _doPause() {}

private:
  bool _isPlaying;
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

  virtual SourceType getSourceType() const { return SourceType::Color; }

  virtual QIcon getIcon() const {
    QPixmap pixmap(MM::MAPPING_LIST_ICON_SIZE, MM::MAPPING_LIST_ICON_SIZE);
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
  }

public:
  virtual ~Texture() {
    // TODO: this needs to be fixed: it will not work unless it is executed from within a GL context
    // see issue #229
    if (textureId != 0)
      glDeleteTextures(1, &textureId);
  }

public:
  virtual void update();

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

  // Get Camera human-readable name from url
  QString getCameraNameFromUri(const QString &uri) {
    return QCameraInfo(uri.toLocal8Bit()).description();
  }

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

  Q_PROPERTY(double rate READ getRate WRITE setRate)

protected:
  QString _uri;
  QVector<QImage> _images;
  double _rate;

  uint  _currentFrame;
  qreal _currentFrameReal;
  qreal _prevTime;

  uchar* _bits;

  QElapsedTimer _timer;

public:
  Q_INVOKABLE Image(int id=NULL_UID);
  Image(const QString uri_, uid id=NULL_UID);

  virtual ~Image() {}

  virtual void build();
  virtual void update();

  /// Rewinds.
  virtual void rewind();

  const QString getUri() const { return _uri; }
  bool setUri(const QString &uri);

  virtual SourceType getSourceType() const { return SourceType::Image; }

  bool isAnimation() const { return (_images.size() > 1); }

  virtual int getWidth() const  { return (_images.isEmpty() ? 0 : _images[0].width()); }
  virtual int getHeight() const { return (_images.isEmpty() ? 0 : _images[0].height()); }

  virtual const uchar* getBits();

  virtual bool bitsHaveChanged() const { return bitsChanged; }

  virtual QIcon getIcon() const;

  /// Sets playback rate (in %). Negative values mean reverse playback.
  virtual void setRate(double rate);

  /// Returns playback rate.
  double getRate() const { return _rate; }

protected:

  /// Starts playback.
  virtual void _doPlay();

  /// Current elapsed time in seconds.
  qreal _elapsedTime() const { return _timer.elapsed() / 1000.0; }
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
  Video(const QString uri_, VideoType type, double rate, uid id=NULL_UID);
  virtual ~Video();

  const QString getUri() const { return _uri; }
  bool setUri(const QString &uri);

  virtual void build();
  virtual void update();

  /// Rewinds.
  virtual void rewind();

  /// Locks mutex (default = no effect).
  virtual void lockMutex();

  /// Unlocks mutex (default = no effect).
  virtual void unlockMutex();

  virtual SourceType getSourceType() const { return SourceType::Video; }

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

  /// Starts playback.
  virtual void _doPlay();

  /// Pauses playback.
  virtual void _doPause();

  // Try to generate a thumbnail from currently loaded movie.
  bool _generateThumbnail();

  QString _uri;
  QIcon _icon;
  VideoType _videoType;

  /**
   * Private implementation, so that GStreamer headers don't need
   * to be included from every file in the project.
   */
  VideoImpl *_impl;
};

}

#endif /* PAINT_H_ */
