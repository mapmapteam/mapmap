/*
 * McpServer.cpp
 *
 * Embedded Model Context Protocol (MCP) server for MapMap.
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

#include "McpServer.h"

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHostAddress>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMetaProperty>
#include <QColor>
#include <QDebug>

#include "MM.h"
#include "Element.h"
#include "Paint.h"
#include "Mapping.h"
#include "MappingManager.h"
#include "MainWindow.h"

namespace mmp {

McpServer::McpServer(MainWindow* mainWindow, QObject* parent)
  : QObject(parent), _mainWindow(mainWindow), _httpServer(nullptr), _port(0)
{
}

McpServer::~McpServer()
{
  delete _httpServer;
}

quint16 McpServer::start(quint16 port)
{
  // Recreate from scratch so re-binding to a new port is clean.
  delete _httpServer;
  _httpServer = new QHttpServer(this);
  _port = 0;

  _httpServer->route("/mcp", [this](const QHttpServerRequest& request) -> QHttpServerResponse {
    if (request.method() != QHttpServerRequest::Method::Post)
      return QHttpServerResponse("text/plain", QByteArray("Method Not Allowed"),
                                 QHttpServerResponse::StatusCode::MethodNotAllowed);
    const QByteArray out = handleRpc(request.body());
    if (out.isEmpty()) // notification: acknowledge with no body
      return QHttpServerResponse(QHttpServerResponse::StatusCode::Accepted);
    return QHttpServerResponse("application/json", out, QHttpServerResponse::StatusCode::Ok);
  });

  // Localhost only — never expose the control surface on all interfaces.
  _port = _httpServer->listen(QHostAddress::LocalHost, port);
  if (_port == 0)
  {
    qWarning() << "MCP server failed to bind to port" << port;
    delete _httpServer;
    _httpServer = nullptr;
  }
  return _port;
}

bool McpServer::isListening() const
{
  return _httpServer != nullptr && _port != 0;
}

QByteArray McpServer::handleRpc(const QByteArray& body)
{
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
  {
    const QJsonObject err = makeError(QJsonValue(), -32700, "Parse error");
    return QJsonDocument(err).toJson(QJsonDocument::Compact);
  }

  const QJsonObject request = doc.object();
  // JSON-RPC notifications carry no "id" and expect no response.
  if (!request.contains("id"))
    return QByteArray();

  const QJsonObject response = dispatch(request);
  return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

QJsonObject McpServer::dispatch(const QJsonObject& request)
{
  const QJsonValue id = request.value("id");
  const QString method = request.value("method").toString();
  const QJsonObject params = request.value("params").toObject();

  if (method == "initialize") return makeResult(id, handleInitialize(params));
  if (method == "ping")       return makeResult(id, QJsonObject());
  if (method == "tools/list") return makeResult(id, handleToolsList());
  if (method == "tools/call") return makeResult(id, handleToolsCall(params));

  return makeError(id, -32601, QString("Method not found: %1").arg(method));
}

QJsonObject McpServer::makeResult(const QJsonValue& id, const QJsonValue& result)
{
  QJsonObject o;
  o["jsonrpc"] = "2.0";
  o["id"] = id;
  o["result"] = result;
  return o;
}

QJsonObject McpServer::makeError(const QJsonValue& id, int code, const QString& message)
{
  QJsonObject error;
  error["code"] = code;
  error["message"] = message;
  QJsonObject o;
  o["jsonrpc"] = "2.0";
  o["id"] = id;
  o["error"] = error;
  return o;
}

QJsonObject McpServer::handleInitialize(const QJsonObject& params)
{
  QString protocolVersion = params.value("protocolVersion").toString();
  if (protocolVersion.isEmpty())
    protocolVersion = "2025-06-18";

  QJsonObject capabilities;
  capabilities["tools"] = QJsonObject();

  QJsonObject serverInfo;
  serverInfo["name"] = "MapMap";
  serverInfo["version"] = MM::VERSION;

  QJsonObject result;
  result["protocolVersion"] = protocolVersion;
  result["capabilities"] = capabilities;
  result["serverInfo"] = serverInfo;
  return result;
}

QJsonObject McpServer::handleToolsList() const
{
  QJsonObject result;
  result["tools"] = toolDefinitions();
  return result;
}

// --- Tool dispatch -----------------------------------------------------------

namespace {

QJsonObject textResult(const QString& text, bool isError = false)
{
  QJsonObject content;
  content["type"] = "text";
  content["text"] = text;
  QJsonArray contents;
  contents.append(content);
  QJsonObject result;
  result["content"] = contents;
  result["isError"] = isError;
  return result;
}

QJsonObject jsonResult(const QJsonObject& data)
{
  QJsonObject result = textResult(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
  result["structuredContent"] = data;
  return result;
}

} // namespace

QJsonObject McpServer::handleToolsCall(const QJsonObject& params)
{
  const QString name = params.value("name").toString();
  const QJsonObject args = params.value("arguments").toObject();
  MappingManager& mm = _mainWindow->getMappingManager();

  // ---- Playback / project ----
  if (name == "play")    { _mainWindow->play();    return textResult("Playback started."); }
  if (name == "pause")   { _mainWindow->pause();   return textResult("Playback paused."); }
  if (name == "rewind")  { _mainWindow->rewind();  return textResult("Rewound to start."); }
  if (name == "quit")
  {
    // Defer so we can still send the HTTP response (and not block on a dialog).
    QMetaObject::invokeMethod(_mainWindow, "close", Qt::QueuedConnection);
    return textResult("Quitting MapMap.");
  }
  if (name == "clear_project") { _mainWindow->clearProject(); return textResult("Project cleared."); }
  if (name == "load_project")
  {
    const QString path = args.value("path").toString();
    if (path.isEmpty()) return textResult("Missing required argument 'path'.", true);
    const bool ok = _mainWindow->loadFile(path);
    return textResult(ok ? QString("Loaded project: %1").arg(path)
                         : QString("Failed to load project: %1").arg(path), !ok);
  }
  if (name == "save_project")
  {
    const QString path = args.value("path").toString();
    if (path.isEmpty()) return textResult("Missing required argument 'path'.", true);
    const bool ok = _mainWindow->saveFile(path);
    return textResult(ok ? QString("Saved project: %1").arg(path)
                         : QString("Failed to save project: %1").arg(path), !ok);
  }
  if (name == "set_fps")
  {
    const qreal fps = args.value("fps").toDouble();
    if (fps <= 0) return textResult("'fps' must be greater than 0.", true);
    _mainWindow->setFramesPerSecond(fps);
    return textResult(QString("Frame rate set to %1 fps.").arg(fps));
  }

  // ---- Sources (paints) ----
  if (name == "create_color_source")
  {
    const QColor color(args.value("color").toString());
    if (!color.isValid())
      return textResult("Invalid 'color' (use e.g. \"#ff0000\" or \"red\").", true);
    if (!_mainWindow->addColorPaint(color))
      return textResult("Failed to create color source.", true);
    return jsonResult(sourceSummary(_mainWindow->getCurrentPaintId()));
  }
  if (name == "create_media_source")
  {
    const QString uri = args.value("uri").toString();
    if (uri.isEmpty()) return textResult("Missing required argument 'uri'.", true);
    const bool isImage = args.value("is_image").toBool(false);
    if (!_mainWindow->importMediaFile(uri, isImage, false))
      return textResult(QString("Failed to import media: %1").arg(uri), true);
    return jsonResult(sourceSummary(_mainWindow->getCurrentPaintId()));
  }
  if (name == "delete_source")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    if (mm.getPaintById(id).isNull()) return textResult(QString("No source with id %1.").arg(id), true);
    _mainWindow->deletePaint(id);
    return textResult(QString("Deleted source %1.").arg(id));
  }

  // ---- Layers (mappings) ----
  if (name == "delete_layer")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    if (mm.getMappingById(id).isNull()) return textResult(QString("No layer with id %1.").arg(id), true);
    _mainWindow->deleteMapping(id);
    return textResult(QString("Deleted layer %1.").arg(id));
  }
  if (name == "duplicate_layer")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    if (mm.getMappingById(id).isNull()) return textResult(QString("No layer with id %1.").arg(id), true);
    _mainWindow->duplicateMapping(id);
    return textResult(QString("Duplicated layer %1.").arg(id));
  }
  if (name == "move_layer")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    if (mm.getMappingById(id).isNull()) return textResult(QString("No layer with id %1.").arg(id), true);
    const int index = static_cast<int>(args.value("index").toInteger(0));
    _mainWindow->moveMapping(id, index);
    return textResult(QString("Moved layer %1 to index %2.").arg(id).arg(index));
  }
  if (name == "set_layer_visible" || name == "set_layer_solo" || name == "set_layer_locked")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    if (mm.getMappingById(id).isNull()) return textResult(QString("No layer with id %1.").arg(id), true);
    const bool value = args.value("value").toBool();
    if (name == "set_layer_visible")     _mainWindow->setMappingVisible(id, value);
    else if (name == "set_layer_solo")   _mainWindow->setMappingSolo(id, value);
    else                                 _mainWindow->setMappingLocked(id, value);
    return jsonResult(layerSummary(id));
  }

  // ---- Generic property set ----
  if (name == "set_property")
  {
    const QString kind = args.value("kind").toString();
    const int id = static_cast<int>(args.value("id").toInteger(0));
    const QString property = args.value("property").toString();
    const QVariant value = args.value("value").toVariant();

    QSharedPointer<Element> element;
    if (kind == "source")     element = mm.getPaintById(id);
    else if (kind == "layer") element = mm.getMappingById(id);
    else return textResult("'kind' must be \"source\" or \"layer\".", true);

    if (element.isNull()) return textResult(QString("No %1 with id %2.").arg(kind).arg(id), true);
    if (property.isEmpty()) return textResult("Missing required argument 'property'.", true);

    if (!element->setProperty(property.toUtf8().constData(), value))
      return textResult(QString("Could not set property '%1' on %2 %3.").arg(property, kind).arg(id), true);
    _mainWindow->updateCanvases();
    return textResult(QString("Set %1 %2 property '%3'.").arg(kind).arg(id).arg(property));
  }

  // ---- Read-back ----
  if (name == "get_state")
  {
    QJsonObject state;
    state["playing"] = _mainWindow->isPlaying();
    state["fps"] = _mainWindow->framesPerSecond();
    state["nSources"] = mm.nPaints();
    state["nLayers"] = mm.nMappings();
    state["currentSourceId"] = static_cast<qint64>(_mainWindow->getCurrentPaintId());
    state["currentLayerId"] = static_cast<qint64>(_mainWindow->getCurrentMappingId());
    return jsonResult(state);
  }
  if (name == "list_sources")
  {
    QJsonArray sources;
    for (int i = 0; i < mm.nPaints(); ++i)
    {
      Paint::ptr paint = mm.getPaint(i);
      if (!paint.isNull()) sources.append(sourceSummary(paint->getId()));
    }
    QJsonObject result;
    result["sources"] = sources;
    return jsonResult(result);
  }
  if (name == "list_layers")
  {
    QJsonArray layers;
    for (int i = 0; i < mm.nMappings(); ++i)
    {
      Mapping::ptr mapping = mm.getMapping(i);
      if (!mapping.isNull()) layers.append(layerSummary(mapping->getId()));
    }
    QJsonObject result;
    result["layers"] = layers;
    return jsonResult(result);
  }
  if (name == "get_source")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    Paint::ptr paint = mm.getPaintById(id);
    if (paint.isNull()) return textResult(QString("No source with id %1.").arg(id), true);
    return jsonResult(elementProperties(paint.data()));
  }
  if (name == "get_layer")
  {
    const int id = static_cast<int>(args.value("id").toInteger(0));
    Mapping::ptr mapping = mm.getMappingById(id);
    if (mapping.isNull()) return textResult(QString("No layer with id %1.").arg(id), true);
    return jsonResult(elementProperties(mapping.data()));
  }

  return textResult(QString("Unknown tool: %1").arg(name), true);
}

// --- Read-back helpers -------------------------------------------------------

QJsonObject McpServer::sourceSummary(int paintId) const
{
  QJsonObject o;
  Paint::ptr paint = _mainWindow->getMappingManager().getPaintById(paintId);
  if (paint.isNull()) return o;
  o["id"] = static_cast<qint64>(paint->getId());
  o["name"] = paint->getName();
  o["opacity"] = paint->getOpacity();
  o["locked"] = paint->isLocked();
  switch (paint->getSourceType())
  {
    case Paint::Video: o["type"] = "video"; break;
    case Paint::Image: o["type"] = "image"; break;
    case Paint::Color: o["type"] = "color"; break;
    default:           o["type"] = "unknown"; break;
  }
  return o;
}

QJsonObject McpServer::layerSummary(int mappingId) const
{
  QJsonObject o;
  Mapping::ptr mapping = _mainWindow->getMappingManager().getMappingById(mappingId);
  if (mapping.isNull()) return o;
  o["id"] = static_cast<qint64>(mapping->getId());
  o["name"] = mapping->getName();
  o["visible"] = mapping->isVisible();
  o["solo"] = mapping->isSolo();
  o["locked"] = mapping->isLocked();
  o["depth"] = mapping->getDepth();
  o["opacity"] = mapping->getOpacity();
  o["sourceId"] = mapping->getPaint().isNull() ? -1 : static_cast<qint64>(mapping->getPaintId());
  return o;
}

QJsonObject McpServer::elementProperties(Element* element) const
{
  QJsonObject o;
  if (!element) return o;
  const QMetaObject* meta = element->metaObject();
  for (int i = 0; i < meta->propertyCount(); ++i)
  {
    const QMetaProperty property = meta->property(i);
    if (!property.isReadable()) continue;
    const QVariant value = property.read(element);
    if (value.typeId() == QMetaType::QColor)
      o[QString::fromUtf8(property.name())] = qvariant_cast<QColor>(value).name();
    else if (value.typeId() == QMetaType::QIcon)
      continue; // not meaningfully serialisable
    else
      o[QString::fromUtf8(property.name())] = QJsonValue::fromVariant(value);
  }
  return o;
}

// --- Tool catalogue ----------------------------------------------------------

QJsonArray McpServer::toolDefinitions() const
{
  auto tool = [](const QString& name, const QString& description, const QJsonObject& properties,
                 const QJsonArray& required) {
    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = properties;
    if (!required.isEmpty()) schema["required"] = required;
    QJsonObject t;
    t["name"] = name;
    t["description"] = description;
    t["inputSchema"] = schema;
    return t;
  };
  auto prop = [](const QString& type, const QString& description) {
    QJsonObject p;
    p["type"] = type;
    p["description"] = description;
    return p;
  };

  QJsonArray tools;

  tools.append(tool("play", "Start playback of all sources.", QJsonObject(), QJsonArray()));
  tools.append(tool("pause", "Pause playback.", QJsonObject(), QJsonArray()));
  tools.append(tool("rewind", "Rewind all sources to the start.", QJsonObject(), QJsonArray()));
  tools.append(tool("quit", "Quit the MapMap application.", QJsonObject(), QJsonArray()));
  tools.append(tool("clear_project", "Clear all sources and layers from the project.",
                    QJsonObject(), QJsonArray()));

  tools.append(tool("load_project", "Load a MapMap project from a file path.",
                    QJsonObject{{"path", prop("string", "Absolute path to the .mmp project file.")}},
                    QJsonArray{"path"}));
  tools.append(tool("save_project", "Save the current project to a file path.",
                    QJsonObject{{"path", prop("string", "Absolute path to write the .mmp project file.")}},
                    QJsonArray{"path"}));
  tools.append(tool("set_fps", "Set the playback frame rate.",
                    QJsonObject{{"fps", prop("number", "Frames per second (> 0).")}},
                    QJsonArray{"fps"}));

  tools.append(tool("create_color_source", "Create a solid-colour source. Returns the new source.",
                    QJsonObject{{"color", prop("string", "Colour as \"#rrggbb\" or a named colour.")}},
                    QJsonArray{"color"}));
  tools.append(tool("create_media_source",
                    "Import an image or video file as a source. Returns the new source.",
                    QJsonObject{
                      {"uri", prop("string", "Absolute path to the media file.")},
                      {"is_image", prop("boolean", "True for an image, false for a video (default false).")}
                    },
                    QJsonArray{"uri"}));
  tools.append(tool("delete_source", "Delete a source and its associated layers.",
                    QJsonObject{{"id", prop("integer", "Source id.")}}, QJsonArray{"id"}));

  tools.append(tool("delete_layer", "Delete a layer.",
                    QJsonObject{{"id", prop("integer", "Layer id.")}}, QJsonArray{"id"}));
  tools.append(tool("duplicate_layer", "Duplicate a layer.",
                    QJsonObject{{"id", prop("integer", "Layer id.")}}, QJsonArray{"id"}));
  tools.append(tool("move_layer", "Move a layer to a new stacking index.",
                    QJsonObject{
                      {"id", prop("integer", "Layer id.")},
                      {"index", prop("integer", "Target index (0 = bottom).")}
                    },
                    QJsonArray{"id", "index"}));
  tools.append(tool("set_layer_visible", "Show or hide a layer. Returns the layer.",
                    QJsonObject{
                      {"id", prop("integer", "Layer id.")},
                      {"value", prop("boolean", "Visible if true.")}
                    },
                    QJsonArray{"id", "value"}));
  tools.append(tool("set_layer_solo", "Set a layer's solo state. Returns the layer.",
                    QJsonObject{
                      {"id", prop("integer", "Layer id.")},
                      {"value", prop("boolean", "Solo if true.")}
                    },
                    QJsonArray{"id", "value"}));
  tools.append(tool("set_layer_locked", "Lock or unlock a layer. Returns the layer.",
                    QJsonObject{
                      {"id", prop("integer", "Layer id.")},
                      {"value", prop("boolean", "Locked if true.")}
                    },
                    QJsonArray{"id", "value"}));

  tools.append(tool("set_property",
                    "Set an arbitrary property on a source or layer (e.g. name, opacity, color, uri).",
                    QJsonObject{
                      {"kind", prop("string", "\"source\" or \"layer\".")},
                      {"id", prop("integer", "Element id.")},
                      {"property", prop("string", "Property name.")},
                      {"value", QJsonObject{{"description", "Property value (type depends on the property)."}}}
                    },
                    QJsonArray{"kind", "id", "property", "value"}));

  tools.append(tool("get_state",
                    "Get overall state: playing, fps, counts and current selection.",
                    QJsonObject(), QJsonArray()));
  tools.append(tool("list_sources", "List all sources with their summary fields.",
                    QJsonObject(), QJsonArray()));
  tools.append(tool("list_layers", "List all layers with their summary fields.",
                    QJsonObject(), QJsonArray()));
  tools.append(tool("get_source", "Get all properties of a source.",
                    QJsonObject{{"id", prop("integer", "Source id.")}}, QJsonArray{"id"}));
  tools.append(tool("get_layer", "Get all properties of a layer.",
                    QJsonObject{{"id", prop("integer", "Layer id.")}}, QJsonArray{"id"}));

  return tools;
}

}
