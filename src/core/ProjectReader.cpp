/*
 * ProjectReader.cpp
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
#include "ProjectReader.h"
#include "Generators.h"
#include "Shapes.h"
#include "Util.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace mmp {

namespace {

/// Class names, as written in project files, of the colour sources and colour layers of
/// earlier versions: they are read as colour generators and texture layers.
const char* const LEGACY_COLOR_CLASS = "Color";
const char* const LEGACY_COLOR_LAYER_CLASSES[] = { "ColorLayer", "ColorMapping" };

/// Key of the colour of a colour source, and the alpha of an opaque colour.
const char* const LEGACY_COLOR_KEY = "color";
const int OPAQUE_ALPHA = 255;

/// Number of corners of a quad.
const int QUAD_CORNERS = 4;

} // namespace

ProjectReader::ProjectReader(MappingManager& manager, MediaLocator mediaLocator, const QString& projectDirectory)
  : _projectDirectory(projectDirectory), _manager(manager), _mediaLocator(mediaLocator)
{
}

bool ProjectReader::isValidVersion(const QString& versionString)
{
    QRegularExpression re(MM::SUPPORTED_FILE_VERSIONS);
    QRegularExpressionMatch match = re.match(versionString);
    return match.hasMatch();
}

bool ProjectReader::readFile(QIODevice* device, std::function<void()> beforeReplace)
{
  _errorString.clear();
  _skippedEntries.clear();
  QByteArray data = device->readAll();
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (doc.isNull()) {
    // Projects saved by MapMap 0.6 and earlier are XML: they cannot be
    // opened anymore, so say so plainly instead of reporting a parse error.
    if (data.trimmed().startsWith('<')) {
      QRegularExpressionMatch match =
          QRegularExpression("<project[^>]*\\sversion=\"([^\"]*)\"").match(QString::fromUtf8(data));
      QString oldVersion = match.hasMatch() ? match.captured(1) : QObject::tr("unknown");
      _errorString = QObject::tr("This project was saved in the old XML format by MapMap %1. "
                                 "This version of MapMap (%2) only opens JSON projects.")
          .arg(oldVersion, MM::VERSION);
      return false;
    }
    _errorString = QObject::tr("Parse error at offset %1: %2")
        .arg(parseError.offset).arg(parseError.errorString());
    return false;
  }

  QJsonObject root = doc.object();
  QString projectVersion = root["version"].toString();

  if (projectVersion.isEmpty()) {
    _errorString = QObject::tr("The contents of this file does not look like a MapMap project.");
    return false;
  } else if (!isValidVersion(projectVersion)) {
    _errorString = QObject::tr("The version of MapMap %1 used to save this file is not readable by this MapMap version %2.")
        .arg(projectVersion, MM::VERSION);
    return false;
  }

  for (const char* key : { ProjectLabels::SOURCES, ProjectLabels::LAYERS })
  {
    if (!root.value(key).isArray())
    {
      _errorString = QObject::tr("The project field '%1' must be an array.").arg(key);
      return false;
    }
  }
  if (beforeReplace)
    beforeReplace();
  parseProject(root);
  return true;
}

void ProjectReader::parseOutputResolution(const QJsonObject& project)
{
  // Older projects have none: MapMap then picks one (see MainWindow).
  if (!project.contains(ProjectLabels::OUTPUT_RESOLUTION))
    return;

  const QJsonObject resolution = project[ProjectLabels::OUTPUT_RESOLUTION].toObject();
  const QSize size(resolution[ProjectLabels::WIDTH].toInt(), resolution[ProjectLabels::HEIGHT].toInt());
  if (MappingManager::isValidOutputResolution(size))
    _manager.setOutputResolution(size);
  else
    qWarning() << "Ignoring unreadable output resolution in project:"
               << project[ProjectLabels::OUTPUT_RESOLUTION];
}

QString ProjectReader::errorString() const
{
  return _errorString;
}

void ProjectReader::parseProject(const QJsonObject& project)
{
  MappingManager& manager = _manager;
  manager.clearAll();

  parseOutputResolution(project);

  QJsonArray sources = project[ProjectLabels::SOURCES].toArray();
  QJsonArray layers  = project[ProjectLabels::LAYERS].toArray();

  const bool legacyPaths = project.value("version").toString() != ProjectLabels::FORMAT_VERSION;
  for (qsizetype i = 0; i < sources.size(); ++i)
  {
    Source::ptr source = parseSource(sources.at(i).toObject(), legacyPaths);

    if (source.isNull())
    {
      _skippedEntries << QStringLiteral("sources[%1]").arg(i);
      qWarning() << "Skipping" << _skippedEntries.last();
    }
    else
    {
      manager.addSource(source);
    }
  }

  // Parse layers (formerly mappings).
  QVector<Layer::ptr> allLayers;
  for (qsizetype i = 0; i < layers.size(); ++i)
  {
    Layer::ptr layer = parseLayer(layers.at(i).toObject());
    if (layer.isNull())
    {
      _skippedEntries << QStringLiteral("layers[%1]").arg(i);
      qWarning() << "Skipping" << _skippedEntries.last();
    }
    else
    {
      allLayers.push_back(layer);
    }
  }

  // Add all mappings in reverse order.
  for (QVector<Layer::ptr>::const_reverse_iterator it = allLayers.rbegin();
          it != allLayers.rend(); ++it)
  {
    manager.addLayer(*it);
  }
}

Source::ptr ProjectReader::parseSource(QJsonObject obj, bool legacyPaths)
{
  QString className = Serializable::classNameCleanToReal(obj[ProjectLabels::CLASS_NAME].toString());
  int id            = obj[ProjectLabels::ID].toInt(NULL_UID);

  if (id <= NULL_UID || Source::getUidAllocator().exists(id))
    return Source::ptr();

  qDebug() << "Found source with classname: " << className << Qt::endl;

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);

  // A colour source of an earlier version is a colour generator: same id, name, colour and
  // opacity, and an image that its layers read through an input shape.
  if (className == Serializable::classNameCleanToReal(QLatin1String(LEGACY_COLOR_CLASS)))
  {
    metaObject = &ColorGenerator::staticMetaObject;
    // Those sources drew their colour opaque, at the opacity of the source and the layer:
    // the alpha of the colour was not used. It is dropped, so that the project looks the same.
    QColor color(obj.value(LEGACY_COLOR_KEY).toString());
    if (color.isValid())
    {
      color.setAlpha(OPAQUE_ALPHA);
      obj.insert(LEGACY_COLOR_KEY, color.name(QColor::HexRgb));
    }
  }

  // Projects saved before the Camera class existed recorded a camera as a
  // plain Video whose URI is a capture device ID: reopen those as cameras.
  if (metaObject == &Video::staticMetaObject
      && Camera::isCaptureDeviceId(obj[ProjectLabels::URI].toString()))
  {
    metaObject = &Camera::staticMetaObject;
  }

  if (metaObject && metaObject->inherits(&Source::staticMetaObject))
  {
    Source::ptr source(qobject_cast<Source*>(metaObject->newInstance(Q_ARG(int, id))));

    if (source.isNull())
    {
      // Registered type that could not be instantiated (malformed or
      // incompatible entry). Bail out instead of dereferencing a null source.
      qWarning() << "Could not instantiate source of type" << className;
      return Source::ptr();
    }

    qDebug() << "Created new instance with id: " << source->getId();
    const bool isImage = metaObject->inherits(&Image::staticMetaObject);
    const bool isVideoFile = metaObject->inherits(&Video::staticMetaObject)
        && !metaObject->inherits(&Camera::staticMetaObject);
    QString path = obj.value(ProjectLabels::URI).toString();
    if ((isImage || isVideoFile) && !path.isEmpty())
    {
      const bool relative = Texture::isRelativeMediaPath(path);
      if (relative && !_projectDirectory.isEmpty())
      {
        const QString fromProject = QDir::cleanPath(QDir(_projectDirectory).absoluteFilePath(path));
        path = legacyPaths && !QFileInfo(fromProject).isFile() && QFileInfo(path).isFile()
            ? QFileInfo(path).absoluteFilePath() : fromProject;
      }
      if (_mediaLocator)
      {
        const QString replacement = _mediaLocator(path, isImage);
        if (!replacement.isEmpty())
          path = replacement;
      }
      static_cast<Texture*>(source.data())->setPrefersRelativeMediaPath(relative);
      obj.insert(ProjectLabels::URI, path);
    }
    source->read(obj);
    return source;
  }
  else
  {
    // Unknown source type (e.g. a macOS-only Syphon source opened on another
    // platform). Skip it with a warning rather than failing the whole load.
    qWarning() << "Skipping unsupported source of type" << className;
    return Source::ptr();
  }
}

QString ProjectReader::unusableLayerReason(const Layer::ptr& layer)
{
  // The interface has an editor for these shapes only (see
  // MainWindow::addLayerItem): a triangle, a mesh or an ellipse.
  auto isSupported = [](const MShape::ptr& shape) {
    return shape->getType() == MShape::Triangle
        || shape->getType() == MShape::Mesh
        || shape->getType() == MShape::Ellipse;
  };

  if (layer->getSource().isNull())
    return "its source is missing";

  const MShape::ptr output = layer->getShape();
  if (output.isNull())
    return "its destination shape is missing or of an unknown type";
  if (!isSupported(output))
    return QString("its destination shape is of an unsupported type (%1)").arg(output->metaObject()->className());

  if (layer->hasInputShape())
  {
    const MShape::ptr input = layer->getInputShape();
    if (input.isNull())
      return "its source shape is missing or of an unknown type";
    if (input->getType() != output->getType())
      return QString("its source shape (%1) does not match its destination shape (%2)")
          .arg(input->metaObject()->className(), output->metaObject()->className());
  }

  return QString();
}

void ProjectReader::giveInputShapeToColorLayer(const Layer::ptr& layer)
{
  MShape::ptr output = layer->getShape();
  QSharedPointer<Texture> texture = qSharedPointerDynamicCast<Texture>(layer->getSource());
  if (output.isNull() || texture.isNull())
    return; // unusableLayerReason() says why.

  // Texture layers have no plain quad: a quad is a mesh of four corners.
  if (output->getType() == MShape::Quad)
  {
    const QVector<QPointF> corners = output->getVertices();
    if (corners.size() != QUAD_CORNERS)
      return;
    MShape::ptr mesh(new Mesh(corners.at(0), corners.at(1), corners.at(2), corners.at(3)));
    mesh->setLocked(output->isLocked());
    layer->setShape(mesh);
    output = mesh;
  }

  // The input shape covers the whole image, as for a new layer, so that it looks the same.
  const QRectF frame = texture->getRect();
  MShape::ptr input;
  switch (output->getType())
  {
  case MShape::Mesh:
  {
    QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(output);
    Mesh* inputMesh = Util::createMeshForRect(frame);
    inputMesh->resize(outputMesh->nColumns(), outputMesh->nRows());
    input.reset(inputMesh);
    break;
  }
  case MShape::Triangle:
    input.reset(Util::createTriangleForRect(frame));
    break;
  case MShape::Ellipse:
    input.reset(Util::createEllipseForRect(frame));
    break;
  default:
    return; // unusableLayerReason() says why.
  }
  layer->setInputShape(input);
}

Layer::ptr ProjectReader::parseLayer(const QJsonObject& obj)
{
  QString className = Serializable::classNameCleanToReal(obj[ProjectLabels::CLASS_NAME].toString());
  int id            = obj[ProjectLabels::ID].toInt(NULL_UID);

  if (id <= NULL_UID || Layer::getUidAllocator().exists(id))
    return Layer::ptr();

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);

  // A colour layer of an earlier version is a texture layer: its source is now a colour
  // generator. It is given an input shape below.
  bool legacyColorLayer = false;
  for (const char* legacyClass : LEGACY_COLOR_LAYER_CLASSES)
    legacyColorLayer = legacyColorLayer
        || className == Serializable::classNameCleanToReal(QLatin1String(legacyClass));
  if (legacyColorLayer)
    metaObject = &TextureLayer::staticMetaObject;

  if (metaObject && metaObject->inherits(&Layer::staticMetaObject))
  {
    Layer::ptr layer(qobject_cast<Layer*>(metaObject->newInstance(Q_ARG(int, id))));
    if (layer.isNull())
    {
      // Registered type that could not be instantiated. Report and bail out
      // instead of dereferencing a null layer.
      qWarning() << "Unable to create layer of type" << className;
      return Layer::ptr();
    }

    // Resolve the source first: reading the shapes may depend on it.
    // The setting knows the key of its value, also in the projects of earlier versions.
    const Setting* sourceSetting = layer->settings()->find(ProjectLabels::SOURCE_ID);
    uid sourceId = obj[Serializable::projectKey(obj, sourceSetting)].toInt(NULL_UID);
    layer->setSource(_manager.getSourceById(sourceId));
    if (layer->getSource().isNull())
    {
      // Missing or incompatible source (eg. a macOS-only Syphon source that
      // was skipped on another platform). Skip the layer rather than keep a
      // layer without a source, which cannot be drawn nor saved.
      qWarning() << "Skipping layer" << id << "of type" << className
                 << ": no compatible source with id" << sourceId;
      return Layer::ptr();
    }

    layer->read(obj);
    if (legacyColorLayer)
      giveInputShapeToColorLayer(layer);

    // A hand-written or damaged file can describe a layer MapMap cannot show.
    // Skip it with a warning, like an unsupported source, rather than crash
    // later when the interface builds its editor.
    const QString problem = unusableLayerReason(layer);
    if (!problem.isEmpty())
    {
      qWarning() << "Skipping layer" << id << ":" << problem;
      return Layer::ptr();
    }
    return layer;
  }
  else
  {
    qWarning() << "Unable to create layer of type" << className;
    return Layer::ptr();
  }
}

}
