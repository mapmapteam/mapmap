/*
 * SourceGui.cpp
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

#include <SourceGui.h>
#include "ProjectLabels.h"
#include "Generator.h"

#ifdef HAVE_SYPHON
#include <QAbstractItemView>
#include <QLabel>
#include <QTimer>
#endif

namespace mmp {

namespace {

/// Key of the tab of a source in the preferences, where its folded groups are kept.
const QString SOURCE_PAGE_KEY = QStringLiteral("source");

#ifdef HAVE_SYPHON
/// Delay between two searches of the Syphon servers, in milliseconds.
const int SYPHON_REFRESH_INTERVAL_MS = 1000;
#endif

} // namespace

SourceGui::SourceGui(Source::ptr source)
  : _source(source),
    _inspector(new Inspector),
    _page(nullptr)
{
  _inspector->setHeader(_source->getName(), kindOf(_source));
  // The inspector is made from the settings and follows them.
  _page = _inspector->addPage(tr("Media"), _source->settings(), SOURCE_PAGE_KEY);
  connect(_inspector, &Inspector::edited, this, [this]() { emit valueChanged(_source); });
  connect(_source.data(), &Source::settingChanged, _page,
          [this](uid, const QString& path, const QVariant&) {
            _page->refresh(path);
            if (path == QLatin1String("name"))
              _inspector->setHeader(_source->getName(), kindOf(_source));
          });
  connect(_source.data(), &Source::settingsStructureChanged, _page, &InspectorPage::rebuild);
}

SourceGui::~SourceGui()
{
  delete _inspector;
}

QWidget* SourceGui::getPropertiesEditor()
{
  return _inspector;
}

QString SourceGui::kindOf(const Source::ptr& source)
{
  if (!source)
    return QString();
  if (qobject_cast<Camera*>(source.data()))
    return tr("Camera");
  switch (source->getSourceType()) {
  case Source::Video:  return tr("Video");
  case Source::Image:  return tr("Image");
  case Source::Color:  return tr("Color");
  case Source::Syphon: return tr("Syphon");
  case Source::Generator: {
    const Generator* generator = qobject_cast<const Generator*>(source.data());
    return generator ? generator->generatorName() : tr("Generator");
  }
  }
  return QString();
}

#ifdef HAVE_SYPHON
SyphonGui::SyphonGui(Source::ptr source)
  : SourceGui(source),
    _serverMenu(new RefreshingComboBox),
    _statusLabel(new QLabel),
    _updatingMenu(false),
    _refreshTimer(nullptr)
{
  syphon = qSharedPointerCast<Syphon>(source);
  Q_CHECK_PTR(syphon);

  // Below the rows made from the settings ("Respect source alpha" among them). The menu
  // searches the servers again each time it opens.
  _serverMenu->setAboutToShow([this]() { _rebuildServerMenu(); });
  _page->addExtraRow(tr("Server"), _serverMenu);
  _statusLabel->setEnabled(false);
  _page->addExtraRow(tr("Status"), _statusLabel);

  connect(_serverMenu, QOverload<int>::of(&QComboBox::activated), this, &SyphonGui::_chooseServer);

  // The server may also be changed by OSC, MCP or a project file.
  connect(_source.data(), &Source::settingChanged, this,
          [this](uid, const QString& path, const QVariant&) {
            if (path == ProjectLabels::SERVER_UUID || path == ProjectLabels::SERVER_NAME ||
                path == ProjectLabels::APP_NAME)
              _rebuildServerMenu();
          });

  // Fill the menu and the state now, then keep them current. Syphon has no Qt-native
  // change signal, so the shared directory is polled on a timer.
  refreshServers();

  _refreshTimer = new QTimer(this);
  connect(_refreshTimer, &QTimer::timeout, this, &SyphonGui::refreshServers);
  _refreshTimer->start(SYPHON_REFRESH_INTERVAL_MS);
}

void SyphonGui::_rebuildServerMenu()
{
  _updatingMenu = true;

  _servers = Syphon::availableServers();

  const QString boundUuid = syphon->getServerUUID();
  const QString boundName = syphon->getServerName();
  const QString boundApp  = syphon->getAppName();
  const bool hasBound = !(boundUuid.isEmpty() && boundName.isEmpty() && boundApp.isEmpty());

  auto matchesBound = [&](const SyphonServerDescription& s) {
    if (!boundUuid.isEmpty())
      return s.uuid == boundUuid;
    return s.name == boundName && s.appName == boundApp;
  };

  // Make sure the bound server is always selectable, even if currently offline.
  bool boundPresent = false;
  for (const SyphonServerDescription& s : _servers)
    if (matchesBound(s)) { boundPresent = true; break; }
  if (hasBound && !boundPresent)
  {
    SyphonServerDescription bound;
    bound.uuid = boundUuid;
    bound.name = boundName;
    bound.appName = boundApp;
    _servers.prepend(bound);
  }

  _serverMenu->clear();
  _serverMenu->addItem(tr("(none)"));
  int currentIndex = 0;
  for (int i = 0; i < _servers.size(); ++i)
  {
    QString label = _servers[i].displayName();
    if (label.isEmpty())
      label = tr("Unknown server");
    _serverMenu->addItem(label);
    if (hasBound && matchesBound(_servers[i]))
      currentIndex = i + 1;
  }
  _serverMenu->setCurrentIndex(currentIndex);

  _updatingMenu = false;
}

void SyphonGui::_updateStatus()
{
  QString status;
  if (syphon->isConnected())
    status = tr("Connected");
  else if (syphon->getServerUUID().isEmpty() &&
           syphon->getServerName().isEmpty() &&
           syphon->getAppName().isEmpty())
    status = tr("No server selected");
  else
    status = tr("Waiting for server…");
  _statusLabel->setText(status);
}

void SyphonGui::refreshServers()
{
  // Only rebuild the menu when the set of available servers changes, so as not to
  // disrupt the user (e.g. close an open menu) every tick.
  QStringList signature;
  const QList<SyphonServerDescription> servers = Syphon::availableServers();
  for (const SyphonServerDescription& s : servers)
    signature << (s.uuid + "|" + s.appName + "|" + s.name);
  signature.sort();

  if (signature != _lastSignature && !_serverMenu->view()->isVisible())
  {
    _lastSignature = signature;
    _rebuildServerMenu();
  }

  _updateStatus();
}

void SyphonGui::_chooseServer(int index)
{
  if (_updatingMenu)
    return; // A rebuild of the menu, not a choice of the user.

  if (index <= 0)
    syphon->connectToServer(QString(), QString(), QString()); // "(none)"
  else if (index - 1 < _servers.size())
  {
    const SyphonServerDescription& s = _servers[index - 1];
    syphon->connectToServer(s.uuid, s.name, s.appName);
  }
  emit valueChanged(_source);
  _updateStatus();
}
#endif // HAVE_SYPHON

}
