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
 * Implemented as a Texture: each frame is read back from the shared
 * GL_TEXTURE_RECTANGLE into a CPU RGBA8888 buffer, so it flows through the
 * exact same upload/render path as Video and Image (see ShapeGraphicsItem).
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

public:
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

  /// Point this source at a specific server (sets all three identifiers at once).
  void connectToServer(const QString& uuid, const QString& name, const QString& appName);

  /// True if a live connection to the chosen server is currently established.
  bool isConnected() const;

  /// Lists the Syphon servers currently available on the system.
  static QList<SyphonServerDescription> availableServers();

  /// Builds the default Syphon glyph icon (drawn, no binary asset required).
  static QIcon defaultIcon(int size, const QColor& color);

private:
  /// Marks that the underlying client should (re)connect on the next update().
  void _reconnect();

  QString _serverUUID;
  QString _serverName;
  QString _appName;

  SyphonImpl* _impl;
};

}

#endif // HAVE_SYPHON

#endif /* SYPHON_H_ */
