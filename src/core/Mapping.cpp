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

#include "Mapping.h"
#include "MainWindow.h"

namespace mmp {

UidAllocator Mapping::allocator;

Mapping::Mapping(uid id)
: Mapping(Paint::ptr(), MShape::ptr(), MShape::ptr(), id) {}

Mapping::Mapping(Paint::ptr paint, uid id)
: Mapping(paint, MShape::ptr(), MShape::ptr(), id) {}

Mapping::Mapping(Paint::ptr paint, MShape::ptr shape, uid id)
: Mapping(paint, shape, MShape::ptr(), id) {}

Mapping::Mapping(Paint::ptr paint, MShape::ptr shape, MShape::ptr inputShape, uid id)
  : Element(id, &allocator),
    _paint(paint), _shape(shape), _inputShape(inputShape),
    _isSolo(false), _isVisible(true)
{
  // Default.
  _depth = getId();
}

Mapping::~Mapping() {
  allocator.free(getId());
}

void Mapping::setSolo(bool solo)
{
  if (solo != _isSolo)
  {
    _isSolo = solo;
    _emitPropertyChanged("solo");
  }
}

void Mapping::setVisible(bool visible)
{
  if (visible != _isVisible)
  {
    _isVisible = visible;
    _emitPropertyChanged("visible");
  }
}

void Mapping::setDepth(int depth)
{
  if (depth != _depth)
  {
    _depth = depth;
    _emitPropertyChanged("depth");
  }
}

void Mapping::setLocked(bool locked)
{
  if (!_shape.isNull())
    _shape->setLocked(locked);
  if (!_inputShape.isNull())
    _inputShape->setLocked(locked);
  Element::setLocked(locked);
}

void Mapping::setPaint(Paint::ptr paint)
{
	if (paintIsCompatible(paint))
	{
		_paint = paint;
	  _emitPropertyChanged("paintId");
	}
}

void Mapping::setPaintById(uid paintId)
{
  setPaint(MainWindow::window()->getMappingManager().getPaintById(paintId));
}

void Mapping::read(const QDomElement& obj)
{
  // Read basic data.
  Element::read(obj);

  // // Read paint (stored in attributes for backward compatibility).
  int paintId = obj.attribute(ProjectLabels::PAINT_ID).toInt();
	setPaintById(paintId);

  // Read output shape.
  _readShape(obj, true);

  // Read input shape.
  if (hasInputShape())
  {
    _readShape(obj, false);
  }

}

void Mapping::write(QDomElement& obj)
{
  // Write basic data.
  Element::write(obj);

  // // Write paint ID.
  obj.setAttribute("paintId", getPaintId());

  // Write output shape.
  _writeShape(obj, true);

  // Write input shape.
  if (hasInputShape())
  {
    _writeShape(obj, false);
  }
}

void Mapping::_readShape(const QDomElement& obj, bool isOutput)
{
  QString tag       = isOutput ? ProjectLabels::DESTINATION : ProjectLabels::SOURCE;

  QDomElement shapeObj = obj.firstChildElement(tag);

  QString className = Serializable::classNameCleanToReal(shapeObj.attribute(ProjectLabels::CLASS_NAME));

  const QMetaObject* metaObject = MetaObjectRegistry::instance().getMetaObject(className);
  if (metaObject)
  {
    // Create new instance.
    MShape::ptr shape (qobject_cast<MShape*>(metaObject->newInstance()));
    if (shape.isNull())
    {
      qDebug() << QObject::tr("Problem at creation of shape.") << endl;
//      _xml.raiseError(QObject::tr("Problem at creation of paint."));
    }

    // Read shape.
    shape->read(shapeObj);

    // Set shape.
    if (isOutput)
      setShape(shape);
    else
      setInputShape(shape);
  }

  else
  {
    qDebug() << QObject::tr("Unable to create paint of type '%1'.").arg(className) << endl;
  }

}

void Mapping::_writeShape(QDomElement& obj, bool isOutput)
{
  QString tag       = isOutput ? ProjectLabels::DESTINATION : ProjectLabels::SOURCE;
  MShape::ptr shape = isOutput ? getShape() : getInputShape();
  QDomElement shapeObj = obj.ownerDocument().createElement(tag);
  shape->write(shapeObj);
  obj.appendChild(shapeObj);
}

bool ColorMapping::paintIsCompatible(Paint::ptr paint) const
{
	return paint->inherits("mmp::Color");
}

bool TextureMapping::paintIsCompatible(Paint::ptr paint) const
{
	return paint->inherits("mmp::Texture");
}

}
