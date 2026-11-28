/*
 * LayerGui.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#include "LayerGui.h"

#include "ProjectLabels.h"

namespace mmp {

namespace {

/// Key of the Surface tab in the preferences, where its folded groups are kept.
const QString LAYER_PAGE_KEY = QStringLiteral("layer");

/// Key of the Media tab in the preferences; the same as the inspector of a source.
const QString SOURCE_PAGE_KEY = QStringLiteral("source");

} // namespace

LayerGui::LayerGui(Layer::ptr layer)
  : _layer(layer),
    _graphicsItem(nullptr),
    _inputGraphicsItem(nullptr)
{
  outputShape = layer->getShape();
  Q_CHECK_PTR(outputShape);

  // The inspector is made from the settings of the layer and of its shapes, and of its
  // source, and follows them.
  _inspector.reset(new Inspector);
  _surfacePage = _inspector->addPage(tr("Surface"), _layer->settings(), LAYER_PAGE_KEY);
  _surfacePage->setGroupLabel("output", tr("Output shape"));
  _surfacePage->setGroupLabel("input", tr("Input shape"));
  _mediaPage = _inspector->addPage(tr("Media"), nullptr, SOURCE_PAGE_KEY);
  connect(_layer.data(), &Layer::settingChanged, _surfacePage,
          [this](uid, const QString& path, const QVariant&) {
            _surfacePage->refresh(path);
            if (path == QLatin1String("name") || path == ProjectLabels::SOURCE_ID)
              _showSource();
          });
  connect(_layer.data(), &Layer::settingsStructureChanged, _surfacePage, &InspectorPage::rebuild);
  _showSource();
}

LayerGui::SourceUseCounter LayerGui::_sourceUseCounter;

void LayerGui::setSourceUseCounter(SourceUseCounter counter)
{
  _sourceUseCounter = counter;
}

QString LayerGui::kindOf(const Layer::ptr& layer)
{
  if (!layer || !layer->getShape())
    return QString();
  switch (layer->getType()) {
  case MShape::Mesh:     return tr("Mesh");
  case MShape::Triangle: return tr("Triangle");
  case MShape::Ellipse:  return tr("Ellipse");
  case MShape::Quad:     return tr("Quad");
  }
  return QString();
}

void LayerGui::_showSource()
{
  _inspector->setHeader(_layer->getName(), kindOf(_layer));

  const Source::ptr source = _layer->getSource();
  if (source != _shownSource) {
    disconnect(_sourceValueConnection);
    disconnect(_sourceStructureConnection);
    _shownSource = source;
    _mediaPage->setRoot(source ? source->settings() : nullptr);
    if (source) {
      _sourceValueConnection = connect(source.data(), &Source::settingChanged, _mediaPage,
                                       [this](uid, const QString& path, const QVariant&) { _mediaPage->refresh(path); });
      _sourceStructureConnection = connect(source.data(), &Source::settingsStructureChanged,
                                           _mediaPage, &InspectorPage::rebuild);
    }
  }

  // The Media tab changes a source that other layers may use too.
  const int users = (source && _sourceUseCounter) ? _sourceUseCounter(source) : 0;
  _mediaPage->setNotice(users > 1
                        ? tr("Used by %1 layers: changing it changes them all.").arg(users)
                        : QString());
}

void LayerGui::updateSources()
{
  // The menu is computed again from the options of the setting.
  _surfacePage->refresh(ProjectLabels::SOURCE_ID);
  _showSource();
}

TextureLayerGui::TextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : LayerGui(mapping)
{
  // Assign members pointers.
  textureLayer = qSharedPointerCast<TextureLayer>(_layer);
  Q_CHECK_PTR(textureLayer);

  inputShape = textureLayer.toStrongRef()->getInputShape();
  Q_CHECK_PTR(inputShape);
}

TriangleTextureLayerGui::TriangleTextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new TriangleTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new TriangleTextureGraphicsItem(_layer, false));
}

MeshTextureLayerGui::MeshTextureLayerGui(QSharedPointer<TextureLayer> mapping)
  : PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new MeshTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new MeshTextureGraphicsItem(_layer, false));
}

EllipseTextureLayerGui::EllipseTextureLayerGui(QSharedPointer<TextureLayer> mapping)
: PolygonTextureLayerGui(mapping)
{
  _graphicsItem.reset(new EllipseTextureGraphicsItem(_layer, true));
  _inputGraphicsItem.reset(new EllipseTextureGraphicsItem(_layer, false));
}

}
