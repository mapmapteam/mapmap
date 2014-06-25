/*
 * ProxyAction.h
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

#ifndef PROXYACTION_H_
#define PROXYACTION_H_

#include <QtGlobal>
#include <QAction>

class ProxyAction: public QAction
{
  Q_OBJECT

public:
  explicit ProxyAction(QAction* mainAction);
  explicit ProxyAction(const QString &text, QAction* mainAction);
  explicit ProxyAction(const QIcon &icon, const QString &text, QAction* mainAction);
  virtual ~ProxyAction();

public slots:

  void syncFromMain();
  void syncToMain();

private:

  void _init();
  static void _sync(QAction* from, QAction* to);
  void _connectAll();
  void _disconnectAll();

  QAction* _mainAction;
};

#endif /* PROXYACTION_H_ */
