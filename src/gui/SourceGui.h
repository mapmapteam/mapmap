/*
 * SourceGui.h
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

#ifndef SOURCEGUI_H_
#define SOURCEGUI_H_


#include <QtGlobal>

#include <QtGui/qopengl.h>

#include "MM.h"

#include "Source.h"

#ifdef HAVE_SYPHON
#include "Syphon.h"
class QTimer;
#endif

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

#include "SettingsBrowser.h"

namespace mmp {

/**
 * The view components corresponding to a Source (which is the model) in the interface:
 * the property browser of the source, made from its settings by a SettingsBrowser, which
 * follows the changes of the settings.
 *
 * In other words the SourceGui is to Source what LayerGui is to Layer.
 */
class SourceGui : public QObject {
  Q_OBJECT

public:
  typedef QSharedPointer<SourceGui> ptr;

public:
  /// Constructor. A source gui applies to a source.
  SourceGui(Source::ptr source);
  virtual ~SourceGui();

public:
  /// Returns a pointer to the properties editor for that source.
  virtual QWidget* getPropertiesEditor();

  /// Returns the object making the panel from the settings of the source.

signals:
  /// The user changed a setting of the source through the panel.
  void valueChanged(Source::ptr);

protected:
  Source::ptr _source;
  QtTreePropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
  SettingsBrowser* _settingsBrowser;
};

#ifdef HAVE_SYPHON
/**
 * Property editor for a Syphon source (macOS only). Under the rows made from the settings,
 * exposes a live-refreshed "Server" dropdown so the source can be re-pointed at any
 * available Syphon server, plus a read-only connection status.
 */
class SyphonGui : public SourceGui {
  Q_OBJECT

public:
  SyphonGui(Source::ptr source);
  virtual ~SyphonGui() {}

private slots:
  /// Polls the Syphon directory and refreshes the dropdown/status.
  void refreshServers();

  /// Connects to the server chosen in the "Server" row.
  void _chooseServer(QtProperty* property, const QVariant& value);

protected:
  void _rebuildServerEnum();
  void _updateStatus();

  QSharedPointer<Syphon> syphon;
  QtVariantProperty* _serverItem;
  QtVariantProperty* _statusItem;

  // _servers[i] corresponds to dropdown index i+1 (index 0 is "(none)").
  QList<SyphonServerDescription> _servers;
  QStringList _lastSignature;
  bool _updatingEnum;
  QTimer* _refreshTimer;
};
#endif // HAVE_SYPHON

}

#endif /* SOURCEGUI_H_ */
