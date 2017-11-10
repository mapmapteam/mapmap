/*
 * PaintGui.h
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

#ifndef PAINTGUI_H_
#define PAINTGUI_H_


#include <QtGlobal>

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "MM.h"

#include "Paint.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

namespace mmp {

/**
 * The view components corresponding to a Paint (which is the model) in the interface.
 * Mainly manages the property browser for the Paint.
 *
 * In other words the PaintGui is to Paint what MappingGui is to Mapping.
 */
class PaintGui : public QObject {
  Q_OBJECT

public:
  typedef QSharedPointer<PaintGui> ptr;

public:
  // TODO: should be protected
  /// Constructor. A paint gui applies to a paint.
  PaintGui(Paint::ptr paint);
  virtual ~PaintGui();

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor();

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

signals:
  void valueChanged(Paint::ptr);

protected:
  Paint::ptr _paint;
  QtAbstractPropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;

  QtVariantProperty* _idItem;
  QtVariantProperty* _opacityItem;
};

class ColorGui : public PaintGui {
  Q_OBJECT

public:
  ColorGui(Paint::ptr paint);
  virtual ~ColorGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);
  virtual void setValue(QString propertyName, QVariant value);

protected:
  QSharedPointer<Color> color;
  QtVariantProperty* _colorItem;
};

class TextureGui : public PaintGui {
  Q_OBJECT

public:
  TextureGui(Paint::ptr paint);
  virtual ~TextureGui() {}
};

class ImageGui : public TextureGui {
  Q_OBJECT

public:
  ImageGui(Paint::ptr paint);
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
  VideoGui(Paint::ptr paint);
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

}

#endif /* PAINTGUI_H_ */
