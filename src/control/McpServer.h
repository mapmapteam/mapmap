/*
 * McpServer.h
 *
 * Embedded Model Context Protocol (MCP) server for MapMap.
 *
 * Exposes MapMap's control surface (sources, layers, playback, project I/O)
 * to MCP clients over a local HTTP endpoint speaking JSON-RPC 2.0. Runs on the
 * GUI thread's event loop, so tool handlers call into MainWindow directly.
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

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

QT_BEGIN_NAMESPACE
class QHttpServer;
QT_END_NAMESPACE

namespace mmp {

class MainWindow;

/**
 * Embedded MCP server exposing MapMap over local HTTP (JSON-RPC 2.0).
 * The endpoint is POST http://localhost:<port>/mcp.
 */
class McpServer : public QObject {
  Q_OBJECT

public:
  explicit McpServer(MainWindow* mainWindow, QObject* parent = nullptr);
  ~McpServer();

  /// Starts listening on localhost:<port>. Returns the bound port, or 0 on failure.
  quint16 start(quint16 port);

  /// True when the HTTP server is bound and listening.
  bool isListening() const;

  /// Currently bound port (0 if not listening).
  quint16 port() const { return _port; }

private:
  // --- JSON-RPC plumbing ---
  // Returns the response body, or an empty array for notifications (no reply).
  QByteArray handleRpc(const QByteArray& body);
  QJsonObject dispatch(const QJsonObject& request);

  static QJsonObject makeResult(const QJsonValue& id, const QJsonValue& result);
  static QJsonObject makeError(const QJsonValue& id, int code, const QString& message);

  // --- MCP methods ---
  QJsonObject handleInitialize(const QJsonObject& params);
  QJsonObject handleToolsList() const;
  // Routes a tools/call by name; sets isError on failure. Returns MCP result object.
  QJsonObject handleToolsCall(const QJsonObject& params);

  // --- Tool definitions (for tools/list) ---
  QJsonArray toolDefinitions() const;

  // --- Read-back helpers ---
  QJsonObject sourceSummary(int sourceId) const;
  QJsonObject layerSummary(int layerId) const;
  // Full Q_PROPERTY dump for an Element (Source or Layer).
  QJsonObject elementProperties(class Element* element) const;

  MainWindow* _mainWindow;
  QHttpServer* _httpServer;
  quint16 _port;
};

}
