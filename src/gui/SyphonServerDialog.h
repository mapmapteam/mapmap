/*
 * SyphonServerDialog.h
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef SYPHON_SERVER_DIALOG_H_
#define SYPHON_SERVER_DIALOG_H_

#include <QtGlobal>

#ifdef HAVE_SYPHON

#include <QDialog>
#include <QList>

#include "Syphon.h"

class QListWidget;
class QTimer;

namespace mmp {

/**
 * Modal picker shown when creating a Syphon source. Lists the Syphon servers
 * currently available on the system (refreshed live) and lets the user either
 * pick one or create the source unbound, to be connected later.
 */
class SyphonServerDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SyphonServerDialog(QWidget* parent = nullptr);

  /// True if the user selected an actual server (rather than "create unbound").
  bool hasSelection() const;

  /// The chosen server, or an empty description if created unbound.
  SyphonServerDescription selectedServer() const;

private slots:
  void refresh();

private:
  QListWidget* _list;
  QTimer* _timer;
  QList<SyphonServerDescription> _servers;
};

}

#endif // HAVE_SYPHON

#endif /* SYPHON_SERVER_DIALOG_H_ */
