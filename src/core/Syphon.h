/*
 * Syphon.h
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef SYPHON_H_
#define SYPHON_H_

#include <QtGlobal>

// Syphon is a macOS-only inter-application video-sharing framework.
#ifdef HAVE_SYPHON

#include <QList>
#include <QString>
#include <QImage>

#include "Source.h"

namespace mmp {

/**
 * Lightweight, UI-friendly description of a discoverable Syphon server.
 *
 * A Syphon server is published by another application (Resolume, VDMX,
 * MadMapper, a Processing/openFrameworks sketch, ...). It is identified
 * system-wide by its UUID; the name and appName are human-readable labels.
 */
struct SyphonServerDescription
{
  QString uuid;     ///< System-wide unique identifier (never shown to the user).
  QString name;     ///< Server name (may be empty).
  QString appName;  ///< Localized name of the hosting application.
  QImage  icon;     ///< Hosting application's icon (may be null).

  /// Human-readable label combining app name and server name.
  QString displayName() const
  {
    if (name.isEmpty())    return appName;
    if (appName.isEmpty()) return name;
    return QString("%1 — %2").arg(appName, name); // "App — Server"
  }

  bool isEmpty() const { return uuid.isEmpty() && name.isEmpty() && appName.isEmpty(); }
};

// Objective-C++ implementation, defined in SyphonImpl.mm. Kept opaque so this
// header stays pure C++ and can be included by C++ translation units and moc.
class SyphonImpl;

/**
 * Source that receives live frames from a Syphon server (macOS only).
 *
 * Implemented as a Texture: each frame is blitted on the GPU from the shared
 * GL_TEXTURE_RECTANGLE into this source's own GL_TEXTURE_2D, which MapMap then
 * draws like any other texture (see SyphonImpl.mm and ShapeGraphicsItem).
 *
 * The chosen server is persisted (uuid/name/appName) so the source reconnects
 * automatically across project loads and when the publishing app (re)starts.
 */
class Syphon : public Texture
{
  Q_OBJECT

  Q_PROPERTY(QString serverUUID READ getServerUUID WRITE setServerUUID)
  Q_PROPERTY(QString serverName READ getServerName WRITE setServerName)
  Q_PROPERTY(QString appName    READ getAppName    WRITE setAppName)
  Q_PROPERTY(bool respectAlpha  READ getRespectAlpha WRITE setRespectAlpha)

public:
  /// Frame size assumed before the first frame arrives (drives a new mapping's
  /// initial input shape; auto-fitted once the real resolution is known).
  static const int DEFAULT_WIDTH  = 640;
  static const int DEFAULT_HEIGHT = 480;

  Q_INVOKABLE Syphon(int id = NULL_UID);
  virtual ~Syphon();

  virtual void build();
  virtual void update();

  virtual SourceType getSourceType() const { return SourceType::Syphon; }

  virtual int getWidth() const;
  virtual int getHeight() const;
  virtual const uchar* getBits();
  virtual bool bitsHaveChanged() const;

  virtual QIcon getIcon() const;

  // --- Persisted server identity ---------------------------------------
  QString getServerUUID() const { return _serverUUID; }
  QString getServerName() const { return _serverName; }
  QString getAppName()    const { return _appName; }

  void setServerUUID(const QString& uuid)   { _serverUUID = uuid; _reconnect(); }
  void setServerName(const QString& name)   { _serverName = name; _reconnect(); }
  void setAppName(const QString& appName)   { _appName    = appName; _reconnect(); }

  /// When false (default), incoming frames are forced opaque (Syphon servers
  /// often publish a garbage alpha). When true, the server's alpha is kept.
  bool getRespectAlpha() const { return _respectAlpha; }
  void setRespectAlpha(bool respect) { _respectAlpha = respect; }

  /// Point this source at a specific server (sets all three identifiers at once).
  void connectToServer(const QString& uuid, const QString& name, const QString& appName);

  /// True if a live connection to the chosen server is currently established.
  bool isConnected() const;

  /// Lists the Syphon servers currently available on the system.
  static QList<SyphonServerDescription> availableServers();

  /// Builds the default Syphon glyph icon (drawn, no binary asset required).
  static QIcon defaultIcon(int size, const QColor& color);

signals:
  /// Emitted once, the first time a real frame resolution becomes known, so the
  /// UI can fit input shapes created before any frame had arrived.
  void frameSizeKnown(int sourceId, int width, int height);

private:
  /// Marks that the underlying client should (re)connect on the next update().
  void _reconnect();

  QString _serverUUID;
  QString _serverName;
  QString _appName;
  bool    _respectAlpha = false;
  bool    _frameAnnounced = false;

  SyphonImpl* _impl;
};

}

#endif // HAVE_SYPHON

#endif /* SYPHON_H_ */
