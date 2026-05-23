/*
 * OscInterface.cpp
 *
 * Copyright (c) 2010 Alexandre Quessy <alexandre@quessy.net>
 * Copyright (c) 2010 Tristan Matthews <le.businessman@gmail.com>
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2020 Alexandre Quessy -- alexandre(@)quessy(.)net
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
#include <QVariant>

namespace mmp {

static const QString OSC_ROOT("mapmap");
static const QString OSC_SOURCE("source");
static const QString OSC_LAYER("layer");
static const QString OSC_QUIT("quit");
static const QString OSC_PLAY("play");
static const QString OSC_PAUSE("pause");
static const QString OSC_REWIND("rewind");

static const QString OSC_SOURCE_MEDIA("media");
static const QString OSC_SOURCE_COLOR("color");

OscInterface::OscInterface(
    int listen_port) :
    receiver_(listen_port),
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
      //if (is_verbose())
      // std::cout << __FUNCTION__ << ": apply " <<
      //     command.first().toString().toStdString() << std::endl;
      this->applyOscCommand(main_window, command);
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
  Q_UNUSED(main_window);

  if (is_verbose())
  {
    std::cout << "OscInterface::applyOscCommand: Receive OSC: " << std::endl;
    printCommand(command);
  }

  // The two first QVariant objects are: path, typeTags
  if (command.size() < 2)
  {
    return;
  }
  if (command.at(0).typeId() != QMetaType::QString)
  {
    return;
  }
  if (command.at(1).typeId() != QMetaType::QString)
  {
    return;
  }

  QString path = command.at(0).toString();
  QString typetags = command.at(1).toString();

  bool pathIsValid = false;
  // Walks through each token in the form /mapmap/source/color - The first token is "mapmap", and then "source"
  QPair<QString,QString> iterator = next(path);

  if (iterator.first.isEmpty()) {
    // Check root tag.
    iterator = next(iterator.second);
    if (iterator.first == OSC_ROOT)
    {
      // Check type.
      iterator = next(iterator.second);

      // Source.
      if (iterator.first == OSC_SOURCE)
      {
        // Find source (or sources).
        if (command.size() >= 3)
        {
          QVector<Source::ptr> sources;
          if (command.at(2).typeId() == QMetaType::QString)
            sources = main_window.getMappingManager().getSourcesByNameRegExp(command.at(2).toString());
          else
          {
            int id = command.at(2).toInt();
            sources.push_back(main_window.getMappingManager().getSourceById(id));
          }
          // Process all sources.
          iterator = next(iterator.second);
          for (Source::ptr elem: sources)
          {
            // Rewind.
            if (iterator.first == OSC_REWIND)
            {
              elem->rewind();
              pathIsValid = true;
            }
            // Property setting (eg. opacity)
            else if (command.size() >= 4) {
              if (is_verbose())
                qDebug() << "Attempt to set a source property" << iterator.first << command.at(3);
              pathIsValid |= setElementProperty(elem, iterator.first, command.at(3));
            }
          }
        }
      }

      // Layer.
      else if (iterator.first == OSC_LAYER)
      {
        // Find layer (or layers).
        if (command.size() >= 3)
        {
          QVector<Layer::ptr> layers;
          if (command.at(2).typeId() == QMetaType::QString)
            layers = main_window.getMappingManager().getLayersByNameRegExp(command.at(2).toString());
          else
          {
            int id = command.at(2).toInt();
            Layer::ptr layer = main_window.getMappingManager().getLayerById(id);
            if (!layer.isNull())
              layers.push_back(layer);
          }
          // Process all layers (set property).
          if (command.size() >= 4)
          {
            iterator = next(iterator.second);
            for (Layer::ptr elem: layers)
            {
              pathIsValid |= setElementProperty(elem, iterator.first, command.at(3));
            }
          }
        }
      }

      // Play / pause / rewind / quit.
      else if (iterator.first == OSC_PLAY)
      {
        main_window.play();
      }
      else if (iterator.first == OSC_PAUSE)
      {
        main_window.pause();
      }
      else if (iterator.first == OSC_REWIND)
      {
        main_window.rewind();
      }
      else if (iterator.first == OSC_QUIT)
      {
        main_window.close();
      }
    }
  }

  if (! pathIsValid && is_verbose())
  {
    qDebug() << "Path could not be processed: " << path << Qt::endl;
    printCommand(command);
  }

}

QPair<QString,QString> OscInterface::next(const QString& path)
{
  int idx = path.indexOf('/');
  if (idx >= 0)
  {
    return QPair<QString,QString>(path.left(idx), path.right(path.size() - idx - 1));
  }
  else
  {
    return QPair<QString,QString>(path, "");
  }
}

bool OscInterface::setElementProperty(const QSharedPointer<Element>& elem, const QString& property, const QVariant& value)
{
  if (elem.isNull())
  {
    return false;
  }
  else
  {
    return elem->setProperty(property.toUtf8().data(), value);
  }
}

}

