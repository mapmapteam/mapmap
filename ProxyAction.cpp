/*
 * ProxyAction.cpp
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
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

#include <ProxyAction.h>

ProxyAction::ProxyAction(QAction* mainAction)
  : QAction(mainAction->parent()),
    _mainAction(mainAction)
{
  _init();
  _connectAll();
}

ProxyAction::ProxyAction(const QString& text, QAction* mainAction)
: QAction(text, mainAction->parent()),
  _mainAction(mainAction)
{
  _init();
  _connectAll();
}

ProxyAction::ProxyAction(const QIcon& icon, const QString& text, QAction* mainAction)
: QAction(icon, text, mainAction->parent()),
  _mainAction(mainAction)
{
  _init();
  _connectAll();
}

ProxyAction::~ProxyAction()
{
  _disconnectAll();
}

#include <QDebug>
void ProxyAction::syncFromMain()
{
  qDebug() << "Sync from main" << endl;
  _sync(_mainAction, this);
}

void ProxyAction::syncToMain()
{
  qDebug() << "Sync to main" << endl;
  _sync(this, _mainAction);
}

void ProxyAction::_init()
{
  qDebug() << "Main action " << _mainAction->toolTip() << " is checkable ? " << _mainAction->isCheckable() << endl;
  setCheckable(_mainAction->isCheckable());
  syncFromMain();
}

void ProxyAction::_sync(QAction* from, QAction* to)
{
  bool checked = from->isChecked();
  bool enabled = from->isEnabled();
  bool visible = from->isVisible();
  to->setChecked(checked);
  to->setEnabled(enabled);
  to->setVisible(visible);
  from->setChecked(checked);
  from->setEnabled(enabled);
  from->setVisible(visible);
}

void ProxyAction::_connectAll()
{
  connect(this, SIGNAL(triggered()), _mainAction, SLOT(trigger()));
  connect(this, SIGNAL(toggled(bool)), this, SLOT(syncToMain()));
//  connect(this, SIGNAL(toggled(bool)), _mainAction, SLOT(setChecked(bool)));
  connect(this, SIGNAL(changed()), this, SLOT(syncToMain()));

//  connect(_mainAction, SIGNAL(triggered()), this, SLOT(trigger()));
  connect(_mainAction, SIGNAL(toggled(bool)), this, SLOT(syncFromMain));
  connect(_mainAction, SIGNAL(changed()), this, SLOT(syncFromMain()));
}

void ProxyAction::_disconnectAll()
{

}
