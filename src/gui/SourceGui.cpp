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

#ifdef HAVE_SYPHON
#include <QTimer>
#endif

namespace mmp {

SourceGui::SourceGui(Source::ptr source)
  : _source(source)
{
  // Create editor.
  _propertyBrowser = new QtTreePropertyBrowser;
  _variantManager = new VariantManager;
  _variantFactory = new VariantFactory;

  _propertyBrowser->setFactoryForManager(_variantManager, _variantFactory);

  connect(_variantManager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
          this,            SLOT(setValue(QtProperty*, const QVariant&)));

  // Mapping UID.
  _idItem = _variantManager->addProperty(QMetaType::Int, QObject::tr("ID"));
  _idItem->setEnabled(false);
  _idItem->setValue(_source->getId());
  _propertyBrowser->addProperty(_idItem);

  // Source basic properties.
  _opacityItem = _variantManager->addProperty(QMetaType::Double, QObject::tr("Opacity (%)"));
  _opacityItem->setAttribute("minimum", 0.0);
  _opacityItem->setAttribute("maximum", 100.0);
  _opacityItem->setAttribute("decimals", 1);
  _opacityItem->setValue(_source->getOpacity()*100.0);
  _propertyBrowser->addProperty(_opacityItem);
}

SourceGui::~SourceGui()
{
  delete _propertyBrowser;
}

QWidget* SourceGui::getPropertiesEditor()
{
  return _propertyBrowser;
}

void SourceGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _opacityItem)
  {
    double opacity = qBound(value.toDouble() / 100.0, 0.0, 1.0);
    if (opacity != _source->getOpacity())
    {
      _source->setOpacity(opacity);
      emit valueChanged(_source);
    }
  }
}

void SourceGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "opacity")
    _opacityItem->setValue(value.toDouble() * 100);
}

ColorGui::ColorGui(Source::ptr source)
  : SourceGui(source)
{
  color = qSharedPointerCast<Color>(source);
  Q_CHECK_PTR(color);

  _colorItem = _variantManager->addProperty(QMetaType::QColor,
                                            QObject::tr("Color"));

  _colorItem->setValue(color->getColor());

  _propertyBrowser->addProperty(_colorItem);
}

void ColorGui::setValue(QtProperty* property, const QVariant& value) {
  if (property == _colorItem) {
    color->setColor(value.value<QColor>());
    emit valueChanged(_source);
  }
  else
    SourceGui::setValue(property, value);
}

void ColorGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "color")
    setValue(_colorItem, value);
  else
    SourceGui::setValue(propertyName, value);
}

TextureGui::TextureGui(Source::ptr source) : SourceGui(source) {
}

ImageGui::ImageGui(Source::ptr source)
  : TextureGui(source)
{
  image = qSharedPointerCast<Image>(source);
  Q_CHECK_PTR(image);

_imageFileItem = _variantManager->addProperty(VariantManager::filePathTypeId(),
                                              tr("Image file"));

_imageFileItem->setAttribute("filter", tr("Image files (%1);;All files (*)").arg(MM::IMAGE_FILES_FILTER));
_imageFileItem->setValue(image->getUri());

  _imageRateItem = _variantManager->addProperty(QMetaType::Double,
                                                tr("Speed (%)"));
  // we need to save it because the call to setAttribute will set it to minimum
  double rate = image->getRate()*100;
  _imageRateItem->setAttribute("decimals", 1);
  _imageRateItem->setValue(rate);

  _propertyBrowser->addProperty(_imageFileItem);
  _propertyBrowser->addProperty(_imageRateItem);
}

void ImageGui::setValue(QtProperty* property, const QVariant& value) {
  if (property == _imageFileItem) {
    image->setUri(value.toString());
    emit valueChanged(_source);
  }
  else if (property == _imageRateItem)
  {
    //double rateSign = (media->getRate() <= 0 ? -1 : +1);
    image->setRate(value.toDouble()/100.0);
    emit valueChanged(_source);
  }
  else
    TextureGui::setValue(property, value);
}

void ImageGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "uri")
    _imageFileItem->setValue(value);
  else if (propertyName == "rate")
    _imageRateItem->setValue(value.toDouble()*100);
  else
    TextureGui::setValue(propertyName, value);
}

VideoGui::VideoGui(Source::ptr source)
: TextureGui(source)
{
  media = qSharedPointerCast<Video>(source);
  Q_CHECK_PTR(media);

  _mediaFileItem = _variantManager->addProperty(VariantManager::filePathTypeId(),
                                                tr("Source"));

  _mediaFileItem->setAttribute("filter", tr("Video files (%1);;All files (*)").arg(MM::VIDEO_FILES_FILTER));
  _mediaFileItem->setValue(media->getUri());

  _mediaRateItem = _variantManager->addProperty(QMetaType::Double,
                                                tr("Speed (%)"));
  // we need to save it because the call to setAttribute will set it to minimum
  double rate = media->getRate()*100;
  _mediaRateItem->setAttribute("decimals", 1);
  _mediaRateItem->setValue(rate);

  _mediaVolumeItem = _variantManager->addProperty(QMetaType::Double,
                                                tr("Volume (%)"));
  double volume = media->getVolume()*100;
  _mediaVolumeItem->setAttribute("minimum", 0.0);
  _mediaVolumeItem->setAttribute("maximum", 100.0);
  _mediaVolumeItem->setAttribute("decimals", 1);
  _mediaVolumeItem->setValue(volume);

//  _mediaReverseItem = _variantManager->addProperty(QVariant::Bool,
//                                                tr("Reverse"));
//  _mediaReverseItem->setValue(false);

  _propertyBrowser->addProperty(_mediaFileItem);
  _propertyBrowser->addProperty(_mediaRateItem);
  _propertyBrowser->addProperty(_mediaVolumeItem);
//  _propertyBrowser->addProperty(_mediaReverseItem);
}

void VideoGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _mediaFileItem)
  {
    media->setUri(value.toString());
    emit valueChanged(_source);
  }
  else if (property == _mediaRateItem)
  {
    //double rateSign = (media->getRate() <= 0 ? -1 : +1);
    media->setRate(value.toDouble()/100.0);
    emit valueChanged(_source);
  }
//    else if (property == _mediaReverseItem)
//    {
//      double absoluteRate = abs( media->getRate() );
//      media->setRate( (value.toBool() ? -1 : +1) * absoluteRate );
//      emit valueChanged(_source);
//    }
  else if (property == _mediaVolumeItem)
  {
    media->setVolume(value.toDouble()/100.0);
    emit valueChanged(_source);
  }
  else
    TextureGui::setValue(property, value);
}

void VideoGui::setValue(QString propertyName, QVariant value)
{
  if (propertyName == "uri")
    _mediaFileItem->setValue(value);
  if (propertyName == "rate")
    _mediaRateItem->setValue(value.toDouble()*100);
  if (propertyName == "volume")
    _mediaVolumeItem->setValue(value.toDouble()*100);
  else
    TextureGui::setValue(propertyName, value);
}

#ifdef HAVE_SYPHON
SyphonGui::SyphonGui(Source::ptr source)
  : TextureGui(source),
    _updatingEnum(false),
    _refreshTimer(nullptr)
{
  syphon = qSharedPointerCast<Syphon>(source);
  Q_CHECK_PTR(syphon);

  _serverItem = _variantManager->addProperty(QtVariantPropertyManager::enumTypeId(),
                                              tr("Server"));
  _statusItem = _variantManager->addProperty(QMetaType::QString, tr("Status"));
  _statusItem->setEnabled(false);
  _alphaItem = _variantManager->addProperty(QMetaType::Bool, tr("Respect source alpha"));
  _alphaItem->setValue(syphon->getRespectAlpha());

  _propertyBrowser->addProperty(_serverItem);
  _propertyBrowser->addProperty(_statusItem);
  _propertyBrowser->addProperty(_alphaItem);

  // Populate the dropdown and status now, then keep them current. Syphon has
  // no Qt-native change signal, so we poll the shared directory on a timer.
  refreshServers();

  _refreshTimer = new QTimer(this);
  connect(_refreshTimer, SIGNAL(timeout()), this, SLOT(refreshServers()));
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

void SyphonGui::setValue(QtProperty* property, const QVariant& value)
{
  if (property == _serverItem)
  {
    if (_updatingEnum)
      return; // Programmatic rebuild, not a user choice.

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
  else if (property == _alphaItem)
  {
    syphon->setRespectAlpha(value.toBool());
    emit valueChanged(_source);
  }
  else
    TextureGui::setValue(property, value);
}

void SyphonGui::setValue(QString propertyName, QVariant value)
{
  Q_UNUSED(value);
  if (propertyName == "serverUUID" || propertyName == "serverName" || propertyName == "appName")
    _rebuildServerEnum();
  else
    TextureGui::setValue(propertyName, value);
}
#endif // HAVE_SYPHON

}
