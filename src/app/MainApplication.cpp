/*
 * MainApplication.cpp
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

#include "MainApplication.h"

namespace mmp {

MainApplication::MainApplication(int &argc, char *argv[])
  : QApplication(argc, argv)
{
#ifdef Q_OS_WIN32
  // Set settings default format
  QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

  // Set application information.
  setApplicationName(MM::APPLICATION_NAME);
  setApplicationVersion(MM::VERSION);
  setOrganizationName(MM::ORGANIZATION_NAME);
  setOrganizationDomain(MM::ORGANIZATION_DOMAIN);
}

MainApplication::~MainApplication()
{
}

bool MainApplication::notify(QObject *receiver, QEvent *event)
{
  // Last-resort guard: a stray exception during event delivery must never take
  // down a running show. Log loudly (qCritical survives in release builds,
  // unlike qDebug) and drop the offending event instead of terminating.
  const int eventType = event ? static_cast<int>(event->type()) : -1;
  try
  {
    return QApplication::notify(receiver, event);
  }
  catch (const std::exception &ex)
  {
    qCritical() << "Unhandled std::exception during event delivery:" << ex.what()
                << "(event type" << eventType << ")";
  }
  catch (...)
  {
    qCritical() << "Unhandled non-standard exception during event delivery"
                << "(event type" << eventType << ")";
  }

  return false;
}

}
