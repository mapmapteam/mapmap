/*
 * Mapping.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
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

#include "Layer.h"
#include "Ellipse.h"

namespace mmp {

UidAllocator Layer::allocator;
Layer::SourceResolver Layer::_sourceResolver;

Layer::Layer(uid id)
: Layer(Source::ptr(), MShape::ptr(), MShape::ptr(), id) {}

Layer::Layer(Source::ptr source, uid id)
: Layer(source, MShape::ptr(), MShape::ptr(), id) {}

Layer::Layer(Source::ptr source, MShape::ptr shape, uid id)
: Layer(source, shape, MShape::ptr(), id) {}

Layer::Layer(Source::ptr source, MShape::ptr shape, MShape::ptr inputShape, uid id)
  : Element(id, &allocator),
    _source(source), _shape(shape), _inputShape(inputShape),
    _isSolo(false), _isVisible(true)
{
  // Default.
  _depth = getId();

  _assignShapeRole(_shape, true);
  _assignShapeRole(_inputShape, false);
}

void Layer::_assignShapeRole(const MShape::ptr& shape, bool isOutput)
{
  // Output ellipses can be distorted in perspective; input ellipses remain axis-aligned.
  Ellipse* ellipse = qobject_cast<Ellipse*>(shape.data());
  if (ellipse)
    ellipse->setPerspective(isOutput);
}

Layer::~Layer() {
  allocator.free(getId());
}

void Layer::setSolo(bool solo)
{
  if (solo != _isSolo)
  {
    _isSolo = solo;
    _emitPropertyChanged("solo");
  }
}

void Layer::setVisible(bool visible)
{
  if (visible != _isVisible)
  {
    _isVisible = visible;
    _emitPropertyChanged("visible");
  }
}

void Layer::setDepth(int depth)
{
  if (depth != _depth)
  {
    _depth = depth;
    _emitPropertyChanged("depth");
  }
}

void Layer::setLocked(bool locked)
{
  if (!_shape.isNull())
    _shape->setLocked(locked);
  if (!_inputShape.isNull())
    _inputShape->setLocked(locked);
  Element::setLocked(locked);
}

void Layer::setSource(Source::ptr source)
{
	if (!source.isNull() && sourceIsCompatible(source))
	{
		_source = source;
	  _emitPropertyChanged("sourceId");
	}
}

void Layer::setSourceById(uid sourceId)
{
  if (_sourceResolver)
    setSource(_sourceResolver(sourceId));
  else
    qWarning() << "Cannot set source by id: no source resolver installed.";
}

void Layer::setSourceResolver(SourceResolver resolver)
{
  _sourceResolver = resolver;
}

void Layer::read(const QJsonObject& obj)
{
  // The source is not read here: its id is resolved by the caller (see
  // ProjectReader), which knows which set of sources this layer belongs to.
  Element::read(obj);

  // Read output shape.
  _readShape(obj, true);

  // Read input shape.
  if (hasInputShape())
  {
    _readShape(obj, false);
  }
}

void Layer::write(QJsonObject& obj)
{
  Element::write(obj);

  obj[ProjectLabels::SOURCE_ID] = getSourceId();

  // Write output shape.
  _writeShape(obj, true);

  // Write input shape.
  if (hasInputShape())
  {
    _writeShape(obj, false);
  }
}

void Layer::_readShape(const QJsonObject& obj, bool isOutput)
{
  QString tag = isOutput ? ProjectLabels::DESTINATION : ProjectLabels::SOURCE;

  QJsonObject shapeObj = obj[tag].toObject();

  QString className = Serializable::classNameCleanToReal(shapeObj[ProjectLabels::CLASS_NAME].toString());

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    MShape::ptr shape(qobject_cast<MShape*>(metaObject->newInstance()));
    if (shape.isNull())
    {
      // Not a shape class (e.g. a source class name in the shape field).
      // Leave the shape unset; ProjectReader then skips this layer.
      qDebug() << QObject::tr("Problem at creation of shape.") << Qt::endl;
      return;
    }

    shape->read(shapeObj);

    if (isOutput)
      setShape(shape);
    else
      setInputShape(shape);
  }
  else
  {
    qDebug() << QObject::tr("Unable to create shape of type '%1'.").arg(className) << Qt::endl;
  }
}

void Layer::_writeShape(QJsonObject& obj, bool isOutput)
{
  QString tag = isOutput ? ProjectLabels::DESTINATION : ProjectLabels::SOURCE;
  MShape::ptr shape = isOutput ? getShape() : getInputShape();
  QJsonObject shapeObj;
  shape->write(shapeObj);
  obj[tag] = shapeObj;
}

bool ColorLayer::sourceIsCompatible(Source::ptr source) const
{
	return source->inherits("mmp::Color");
}

bool TextureLayer::sourceIsCompatible(Source::ptr source) const
{
	return source->inherits("mmp::Texture");
}

}
