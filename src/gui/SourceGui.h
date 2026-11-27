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
class QLabel;
class QTimer;
#endif

#include "Inspector.h"

namespace mmp {

class RefreshingComboBox;

/**
 * The view components corresponding to a Source (which is the model) in the interface:
 * an inspector made from the settings of the source, which follows their changes.
 *
 * In other words the SourceGui is to Source what LayerGui is to Layer.
 */
class SourceGui : public QObject {
  Q_OBJECT

public:
  typedef QSharedPointer<SourceGui> ptr;

public:
  /// Makes the inspector of a source.
  SourceGui(Source::ptr source);
  virtual ~SourceGui();

public:
  /// Returns the inspector, shown in the Library tab.
  virtual QWidget* getPropertiesEditor();

  /// Returns the translated kind of a source, such as "Video", shown in the inspector.
  static QString kindOf(const Source::ptr& source);

signals:
  void valueChanged(Source::ptr);

protected:
  Source::ptr _source;
  Inspector* _inspector;
  /// The tab of the source.
  InspectorPage* _page;
};

#ifdef HAVE_SYPHON
/**
 * The inspector of a Syphon source, with two rows made by hand under the rows of its
 * settings: the menu of the servers, searched again each time it opens, and the state of
 * the connection.
 */
class SyphonGui : public SourceGui {
  Q_OBJECT

public:
  /// Makes the inspector of a Syphon source.
  SyphonGui(Source::ptr source);
  virtual ~SyphonGui() {}

private slots:
  /// Polls the Syphon directory and refreshes the menu and the state.
  void refreshServers();

protected:
  /// Fills the menu with the servers, the bound one first when it is offline.
  void _rebuildServerMenu();

  /// Shows the state of the connection.
  void _updateStatus();

  /// Connects to the server chosen in the menu.
  void _chooseServer(int index);

  QSharedPointer<Syphon> syphon;
  RefreshingComboBox* _serverMenu;
  QLabel* _statusLabel;

  // _servers[i] corresponds to menu index i+1 (index 0 is "(none)").
  QList<SyphonServerDescription> _servers;
  QStringList _lastSignature;
  bool _updatingMenu;
  QTimer* _refreshTimer;
};
#endif // HAVE_SYPHON

}

#endif /* SOURCEGUI_H_ */
