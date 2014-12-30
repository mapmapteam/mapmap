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

#include <tr1/memory>

#include "MM.h"

#include "Paint.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "variantmanager.h"
#include "variantfactory.h"

class PaintGui : public QObject {
  Q_OBJECT

public:
  typedef std::tr1::shared_ptr<PaintGui> ptr;

public:
  // TODO: should be protected
  /// Constructor. A paint gui applies to a paint.
  PaintGui(Paint::ptr paint);
  virtual ~PaintGui();

public:
  /// Returns a pointer to the properties editor for that mapper.
  virtual QWidget* getPropertiesEditor();

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value) {
    Q_UNUSED(property);
    Q_UNUSED(value);
  }

signals:
  void valueChanged(Paint::ptr);

protected:
  Paint::ptr _paint;
  QtAbstractPropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
  QtProperty* _topItem;
};

class ColorGui : public PaintGui {
  Q_OBJECT

public:
  ColorGui(Paint::ptr paint);
  virtual ~ColorGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

protected:
  std::tr1::shared_ptr<Color> color;
  QtVariantProperty* _colorItem;
};

class TextureGui : public PaintGui {
  Q_OBJECT

public:
  TextureGui(Paint::ptr paint);
  virtual ~TextureGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value) = 0;
};

class ImageGui : public TextureGui {
  Q_OBJECT

public:
  ImageGui(Paint::ptr paint);
  virtual ~ImageGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

protected:
  std::tr1::shared_ptr<Image> image;
  QtVariantProperty* _imageFileItem;
};

class MediaGui : public TextureGui {
  Q_OBJECT

public:
  MediaGui(Paint::ptr paint);
  virtual ~MediaGui() {}

public slots:
  virtual void setValue(QtProperty* property, const QVariant& value);

protected:
  std::tr1::shared_ptr<Media> media;
  QtVariantProperty* _mediaFileItem;
  QtVariantProperty* _mediaRateItem;
//  QtVariantProperty* _mediaReverseItem;
};

#endif /* PAINTGUI_H_ */
