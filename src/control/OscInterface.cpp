/*
 * OscInterface.cpp
 *
 * Copyright (c) 2010 Alexandre Quessy <alexandre@quessy.net>
 * Copyright (c) 2010 Tristan Matthews <le.businessman@gmail.com>
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2020 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#include "OscInterface.h"
#include "MainWindow.h"
#include "RemoteSettingWrite.h"
#include <QVariant>
#include <QPointF>

namespace mmp {

namespace {

/// Returns the source with the given id, or a null pointer, WITHOUT mutating
/// the manager (MappingManager::getSourceById() would insert a null entry for
/// an unknown id, since QMap::operator[] is non-const there).
Source::ptr findSourceById(MappingManager& manager, int id)
{
  for (int i = 0; i < manager.nSources(); ++i)
  {
    Source::ptr source = manager.getSource(i);
    if (!source.isNull() && static_cast<int>(source->getId()) == id)
      return source;
  }
  return Source::ptr();
}

/// Resolves the source(s) targeted by an action (by id or by name pattern).
QVector<Source::ptr> resolveSources(MappingManager& manager, const OscAction& action)
{
  if (action.selectByName)
    return manager.getSourcesByNameRegExp(action.name);

  QVector<Source::ptr> sources;
  Source::ptr source = findSourceById(manager, action.id);
  if (!source.isNull())
    sources.push_back(source);
  return sources;
}

/// Resolves the layer(s) targeted by an action (by id or by name pattern).
QVector<Layer::ptr> resolveLayers(MappingManager& manager, const OscAction& action)
{
  if (action.selectByName)
    return manager.getLayersByNameRegExp(action.name);

  QVector<Layer::ptr> layers;
  Layer::ptr layer = manager.getLayerById(action.id);
  if (!layer.isNull())
    layers.push_back(layer);
  return layers;
}

/// Writes OSC arguments to the setting of an element at path. Returns false, with a
/// warning, when the path names no setting (a group, a list or nothing) or when the
/// setting refuses the arguments.
bool writeSetting(Element& element, const QString& path, const QVariantList& arguments)
{
  Setting* setting = element.settings()->find(path);
  if (!setting)
  {
    qWarning() << "OSC:" << element.metaObject()->className() << element.getId()
               << "has no setting at" << path;
    return false;
  }

  return writeRemoteSetting(element, setting, arguments, sameMediaSourceAllowed());
}

} // namespace

OscInterface::OscInterface(
    int listen_port, bool acceptFromNetwork) :
    receiver_(listen_port, acceptFromNetwork),
    messaging_queue_() {
  receiving_enabled_ = true;
  if (receiving_enabled_) {
    qDebug() << "Listening osc.udp://localhost:" << listen_port;
    // setup handler
    QObject::connect(&receiver_, &OscReceiver::messageReceived, [=](const QString& oscAddress, const QVariantList& arguments) {
      this->messageReceivedCb(oscAddress, arguments);
    });
  }
}


OscInterface::~OscInterface() {
  // pass
}

void OscInterface::push_command(QVariantList command)
{
  messaging_queue_.push(command);
}

void OscInterface::consume_commands(MainWindow &main_window)
{
  bool success = true;
  while (success)
  {
    QVariantList command;
    success = messaging_queue_.try_pop(command);
    if (success)
    {
      this->applyOscCommand(main_window, command);
      main_window.notifyActivity();
    }
  }
}

void OscInterface::start()
{
}

void OscInterface::messageReceivedCb(const QString& oscAddress, const QVariantList& arguments) {
  QVariantList command;
  command.append(QVariant(oscAddress));

  QString types = "";
  for (int i = 0; i < arguments.count(); ++ i) {
    QVariant argument = arguments[i];
    QMetaType::Type type = static_cast<QMetaType::Type>(argument.typeId());

    if (type == QMetaType::Int) {
      types += "i";
    } else if (type == QMetaType::Float) {
      types += "f";
    } else if (type == QMetaType::Double) {
      types += "f";
    } else if (type == QMetaType::QString) {
      types += "s";
    } else if (type == QMetaType::Bool) {
      if (argument.toBool()) {
        types += "T";
      } else {
        types += "F";
      }
    } else {
      qDebug() << "Unhandled OSC argument type " << argument.typeName();
    }
    // TODO: implement other OSC types
  }
  command.append(QVariant(types));

  for (int i = 0; i < arguments.size(); ++i)
  {
    QVariant argument = arguments[i];
    command.append(argument);
  }

  this->push_command(command);
}

static void printCommand(QVariantList &command)
{
  for (int i = 0; i < command.size(); ++i)
  {
    if (command.at(i).typeId() == QMetaType::Int)
    {
      qDebug() << command.at(i).toInt() << " ";
    }
    else if (command.at(i).typeId() == QMetaType::Double)
    {
      qDebug() << command.at(i).toDouble() << " ";
    }
    else if (command.at(i).typeId() == QMetaType::QString)
    {
      qDebug() << command.at(i).toString() << " ";
    }
    else
    {
      qDebug() << "(?) ";
    }
  }
  qDebug() << Qt::endl;
}

void OscInterface::applyOscCommand(MainWindow &main_window, QVariantList & command) {
  if (is_verbose())
  {
    std::cout << "OscInterface::applyOscCommand: Receive OSC: " << std::endl;
    printCommand(command);
  }

  // The two first QVariant objects are: path, typeTags. The rest are the args.
  if (command.size() < 2)
    return;
  if (command.at(0).typeId() != QMetaType::QString)
    return;
  if (command.at(1).typeId() != QMetaType::QString)
    return;

  const QString path = command.at(0).toString();
  const QVariantList args = command.mid(2);

  OscAction action = parseOscAction(path, args);
  bool handled = applyAction(main_window, action);

  if (!handled && is_verbose())
  {
    qDebug() << "OSC path could not be processed: " << path
             << (action.isValid() ? QString("(no matching target)") : action.error)
             << Qt::endl;
    printCommand(command);
  }
}

bool OscInterface::applyAction(MainWindow &main_window, const OscAction& action)
{
  MappingManager& manager = main_window.getMappingManager();

  switch (action.type)
  {
  case OscAction::Invalid:
    return false;

  // Global transport.
  case OscAction::Quit:
    if (main_window.isOscQuitAllowed()) { main_window.close(); return true; }
    qWarning() << "Ignoring OSC /mapmap/quit: remote quit is disabled "
                  "(enable it in Preferences > OSC Setup).";
    return false;
  case OscAction::PlayAll:   main_window.play();   return true;
  case OscAction::PauseAll:  main_window.pause();  return true;
  case OscAction::RewindAll: main_window.rewind(); return true;

  // Per-source commands.
  case OscAction::SourcePlay:
  case OscAction::SourcePause:
  case OscAction::SourceRewind:
  case OscAction::SourceProperty:
  case OscAction::SourceSetting:
  {
    bool handled = false;
    const QVector<Source::ptr> sources = resolveSources(manager, action);
    if (sources.isEmpty() && action.type == OscAction::SourceSetting)
      qWarning() << "OSC: no source with id" << action.id;
    for (Source::ptr source : sources)
    {
      if (source.isNull())
        continue;
      switch (action.type)
      {
      case OscAction::SourcePlay:     source->play();   handled = true; break;
      case OscAction::SourcePause:    source->pause();  handled = true; break;
      case OscAction::SourceRewind:   source->rewind(); handled = true; break;
      case OscAction::SourceProperty: handled |= writeSetting(*source, action.property, { action.value }); break;
      case OscAction::SourceSetting:  handled |= writeSetting(*source, action.path, action.values); break;
      default: break;
      }
    }
    return handled;
  }

  // Per-layer commands.
  case OscAction::LayerProperty:
  case OscAction::LayerSetting:
  case OscAction::LayerMove:
  case OscAction::LayerTranslate:
  case OscAction::LayerVertex:
  case OscAction::LayerFlip:
  case OscAction::LayerRotate:
  {
    bool handled = false;
    const QVector<Layer::ptr> layers = resolveLayers(manager, action);
    if (layers.isEmpty() && action.type == OscAction::LayerSetting)
      qWarning() << "OSC: no layer with id" << action.id;
    for (Layer::ptr layer : layers)
    {
      if (layer.isNull())
        continue;

      if (action.type == OscAction::LayerProperty)
      {
        handled |= writeSetting(*layer, action.property, { action.value });
        continue;
      }
      if (action.type == OscAction::LayerSetting)
      {
        handled |= writeSetting(*layer, action.path, action.values);
        continue;
      }

      // The shape that move/translate/vertex operate on.
      MShape::ptr shape = (action.shapeRole == OscAction::InputShape)
          ? layer->getInputShape()
          : layer->getShape();
      if (shape.isNull())
        continue;

      switch (action.type)
      {
      case OscAction::LayerMove:
      {
        // Absolute: translate so the shape's center lands on (x, y).
        const QPointF center = shape->getCenter();
        shape->translate(QPointF(action.x - center.x(), action.y - center.y()));
        handled = true;
        break;
      }
      case OscAction::LayerTranslate:
        shape->translate(QPointF(action.x, action.y));
        handled = true;
        break;
      case OscAction::LayerVertex:
        if (action.vertexIndex >= 0 && action.vertexIndex < shape->nVertices())
        {
          shape->setVertex(action.vertexIndex, action.x, action.y);
          handled = true;
        }
        break;
      case OscAction::LayerFlip:
      {
        // Same transforms as the Edit menu commands (see FlipShapeCommand).
        const MShape::FlipDirection direction = (action.flipDirection == OscAction::FlipHorizontal)
            ? MShape::Horizontal : MShape::Vertical;
        shape->applyTransform(action.shapeRole == OscAction::InputShape
                              ? shape->imageFlipTransform(direction)
                              : shape->flipTransform(direction));
        handled = true;
        break;
      }
      case OscAction::LayerRotate:
      {
        MShape::Rotation rotation = MShape::Rotate180;
        if (action.turn == OscAction::Turn90CW)
          rotation = MShape::Rotate90CW;
        else if (action.turn == OscAction::Turn90CCW)
          rotation = MShape::Rotate90CCW;
        shape->applyTransform(action.shapeRole == OscAction::InputShape
                              ? shape->imageRotateTransform(rotation)
                              : shape->rotateTransform(rotation));
        handled = true;
        break;
      }
      default:
        break;
      }
    }
    return handled;
  }
  }

  return false;
}

}
