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

#pragma once

#include <QVariant>
#include <QObject>
#include <QMessageLogger>

#include "ConcurrentQueue.h"
#include "oscreceiver.h"

namespace mmp {

class MainWindow;
class Element;

/**
 * Open Sound Control sending and receiving for MapMap.
 */
class OscInterface {
public:
  typedef QSharedPointer<OscInterface> ptr;

  // FIXME: change listen_port to a int
  OscInterface(int listen_port);
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
  // FIXME use QObject signals instead of polling

private:
  bool is_verbose() const { return false; }
  void push_command(QVariantList command);

  bool receiving_enabled_;
  OscReceiver receiver_;
  ConcurrentQueue<QVariantList> messaging_queue_;

  // In the main thread, handles the messages.
  void applyOscCommand(MainWindow &main_window, QVariantList & command);

  // For path = "path_item/rest_of_path" returns (path_item, rest_of_path).
  static QPair<QString,QString> next(const QString& path);

  void messageReceivedCb(const QString& oscAddress, const QVariantList& arguments);

  // Sets property on element with given value.
  bool setElementProperty(const QSharedPointer<Element>& elem, const QString& property, const QVariant& value);
};

}

