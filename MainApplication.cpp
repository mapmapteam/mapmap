/*
 * MainApplication.cpp
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
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

MM_BEGIN_NAMESPACE

MainApplication::MainApplication(int &argc, char *argv[])
  : QApplication(argc, argv)
{
  // Initialize GStreamer.
  gst_init (NULL, NULL);

  // Set application information.
  setApplicationName(MM::APPLICATION_NAME);
  setApplicationVersion(MM::VERSION);
  setOrganizationName(MM::ORGANIZATION_NAME);
  setOrganizationDomain(MM::ORGANIZATION_DOMAIN);
}

MainApplication::~MainApplication()
{
  // Deinitialize GStreamer.
  gst_deinit();
}

bool MainApplication::notify(QObject *receiver, QEvent *event)
{
  try
  {
    return QApplication::notify(receiver, event);
  }
  catch (std::exception &ex)
  {
    qDebug() << "std::exception was caught: " << ex.what() << endl;
    qDebug() << "event type: " << event->type() << endl;
  }

  return false;
}

MM_END_NAMESPACE
