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
  _variantManager = new QtVariantPropertyManager;
  _variantFactory = new QtVariantEditorFactory;

  _topItem = _variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                          QObject::tr("Paint"));

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));

  _propertyBrowser->addProperty(_topItem);
}

PaintGui::~PaintGui()
{
  delete _propertyBrowser;
}

QWidget* PaintGui::getPropertiesEditor()
{
  return _propertyBrowser;
}

ColorGui::ColorGui(Paint::ptr paint)
  : PaintGui(paint)
{
  color = std::tr1::static_pointer_cast<Color>(paint);
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
}
