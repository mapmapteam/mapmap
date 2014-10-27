/*
 * OutputGLWindow.cpp
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

#include "PreferencesDialog.h"
#include "MainWindow.h"
#include <iostream>

PreferencesDialog::PreferencesDialog(MainWindow* mainWindow, QWidget* parent) :
    QDialog(parent)
{
  _main_window = mainWindow;
  // static const int width = 600;
  // static const int height = 400;
  // resize(MainWindow::OUTPUT_WINDOW_MINIMUM_WIDTH, MainWindow::OUTPUT_WINDOW_MINIMUM_HEIGHT);

  // OSC port number
  QLabel* osc_port_label = new QLabel(tr("OSC port number"));

  _osc_port_numbox  = new QSpinBox;
  _osc_port_numbox->setRange(1024, 65534);

  QLayout* hbox_osc_port = new QHBoxLayout;
  hbox_osc_port->addWidget(osc_port_label);
  hbox_osc_port->addWidget(_osc_port_numbox);

  // cancel / ok
  _button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(_button_box, SIGNAL(accepted()), this, SLOT(accept_cb()));
  connect(_button_box, SIGNAL(rejected()), this, SLOT(reject_cb()));

  //QLayout* hbox_confirm = new QHBoxLayout;
  //hbox_confirm->addWidget(_button_box);
  hbox_osc_port->addWidget(_button_box);

  // pack all rows
  //QLayout* vbox = new QVBoxLayout;
  //vbox->addWidget(hbox_osc_port);
  setLayout(hbox_osc_port);

  this->resetValues();
}

void PreferencesDialog::closeEvent(QCloseEvent *event)
{
  emit closed();
  event->accept();
}

void PreferencesDialog::reject_cb()
{
  this->resetValues();
  this->setResult(QMessageBox::Cancel);
  this->close();
}

void PreferencesDialog::resetValues()
{
  this->_osc_port_numbox->setValue(this->_main_window->getOscPort());
}

void PreferencesDialog::accept_cb()
{
  std::cout << "Values:" <<
    " osc port=" << _osc_port_numbox->value() <<
    std::endl;
  this->_main_window->setOscPort(this->_osc_port_numbox->value());
  this->setResult(QMessageBox::Ok);
  this->close();
}

