/*
 * Mapping.cpp
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

#include "Mapping.h"
#include "MainWindow.h"

UidAllocator Mapping::allocator;

Mapping::Mapping(Paint::ptr paint, MShape::ptr shape, uid id)
  : Element(id, &allocator),
    _paint(paint), _shape(shape),
    _isSolo(false), _isVisible(true)
{
  // Default.
  _depth = getId();
}

Mapping::~Mapping() {
  allocator.free(getId());
}

void Mapping::read(const QDomElement& obj)
{
  // Read basic data.
  Element::read(obj);

  // Read paint.
  int paintId = obj.attribute("paintId").toInt();
  setPaint(MainWindow::instance()->getMappingManager().getPaintById(paintId));

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

  // Write paint ID.
  obj.setAttribute("paintId", getPaint()->getId());

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
  QString tag       = isOutput ? "output" : "input";

  QDomElement shapeObj = obj.firstChildElement(tag);

  QString className = shapeObj.attribute("className");

  qDebug() << "Found shape with classname: " << className << endl;

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
    else
      qDebug() << "Created new shape" << endl;

    // Read shape.
    shape->read(shapeObj);

    // Set shape.
    if (isOutput)
      setShape(shape);
    else
      qDebug() << "Shit!!!!" << endl;

  }

  else
  {
    qDebug() << QObject::tr("Unable to create paint of type '%1'.").arg(className) << endl;
  }

}

void Mapping::_writeShape(QDomElement& obj, bool isOutput)
{
  QString tag       = isOutput ? "output" : "input";
  MShape::ptr shape = isOutput ? getShape() : getInputShape();
  QDomElement shapeObj = obj.ownerDocument().createElement(tag);
  shape->write(shapeObj);
  obj.appendChild(shapeObj);
}
