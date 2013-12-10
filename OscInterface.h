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

#include <tr1/memory>
#include "concurrentqueue.h"
#include "OscReceiver.h"
#include <QVariant>

class MainWindow; // forward decl

/**
 * Open Sound Control sending and receiving for LibreMapping.
 */
class OscInterface 
{
  public:
    typedef std::tr1::shared_ptr<OscInterface> ptr;
    OscInterface(
      MainWindow* owner, 
      const std::string &listen_port);
    ~OscInterface() {}
    void start();
    void consume_commands();
    bool is_verbose() { return false; }
  private:
    void push_command(QVariantList command);
    /**
     * OSC callback
     */
    static int image_path_cb(const char *path, 
      const char *types, lo_arg **argv, 
      int argc, void *data, void *user_data);
    static int ping_cb(const char *path, 
            const char *types, lo_arg **argv, 
            int argc, void *data, void *user_data);
    static int pong_cb(const char *path, 
            const char *types, lo_arg **argv, 
            int argc, void *data, void *user_data);
    static int genericHandler(const char *path, 
            const char *types, lo_arg **argv, 
            int argc, void *data, void *user_data);

    bool receiving_enabled_;
    OscReceiver receiver_;
    MainWindow* owner_;
    ConcurrentQueue<QVariantList> messaging_queue_;
};

#endif /* include guard */
