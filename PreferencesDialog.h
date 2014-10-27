/*
 * OutputGLWindow.h
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

#ifndef PREFERENCESDIALOG_H_
#define PREFERENCESDIALOG_H_

#include <QDialog>
#include <QtGlobal>
#include <QTimer>
#include <QSpinBox>
#include <QDialogButtonBox>

class MainWindow;

class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  PreferencesDialog(MainWindow* mainWindow, QWidget* parent = 0);

protected:
  void closeEvent(QCloseEvent* event);

signals:
  void closed();

private slots:
  void accept_cb();
  void reject_cb();

private:
  QSpinBox* _osc_port_numbox;
  QDialogButtonBox* _button_box;
};

#endif /* PREFERENCESDIALOG_H_ */
