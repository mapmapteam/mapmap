/*
 * WelcomeDialog.h
 *
 * A small first-run onboarding dialog: it greets the user, points at the two
 * first steps (import a media file, read the documentation) and lets them opt
 * out of showing it on future launches.
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

#ifndef WELCOME_DIALOG_H_
#define WELCOME_DIALOG_H_

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace mmp {

class WelcomeDialog : public QDialog
{
  Q_OBJECT

public:
  explicit WelcomeDialog(QWidget* parent = nullptr);

  /// Whether the welcome dialog should be shown automatically on launch.
  static bool showOnStartup();

signals:
  /// Emitted when the user chooses to import a media file from the dialog.
  void importMediaRequested();

private:
  QCheckBox* _showOnStartupBox;
};

}

#endif /* WELCOME_DIALOG_H_ */
