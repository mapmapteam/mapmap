/*
 * PaintGui.cpp
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

#include <PaintGui.h>

PaintGui::PaintGui(Paint::ptr paint)
  : _paint(paint)
{
  // Create editor.
  _propertyBrowser = new QtTreePropertyBrowser;
  _variantManager = new VariantManager;
  _variantFactory = new VariantFactory;

  _topItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                          QObject::tr("Paint"));

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));

  _propertyBrowser->addProperty(_topItem);

  // Paint basic properties.
  _opacityItem = _variantManager->addProperty(QVariant::Double, QObject::tr("Opacity (%)"));
  _opacityItem->setAttribute("minimum", 0.0);
  _opacityItem->setAttribute("maximum", 100.0);
  _opacityItem->setAttribute("decimals", 1);
  _opacityItem->setValue(_paint->getOpacity()*100.0);
  _topItem->addSubProperty(_opacityItem);
}

PaintGui::~PaintGui()
{
  delete _propertyBrowser;
}

QWidget* PaintGui::getPropertiesEditor()
{
  return _propertyBrowser;
}

void PaintGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _opacityItem)
  {
    double opacity = qBound(value.toDouble() / 100.0, 0.0, 1.0);
    if (opacity != _paint->getOpacity())
    {
      _paint->setOpacity(opacity);
      emit valueChanged(_paint);
    }
  }
}


ColorGui::ColorGui(Paint::ptr paint)
  : PaintGui(paint)
{
  color = qSharedPointerCast<Color>(paint);
  Q_CHECK_PTR(color);

  _colorItem = _variantManager->addProperty(QVariant::Color,
                                            QObject::tr("Color"));

  _colorItem->setValue(color->getColor());

  _topItem->addSubProperty(_colorItem);
}

void ColorGui::setValue(QtProperty* property, const QVariant& value) {
  if (property == _colorItem) {
    color->setColor(value.value<QColor>());
    emit valueChanged(_paint);
  }
  else
    PaintGui::setValue(property, value);
}

TextureGui::TextureGui(Paint::ptr paint) : PaintGui(paint) {
}

ImageGui::ImageGui(Paint::ptr paint)
  : TextureGui(paint)
{
  image = qSharedPointerCast<Image>(paint);
  Q_CHECK_PTR(image);

_imageFileItem = _variantManager->addProperty(VariantManager::filePathTypeId(),
                                              tr("Image file"));

_imageFileItem->setAttribute("filter", tr("Image files (%1);;All files (*)").arg(MM::IMAGE_FILES_FILTER));
_imageFileItem->setValue(image->getUri());

_topItem->addSubProperty(_imageFileItem);
}

void ImageGui::setValue(QtProperty* property, const QVariant& value) {
  if (property == _imageFileItem) {
    image->setUri(value.toString());
    emit valueChanged(_paint);
  }
  else
    PaintGui::setValue(property, value);
}

MediaGui::MediaGui(Paint::ptr paint)
: TextureGui(paint)
{
  media = qSharedPointerCast<Media>(paint);
  Q_CHECK_PTR(media);

  _mediaFileItem = _variantManager->addProperty(VariantManager::filePathTypeId(),
                                                tr("Media file"));

  _mediaFileItem->setAttribute("filter", tr("Video files (%1);;All files (*)").arg(MM::VIDEO_FILES_FILTER));
  _mediaFileItem->setValue(media->getUri());

  _mediaRateItem = _variantManager->addProperty(QVariant::Double,
                                                tr("Speed (%)"));
  double rate = media->getRate(); // we need to save it because the call to setAttribute will set it to minimum
  _mediaRateItem->setAttribute("minimum", 1);
  _mediaRateItem->setAttribute("decimals", 1);
  _mediaRateItem->setValue(rate);
  
  _mediaVolumeItem = _variantManager->addProperty(QVariant::Double,
                                                tr("Volume (%)"));
  _mediaVolumeItem->setAttribute("minimum", 0.0);
  _mediaVolumeItem->setAttribute("maximum", 100.0);
  _mediaVolumeItem->setAttribute("decimals", 1);
  _mediaVolumeItem->setValue(volume);

//  _mediaReverseItem = _variantManager->addProperty(QVariant::Bool,
//                                                tr("Reverse"));
//  _mediaReverseItem->setValue(false);

  _topItem->addSubProperty(_mediaFileItem);
  _topItem->addSubProperty(_mediaRateItem);
  _topItem->addSubProperty(_mediaVolumeItem);
//  _topItem->addSubProperty(_mediaReverseItem);
}

void MediaGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _mediaFileItem)
  {
    media->setUri(value.toString());
    emit valueChanged(_paint);
  }
  else if (property == _mediaRateItem)
  {
    double rateSign = (media->getRate() <= 0 ? -1 : +1);
    media->setRate(value.toDouble());
    emit valueChanged(_paint);
  }
//    else if (property == _mediaReverseItem)
//    {
//      double absoluteRate = abs( media->getRate() );
//      media->setRate( (value.toBool() ? -1 : +1) * absoluteRate );
//      emit valueChanged(_paint);
//    }
  else if (property == _mediaVolumeItem)
  {
    media->setVolume(value.toDouble());
    emit valueChanged(_paint);
  }
  else
    PaintGui::setValue(property, value);
}
