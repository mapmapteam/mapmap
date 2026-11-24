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

#include <functional>
#include <string>
#include <QColor>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>

#include <QtGui/qopengl.h>

// OpenGL 1.2 tokens, missing from some platform headers (e.g. Windows gl.h).
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
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

  /**
   * Tells whether the views showing this source have to be redrawn even
   * though nobody touched the project: true while the source has, or may
   * soon have, a new image to show. A still source returns false.
   */
  virtual bool needsRedraw() const { return false; }

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
    y(0),
    bitsChanged(false),
    _uploadedWidth(0),
    _uploadedHeight(0)
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

  /// A texture needs a redraw when it holds an image not yet shown.
  virtual bool needsRedraw() const { return bitsHaveChanged(); }

  /// OpenGL pixel format / type of the data returned by getBits().
  virtual GLenum getBitsFormat() const { return GL_RGBA; }
  virtual GLenum getBitsType() const { return GL_UNSIGNED_BYTE; }

  /// Uploads the current bits to the (bound) texture if they have changed.
  /// Reallocates the texture storage only when the frame size changes.
  /// MUST be called with a current GL context and this texture bound.
  void uploadBits();

  /// Makes the texture ready to draw with: uploads new content if needed,
  /// binds it to texture unit 0 (and possibly other units / a shader program,
  /// see Video). Texture coordinates are in [0, 1]. MUST be called with a
  /// current GL context, and paired with release().
  virtual void bind();

  /// Undoes bind() (leaves texture unit 0 active, no program in use).
  virtual void release() {}

protected:
  /// uploadBits() with the mutex already locked.
  void _uploadBitsLocked();

public:

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

  // Size of the texture storage currently allocated on the GPU.
  int _uploadedWidth;
  int _uploadedHeight;
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

  /// An animation picks its next frame in update(), which runs at each draw.
  virtual bool needsRedraw() const { return bitsHaveChanged() || (isAnimation() && isPlaying()); }

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

class VideoImpl;  // forward declaration
class FrameStats; // forward declaration
class YuvTexture; // forward declaration

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
  // Thumbnail generation timeout (in ms): how long to wait for the *seeked*
  // frame once _generateThumbnail() has asked to seek to it. Fine to give up
  // quickly here — worst case is a generic icon instead of a real thumbnail.
  static const int ICON_TIMEOUT = 1000;

  // How long to wait for the *first* decoded frame (gates both the thumbnail
  // attempt and frameSizeKnown, i.e. shape auto-fit). Much more generous than
  // ICON_TIMEOUT: giving up here permanently strands any shape created before
  // the deadline at the DEFAULT_WIDTH/HEIGHT placeholder, which is a real
  // mis-crop, not just a missing thumbnail.
  static const int FIRST_FRAME_TIMEOUT = 15000;

  // How often to re-check for a decoded frame while polling (in ms).
  static const int THUMBNAIL_POLL_INTERVAL = 50;

  /// Frame size assumed before the first frame arrives (drives a new mapping's
  /// initial input shape; auto-fitted once the real resolution is known).
  static const int DEFAULT_WIDTH  = 640;
  static const int DEFAULT_HEIGHT = 480;

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

  /// Returns true if this source reads a live capture device, not a file.
  bool isCamera() const { return _videoType == VIDEO_WEBCAM; }

  virtual int getWidth() const;
  virtual int getHeight() const;

  virtual const uchar* getBits();

  virtual bool bitsHaveChanged() const;

  /// A playing video is redrawn at every tick: update(), which runs at each
  /// draw, is also what restarts a looping movie when it reaches its end.
  virtual bool needsRedraw() const { return bitsHaveChanged() || isPlaying(); }

  /// Draws YUV frames straight from the decoder's planes (converted to RGB by
  /// a shader, see YuvTexture), other frames as a regular RGBA texture.
  virtual void bind();
  virtual void release();

  /// Returns the timing of the frames of this video (see FrameStats).
  FrameStats* frameStats() const;

  /// Upload format of the current frame, which depends on what Qt delivers
  /// (BGRA or RGBA, see VideoImpl::frameToImage()).
  virtual GLenum getBitsFormat() const;
  virtual GLenum getBitsType() const;

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

signals:
  /// Emitted once, the first time a real frame's resolution becomes known, so
  /// the UI can fit input shapes created (at the default size) before any
  /// frame had arrived.
  void frameSizeKnown(int sourceId, int width, int height);

protected:
  /**
   * Creates a source of the given type with no media loaded yet.
   *
   * @param type Whether frames come from a file or from a capture device.
   * @param id Unique ID of the source.
   */
  Video(VideoType type, int id);

  /// Starts playback.
  virtual void _doPlay();

  /// Pauses playback.
  virtual void _doPause();

  // Sets a generic fallback icon (file icon, or a generic video/camera icon).
  void _setFallbackIcon();

  // Builds a thumbnail icon from the frame currently held by _impl. Assumes
  // the caller has already confirmed a fresh frame is available (see
  // _pollForBits()).
  bool _generateThumbnail();

  // Polls (via the Qt event loop, never blocking) until _impl has a fresh
  // frame, then calls onReady. Gives up silently after attemptsLeft tries.
  //
  // Qt Multimedia delivers frames asynchronously through the event loop of
  // the thread that created the QMediaPlayer/QVideoSink (here, the GUI
  // thread), so this cannot be replaced by a synchronous/blocking wait:
  // blocking the GUI thread also blocks the delivery of the very frame being
  // waited for, and starves the media backend's own event processing while
  // doing so. See the removed VideoImpl::waitForNextBits().
  void _pollForBits(std::function<void()> onReady, int attemptsLeft);

  QString _uri;
  QIcon _icon;
  VideoType _videoType;

  VideoImpl *_impl;

  // GPU YUV path (see bind()). Created on first YUV frame.
  YuvTexture *_yuv;
  // True if the content to draw is in _yuv (rather than textureId).
  bool _yuvActive;
};

/**
 * Source that is a Texture retrieved from a live capture device (camera).
 *
 * This is a class of its own so that a project file records that the source
 * is a camera: its URI is a capture device ID rather than a file path, and it
 * has to be reopened with the capture back-end when the project is loaded.
 */
class Camera : public Video
{
  Q_OBJECT

public:
  /// Creates a camera source with no device opened yet (used by ProjectReader).
  Q_INVOKABLE Camera(int id=NULL_UID);

  /**
   * Creates a camera source and opens its capture device.
   *
   * @param deviceId ID of the capture device, as given by QCameraDevice::id().
   * @param rate Playback rate, kept for symmetry with Video.
   * @param id Unique ID of the source.
   */
  Camera(const QString deviceId, double rate, uid id=NULL_UID);

  /**
   * Tells whether a URI names a capture device currently available.
   *
   * @param uri URI read from a project file.
   * @return True if it equals the ID of one of the system's video inputs.
   */
  static bool isCaptureDeviceId(const QString& uri);
};

}

#endif /* SOURCE_H_ */
