/*
 * AboutDialog.h
 *
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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#include <gst/gst.h>
#ifdef HAVE_OSC
#include "lo/lo.h"
#endif

#include "GuiForward.h"
#include "MM.h"

namespace mmp {

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog() {}

public slots:


private:
  void createAboutTab();
  void createChangelogTab();
  void createLibrariesTab();
  void createContributorsTab();
  void createLicenseTab();
  void createOscTab();

  QTabWidget *_tabWidget;

  // Constantes
  static const int ABOUT_WINDOW_WIDTH = 560;
  static const int ABOUT_WINDOW_HEIGHT = 640;

};

}

#endif // ABOUTDIALOG_H
