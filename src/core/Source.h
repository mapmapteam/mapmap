/*
 * Source.h
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


#ifndef SOURCE_H_
#define SOURCE_H_

#include <QtGlobal>

#include <string>
#include <QColor>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "Element.h"
#include "Maths.h"

#include <QMediaDevices>
#include <QCameraDevice>

namespace mmp {

typedef enum {
  VIDEO_URI,
  VIDEO_WEBCAM
} VideoType;

/**
 * A Source is a style that can be applied when drawing potentially any shape.
 *
 * Defines the way to draw any shape.
 * There must be a MappingGui that implements this source for every shape
 * so that this shape might be drawn with it.
 */
class Source : public Element
{
  Q_OBJECT

private:
  static UidAllocator allocator;

  uid _id;

protected:
  Source(uid id=NULL_UID);

public:

  enum SourceType {
    Video, Image, Color, Syphon
  };

  typedef QSharedPointer<Source> ptr;

  virtual ~Source();

  static const UidAllocator& getUidAllocator() { return allocator; }

  /// This method should be called at each call of draw().
  virtual void update() {}

  /// Is the source currently playing?
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

  /// Releases heavy external resources (capture devices, players, network
  /// clients) while keeping the object valid — e.g. when the source is removed
  /// but kept in the undo history. Reversible via reacquireResources().
  virtual void releaseResources() {}

  /// Re-acquires resources released by releaseResources() (e.g. on undo).
  virtual void reacquireResources() {}

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

class Color : public Source
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ getColor WRITE setColor)

protected:
  QColor color;

public:
  Q_INVOKABLE Color(int id=NULL_UID) : Source(id) {}
  Color(const QColor& color_, uid id=NULL_UID) : Source(id), color(color_) {}

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
 * Source that uses an OpenGL texture to render on potentially any MappingGui.
 *
 * This video texture is actually an OpenGL texture.
 */
class Texture : public Source
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
    Source(id),
    textureId(0),
    x(0),
    y(0)
  {
  }

public:
  virtual ~Texture() {
    // A Texture is usually destroyed (e.g. when its source is removed) when no
    // GL context is current, so we cannot call glDeleteTextures here (issue
    // #229). Instead the id is queued and freed by deleteOrphanedTextures(),
    // which the renderer calls while a context is current.
    if (textureId != 0)
      orphanTexture(textureId);
  }

  /// Frees textures queued by destroyed Texture objects. MUST be called with a
  /// current GL context (the renderer calls it at the start of painting).
  static void deleteOrphanedTextures();

protected:
  /// Queues a texture id for deferred deletion (see issue #229).
  static void orphanTexture(GLuint id);

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

  virtual void read(const QJsonObject& obj);
  virtual void write(QJsonObject& obj);

  // Get Camera human-readable name from device ID
  QString getCameraNameFromUri(const QString &uri) {
    for (const QCameraDevice& d : QMediaDevices::videoInputs())
      if (QString::fromUtf8(d.id()) == uri)
        return d.description();
    return uri;
  }

protected:
  // Lists QProperties that should NOT be parsed automatically.
  virtual QList<QString> _propertiesSpecial() const { return Source::_propertiesSpecial() << "x" << "y"; }

private:
  // Texture ids whose Texture objects were destroyed without a current GL
  // context; freed later by deleteOrphanedTextures(). See issue #229.
  static QVector<GLuint> _orphanedTextures;
  static QMutex          _orphanedTexturesMutex;
};

/**
 * Source that is a Texture loaded from an image file.
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
 * Source that is a Texture retrieved via a video file.
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

  /// Releases the capture device / player (keeps the object valid).
  virtual void releaseResources();
  /// Re-opens the capture device / player released by releaseResources().
  virtual void reacquireResources();

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

  VideoImpl *_impl;
};

}

#endif /* SOURCE_H_ */
