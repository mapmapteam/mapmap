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

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

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

namespace mmp {

/**
 * The view components corresponding to a Source (which is the model) in the interface.
 * Mainly manages the property browser for the Source.
 *
 * In other words the SourceGui is to Source what MappingGui is to Mapping.
 */
class SourceGui : public QObject {
  Q_OBJECT

public:
  typedef QSharedPointer<SourceGui> ptr;

public:
  // TODO: should be protected
  /// Constructor. A source gui applies to a source.
  SourceGui(Source::ptr source);
  virtual ~SourceGui();

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor();

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

signals:
  void valueChanged(Source::ptr);

protected:
  Source::ptr _source;
  QtAbstractPropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;

  QtVariantProperty* _idItem;
  QtVariantProperty* _opacityItem;
};

class ColorGui : public SourceGui {
  Q_OBJECT

public:
  ColorGui(Source::ptr source);
  virtual ~ColorGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

protected:
  QSharedPointer<Color> color;
  QtVariantProperty* _colorItem;
};

class TextureGui : public SourceGui {
  Q_OBJECT

public:
  TextureGui(Source::ptr source);
  virtual ~TextureGui() {}
};

class ImageGui : public TextureGui {
  Q_OBJECT

public:
  ImageGui(Source::ptr source);
  virtual ~ImageGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

protected:
  QSharedPointer<Image> image;
  QtVariantProperty* _imageFileItem;
  QtVariantProperty* _imageRateItem;
};

class VideoGui : public TextureGui {
  Q_OBJECT

public:
  VideoGui(Source::ptr source);
  virtual ~VideoGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

protected:
  QSharedPointer<Video> media;
  QtVariantProperty* _mediaFileItem;
  QtVariantProperty* _mediaRateItem;
  QtVariantProperty* _mediaVolumeItem;
//  QtVariantProperty* _mediaReverseItem;
};

#ifdef HAVE_SYPHON
/**
 * Property editor for a Syphon source (macOS only). Exposes a live-refreshed
 * "Server" dropdown so the source can be re-pointed at any available Syphon
 * server, plus a read-only connection status.
 */
class SyphonGui : public TextureGui {
  Q_OBJECT

public:
  SyphonGui(Source::ptr source);
  virtual ~SyphonGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

private slots:
  /// Polls the Syphon directory and refreshes the dropdown/status.
  void refreshServers();

protected:
  void _rebuildServerEnum();
  void _updateStatus();

  QSharedPointer<Syphon> syphon;
  QtVariantProperty* _serverItem;
  QtVariantProperty* _statusItem;
  QtVariantProperty* _alphaItem;

  // _servers[i] corresponds to dropdown index i+1 (index 0 is "(none)").
  QList<SyphonServerDescription> _servers;
  QStringList _lastSignature;
  bool _updatingEnum;
  QTimer* _refreshTimer;
};
#endif // HAVE_SYPHON

}

#endif /* SOURCEGUI_H_ */
