/*
 * OscInterface.h
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

#ifndef OSC_INTERFACE_H_
#define OSC_INTERFACE_H_

// Compiler flag (?)
#ifdef HAVE_OSC

#include <QVariant>
#include <QMessageLogger>

#include "ConcurrentQueue.h"
#include "OscReceiver.h"

namespace mmp {

class MainWindow;
class Element;

/**
 * Open Sound Control sending and receiving for MapMap.
 */
class OscInterface {
public:
  typedef QSharedPointer<OscInterface> ptr;

  static const QString OSC_ROOT;
  static const QString OSC_PAINT;
  static const QString OSC_MAPPING;
  static const QString OSC_QUIT;
  static const QString OSC_PLAY;
  static const QString OSC_PAUSE;
  static const QString OSC_REWIND;

  static const QString OSC_PAINT_MEDIA;
  static const QString OSC_PAINT_COLOR;

  OscInterface(const std::string &listen_port);
  ~OscInterface();

  /// Starts listening if receiving is enabled.
  void start();

  /**
   * Takes action!
   * Should be called when it's time to take action, before rendering a frame, for example.
   * Call this method from the main thread.
   * Each message is stored as a QVariantList.
   * <path> <typeTags> [args]
   */
  void consume_commands(MainWindow &main_window);

private:
  bool is_verbose() const { return false; }

  void push_command(QVariantList command);

  // OSC callbacks
  static int ping_cb(const char *path, const char *types, lo_arg **argv,
      int argc, void *data, void *user_data);
  static int pong_cb(const char *path, const char *types, lo_arg **argv,
      int argc, void *data, void *user_data);
  static int genericHandler(const char *path, const char *types, lo_arg **argv,
      int argc, void *data, void *user_data);

  bool receiving_enabled_;
  OscReceiver receiver_;
  //MainWindow* owner_;

  ConcurrentQueue<QVariantList> messaging_queue_;

  // In the main thread, handles the messages.
  void applyOscCommand(MainWindow &main_window, QVariantList & command);

  // For path = "path_item/rest_of_path" returns (path_item, rest_of_path).
  static QPair<QString,QString> next(const QString& path);

  // Sets property on element with given value.
  bool setElementProperty(const QSharedPointer<Element>& elem, const QString& property, const QVariant& value);
};

}

#endif // HAVE_OSC

#endif /* include guard */
