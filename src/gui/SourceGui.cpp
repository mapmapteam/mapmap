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

#ifdef HAVE_SYPHON
#include <QTimer>
#endif

namespace mmp {

SourceGui::SourceGui(Source::ptr source)
  : _source(source)
{
  // Create editor.
  _propertyBrowser = new QtTreePropertyBrowser;
  VariantManager* variantManager = new VariantManager;
  _variantManager = variantManager;
  _variantFactory = new VariantFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  // The panel is made from the settings and follows them.
  _settingsBrowser = new SettingsBrowser(_source->settings(), _propertyBrowser, variantManager, this);
  connect(_settingsBrowser, &SettingsBrowser::edited, this, [this]() { emit valueChanged(_source); });
  connect(_source.data(), &Source::settingChanged, _settingsBrowser,
          [this](uid, const QString& path, const QVariant& value) { _settingsBrowser->refresh(path, value); });
  connect(_source.data(), &Source::settingsStructureChanged, _settingsBrowser, &SettingsBrowser::rebuild);
}

SourceGui::~SourceGui()
{
  delete _propertyBrowser;
}

QWidget* SourceGui::getPropertiesEditor()
{
  return _propertyBrowser;
}

#ifdef HAVE_SYPHON
SyphonGui::SyphonGui(Source::ptr source)
  : SourceGui(source),
    _updatingEnum(false),
    _refreshTimer(nullptr)
{
  syphon = qSharedPointerCast<Syphon>(source);
  Q_CHECK_PTR(syphon);

  // Below the rows made from the settings ("Respect source alpha" among them).
  _serverItem = _variantManager->addProperty(QtVariantPropertyManager::enumTypeId(),
                                              tr("Server"));
  _statusItem = _variantManager->addProperty(QMetaType::QString, tr("Status"));
  _statusItem->setEnabled(false);

  _propertyBrowser->addProperty(_serverItem);
  _propertyBrowser->addProperty(_statusItem);

  connect(_variantManager, &QtVariantPropertyManager::valueChanged, this, &SyphonGui::_chooseServer);

  // The server may also be changed by OSC, MCP or a project file.
  connect(_source.data(), &Source::settingChanged, this,
          [this](uid, const QString& path, const QVariant&) {
            if (path == ProjectLabels::SERVER_UUID || path == ProjectLabels::SERVER_NAME ||
                path == ProjectLabels::APP_NAME)
              _rebuildServerEnum();
          });

  // Populate the dropdown and status now, then keep them current. Syphon has
  // no Qt-native change signal, so we poll the shared directory on a timer.
  refreshServers();

  _refreshTimer = new QTimer(this);
  connect(_refreshTimer, &QTimer::timeout, this, &SyphonGui::refreshServers);
  _refreshTimer->start(1000);
}

void SyphonGui::_rebuildServerEnum()
{
  _updatingEnum = true;

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

  QStringList names;
  names << tr("(none)");
  int currentIndex = 0;
  for (int i = 0; i < _servers.size(); ++i)
  {
    QString label = _servers[i].displayName();
    if (label.isEmpty())
      label = tr("Unknown server");
    names << label;
    if (hasBound && matchesBound(_servers[i]))
      currentIndex = i + 1;
  }

  _serverItem->setAttribute("enumNames", names);
  _serverItem->setValue(currentIndex);

  _updatingEnum = false;
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
  _statusItem->setValue(status);
}

void SyphonGui::refreshServers()
{
  // Only rebuild the dropdown when the set of available servers changes, so we
  // do not disrupt the user (e.g. close an open combo box) every tick.
  QStringList signature;
  const QList<SyphonServerDescription> servers = Syphon::availableServers();
  for (const SyphonServerDescription& s : servers)
    signature << (s.uuid + "|" + s.appName + "|" + s.name);
  signature.sort();

  if (signature != _lastSignature)
  {
    _lastSignature = signature;
    _rebuildServerEnum();
  }

  _updateStatus();
}

void SyphonGui::_chooseServer(QtProperty* property, const QVariant& value)
{
  if (property != _serverItem || _updatingEnum)
    return; // Another row, or a programmatic rebuild, not a user choice.

  const int index = value.toInt();
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
