/*
 * OscInterface.cpp
 *
 * Copyright (c) 2010 Alexandre Quessy <alexandre@quessy.net>
 * Copyright (c) 2010 Tristan Matthews <le.businessman@gmail.com>
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

#ifdef HAVE_OSC
#include "OscInterface.h"
#include "MainWindow.h"
#include <QVariant>

MM_BEGIN_NAMESPACE

const QString OscInterface::OSC_ROOT("mapmap");
const QString OscInterface::OSC_PAINT("paint");
const QString OscInterface::OSC_MAPPING("mapping");
const QString OscInterface::OSC_QUIT("quit");
const QString OscInterface::OSC_PLAY("play");
const QString OscInterface::OSC_PAUSE("pause");
const QString OscInterface::OSC_REWIND("rewind");

const QString OscInterface::OSC_PAINT_MEDIA("media");
const QString OscInterface::OSC_PAINT_COLOR("color");

OscInterface::OscInterface(
//        MainWindow* owner,
    const std::string &listen_port) :
    receiver_(listen_port),
//    owner_(owner),
    messaging_queue_() {
  //if (listen_port != OSC_PORT_NONE)
  receiving_enabled_ = true;
  if (receiving_enabled_) {
    qDebug() << "Listening osc.udp://localhost:" << listen_port.c_str();
    // receiver_.addHandler("/ping", "", ping_cb, this);
    // receiver_.addHandler("/pong", "", pong_cb, this);
    //receiver_.addHandler("/image/path", "ss", image_path_cb, this);
    receiver_.addHandler(NULL, NULL, genericHandler, this);
  }
}

OscInterface::~OscInterface() {
  // pass
}

/**
 * Handles /pong. Does nothing.
 */
int OscInterface::pong_cb(const char *path, const char * /*types*/,
    lo_arg ** /*argv*/, int /*argc*/, void * /*data*/, void *user_data) {
  OscInterface* context = static_cast<OscInterface*>(user_data);
  if (context->is_verbose())
    qDebug() << "Got " << path;
  return 0;
}

/**
 * Handles /ping. Does nothing.
 */
int OscInterface::ping_cb(const char *path, const char * /*types*/,
    lo_arg ** /*argv*/, int /*argc*/, void * /*data*/, void *user_data) {
  OscInterface* context = static_cast<OscInterface*>(user_data);
  if (context->is_verbose())
    qDebug() << "Got " << path;
  return 0;
}

void OscInterface::push_command(QVariantList command) {
  messaging_queue_.push(command);
}

void OscInterface::consume_commands(MainWindow &main_window) {
  bool success = true;
  while (success) {
    QVariantList command;
    success = messaging_queue_.try_pop(command);
    if (success) {
      //if (is_verbose())
      // std::cout << __FUNCTION__ << ": apply " <<
      //     command.first().toString().toStdString() << std::endl;
      this->applyOscCommand(main_window, command);
    }
  }
}

void OscInterface::start() {
  if (receiving_enabled_) {
    // start a thread to try and subscribe us
    receiver_.listen(); // start listening in separate thread
  }
}

// catch any incoming messages and display them. returning 1 means that the
// message has not been fully handled and the server should try other methods
int OscInterface::genericHandler(const char *path, const char *types,
    lo_arg **argv, int argc, void * /*data*/, void * user_data) {
  OscInterface* context = static_cast<OscInterface*>(user_data);
  QVariantList message;

  message.append(QVariant(QString(path)));
  message.append(QVariant(QString(types)));

  for (int i = 0; i < argc; ++i) {
    switch (types[i]) {
    case 'i':
      message.append(QVariant(argv[i]->i));
      break;
    case 'f':
      message.append(QVariant((double) argv[i]->f));
      break;
    case 's':
      message.append(QVariant(QString(static_cast<const char *>(&argv[i]->s))));
      break;
    case 'd':
      message.append(QVariant((double) argv[i]->d));
      break;
    case 'T':
      message.append(QVariant(true));
      break;
    case 'F':
      message.append(QVariant(false));
      break;
    default:
      break;
    }
  }
  context->push_command(message);
  return 0; // handled
}

static void printCommand(QVariantList &command) {
  for (int i = 0; i < command.size(); ++i) {
    if (command.at(i).type() == QVariant::Int) {
      qDebug() << command.at(i).toInt() << " ";
    } else if (command.at(i).type() == QVariant::Double) {
      qDebug() << command.at(i).toDouble() << " ";
    } else if (command.at(i).type() == QVariant::String) {
      qDebug() << command.at(i).toString() << " ";
    } else {
      qDebug() << "(?) ";
    }
  }
  qDebug() << endl;
}

void OscInterface::applyOscCommand(MainWindow &main_window, QVariantList & command) {
  Q_UNUSED(main_window);
  bool VERBOSE = true;

  if (VERBOSE) {
    std::cout << "OscInterface::applyOscCommand: Receive OSC: " << std::endl;
    printCommand(command);
  }

  // The two first QVariant objects are: path, typeTags
  if (command.size() < 2)
    return;
  if (command.at(0).type() != QVariant::String)
    return;
  if (command.at(1).type() != QVariant::String)
    return;

  QString path     = command.at(0).toString();
  QString typetags = command.at(1).toString();

  bool pathIsValid = false;
  QPair<QString,QString> iterator = next(path);

  if (iterator.first.isEmpty())
  {
    // Check root tag.
    iterator = next(iterator.second);
    if (iterator.first == OSC_ROOT)
    {
      // Check type.
      iterator = next(iterator.second);
      if (iterator.first == OSC_PAINT)
      {
        // Find paint.
        int id = command.at(2).toInt();
        Paint::ptr elem = main_window.getMappingManager().getPaintById(id);
        pathIsValid = setElementProperty(elem, next(iterator.second).first, command.at(3));
      }
      else if (iterator.first == OSC_MAPPING)
      {
        // Find mapping.
        int id = command.at(2).toInt();
        Mapping::ptr elem = main_window.getMappingManager().getMappingById(id);
        pathIsValid = setElementProperty(elem, next(iterator.second).first, command.at(3));
      }
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

  if (!pathIsValid)
  {
    qDebug() << "Path could not be processed: " << path << endl;
    printCommand(command);
  }

}

QPair<QString,QString> OscInterface::next(const QString& path)
{
  int idx = path.indexOf('/');
  if (idx >= 0)
    return QPair<QString,QString>(path.left(idx), path.right(path.size()-idx-1));
  else
    return QPair<QString,QString>(path, "");

}

bool OscInterface::setElementProperty(const QSharedPointer<Element>& elem, const QString& property, const QVariant& value)
{
  if (elem.isNull())
    return false;
  else
    return elem->setProperty(property.toUtf8().data(), value);
}

MM_END_NAMESPACE

#endif // HAVE_OSC
