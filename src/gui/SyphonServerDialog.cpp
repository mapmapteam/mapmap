/*
 * SyphonServerDialog.cpp
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

#include "SyphonServerDialog.h"

#ifdef HAVE_SYPHON

#include <QListWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QStringList>

namespace mmp {

SyphonServerDialog::SyphonServerDialog(QWidget* parent)
  : QDialog(parent),
    _list(nullptr),
    _statusLabel(nullptr),
    _timer(nullptr)
{
  setWindowTitle(tr("Add Syphon Source"));
  setMinimumWidth(380);

  QVBoxLayout* layout = new QVBoxLayout(this);

  QLabel* label = new QLabel(tr("Choose a Syphon server to receive video from:"), this);
  label->setWordWrap(true);
  layout->addWidget(label);

  _list = new QListWidget(this);
  layout->addWidget(_list);

  // Live status line (updated by refresh()); the list refreshes automatically.
  _statusLabel = new QLabel(this);
  _statusLabel->setWordWrap(true);
  _statusLabel->setEnabled(false); // muted look
  layout->addWidget(_statusLabel);

  QDialogButtonBox* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  connect(_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));

  refresh();

  // Syphon servers can appear/disappear at any time; keep the list current.
  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(refresh()));
  _timer->start(1000);
}

void SyphonServerDialog::refresh()
{
  const int prevRow = _list->currentRow();
  QString selectedUuid;
  if (prevRow > 0 && (prevRow - 1) < _servers.size())
    selectedUuid = _servers[prevRow - 1].uuid;

  _servers = Syphon::availableServers();

  _list->blockSignals(true);
  _list->clear();
  _list->addItem(tr("(Create without connecting yet)"));
  for (int i = 0; i < _servers.size(); ++i)
  {
    QString text = _servers[i].displayName();
    if (text.isEmpty())
      text = tr("Unknown server");

    QListWidgetItem* item = new QListWidgetItem(text, _list);
    if (!_servers[i].icon.isNull())
      item->setIcon(QIcon(QPixmap::fromImage(_servers[i].icon)));

    // Tooltip with the full identity (app, server name, UUID).
    QStringList tip;
    if (!_servers[i].appName.isEmpty()) tip << tr("Application: %1").arg(_servers[i].appName);
    if (!_servers[i].name.isEmpty())    tip << tr("Server: %1").arg(_servers[i].name);
    if (!_servers[i].uuid.isEmpty())    tip << tr("ID: %1").arg(_servers[i].uuid);
    item->setToolTip(tip.join('\n'));
  }

  // Update the live status line (also serves as the "no servers" empty state).
  if (_servers.isEmpty())
    _statusLabel->setText(tr("No Syphon servers found yet. Open a Syphon-enabled "
                             "app (Resolume, VDMX, a Simple Server, …), or create "
                             "the source now and connect it later."));
  else
    _statusLabel->setText(tr("%n Syphon server(s) available. The list updates "
                             "automatically.", "", _servers.size()));

  // Restore a sensible selection.
  int restore;
  if (prevRow < 0)
    restore = (_servers.size() > 0) ? 1 : 0; // initial open: first server, if any
  else if (prevRow == 0)
    restore = 0;                             // user explicitly chose "unbound"
  else
  {
    restore = 0;                             // previously-selected server, if still present
    for (int i = 0; i < _servers.size(); ++i)
      if (_servers[i].uuid == selectedUuid) { restore = i + 1; break; }
  }
  _list->setCurrentRow(restore);
  _list->blockSignals(false);
}

bool SyphonServerDialog::hasSelection() const
{
  return _list->currentRow() > 0;
}

SyphonServerDescription SyphonServerDialog::selectedServer() const
{
  const int row = _list->currentRow();
  if (row > 0 && (row - 1) < _servers.size())
    return _servers[row - 1];
  return SyphonServerDescription();
}

}

#endif // HAVE_SYPHON
