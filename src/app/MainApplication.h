/*
 * MainApplication.h
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef MAINAPPLICATION_H_
#define MAINAPPLICATION_H_

#include <gst/gst.h>
#include <QApplication>
#include <QDebug>
#include "MM.h"
#include <QSettings>
#include <QDir>

namespace mmp {

class MainApplication : public QApplication
{
public:
  MainApplication(int &argc, char *argv[]);
  virtual ~MainApplication();

  bool notify(QObject *receiver, QEvent *event);
};

}

#endif /* MAINAPPLICATION_H_ */
