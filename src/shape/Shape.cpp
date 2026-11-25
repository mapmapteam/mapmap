/*
 * Shape.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "Shape.h"

namespace mmp {

/// Id of the display group of the vertices.
static const QString VERTICES_DISPLAY_GROUP = "vertices";
/// Position of the display group of the vertices among those of a shape.
static const int VERTICES_DISPLAY_ORDER = 0;

MShape::MShape() : _isLocked(false), _verticesSettings(nullptr)
{
  _declareSettings();
}

MShape::MShape(const QVector<QPointF>& vertices_) : _isLocked(false), _verticesSettings(nullptr)
{
  _declareSettings();
  setVertices(vertices_);
  build();
}

void MShape::_declareSettings()
{
  SettingGroup* root = settings();

  root->addSetting(SettingInfo::boolean("locked", tr("Locked"))
                   .hidden()
                   .description(tr("Whether the vertices of this shape are protected from changes made with the mouse.")))
      ->bind([this]() { return QVariant(isLocked()); },
             [this](const QVariant& value) { setLocked(value.toBool()); return true; });

  // Points are set while mapping, not played live: their group starts folded.
  root->describeDisplayGroup(VERTICES_DISPLAY_GROUP, tr("Points"), VERTICES_DISPLAY_ORDER, true);
  _verticesSettings = root->addList("vertices",
                                    SettingInfo::point("vertex", tr("Point"))
                                    .unit(SettingInfo::Pixels)
                                    .notStored()
                                    .displayGroup(VERTICES_DISPLAY_GROUP)
                                    .description(tr("The position of a control point of the shape, in pixels.")),
                                    [this]() { return nVertices(); },
                                    [this](int index) { return QVariant(getVertex(index)); },
                                    [this](int index, const QVariant& value) {
                                      setVertex(index, value.toPointF());
                                      return true;
                                    });
}

void MShape::setLocked(bool locked)
{
  if (locked != _isLocked)
  {
    _isLocked = locked;
    settings()->find("locked")->notifyChanged();
  }
}

void MShape::setVertices(const QVector<QPointF>& vertices_)
{
  // Deep copy.
  vertices.resize(vertices_.size());
  std::copy(vertices_.begin(), vertices_.end(), vertices.begin());
  _verticesChanged();
}

void MShape::_verticesChanged()
{
  _verticesSettings->sync();
  _verticesSettings->notifyChanged();
}

void MShape::_vertexChanged(int i)
{
  // The list must follow the number of vertices before one of them is notified.
  if (_verticesSettings->count() != nVertices())
  {
    _verticesChanged();
    return;
  }
  Setting* vertex = _verticesSettings->at(i);
  if (vertex)
    vertex->notifyChanged();
}

void MShape::_verticesMoved(const QVector<QPointF>& before)
{
  if (before.size() != nVertices() || _verticesSettings->count() != nVertices())
  {
    _verticesChanged();
    return;
  }
  for (int i = 0; i < nVertices(); i++)
  {
    if (before.at(i) != vertices.at(i))
      _vertexChanged(i);
  }
}

void MShape::copyFrom(const MShape& shape)
{
  // Just copy vertices.
  setVertices(shape.getVertices());
}

MShape* MShape::clone() const {
  MShape* copyShape = _create();
  copyShape->copyFrom(*this);
  return copyShape;
}

void MShape::applyTransform(const QTransform& transform)
{
  for (QVector<QPointF>::iterator it = vertices.begin();
       it != vertices.end(); ++it)
    (*it) = transform.map(*it);
  _verticesChanged();
}

void MShape::transform(const QPointF& translate, qreal scale, qreal rotate)
{
	const QPointF& center = getCenter();

	// Create transformation.
	QTransform transform;

	// Center and perform rotate + scale, then de-center.
	transform.translate(+center.x(), +center.y());
	if (rotate)
		transform.rotate(rotate);
	if (scale)
		transform.scale(scale, scale);
	transform.translate(-center.x(), -center.y());

	// Apply translation.
	if (!translate.isNull())
		transform.translate(translate.x(), translate.y());

	// Perform operation.
	applyTransform(transform);
}

void MShape::translate(const QPointF& offset)
{
	transform(offset, 0, 0);
}

void MShape::rotate(qreal angle)
{
	transform(QPointF(), 0, angle);
}

void MShape::scale(qreal scaling)
{
	transform(QPointF(), scaling, 0);
}

QRectF MShape::getBoundingRect() const
{
  return QPolygonF(vertices).boundingRect();
}

QTransform MShape::flipTransform(FlipDirection direction, bool keepBounds) const
{
  const QPointF center = keepBounds ? getBoundingRect().center() : getCenter();

  QTransform transform;
  transform.translate(center.x(), center.y());
  if (direction == Horizontal)
    transform.scale(-1, 1);
  else
    transform.scale(1, -1);
  transform.translate(-center.x(), -center.y());
  return transform;
}

QTransform MShape::rotateTransform(Rotation rotation, bool keepBounds) const
{
  const QRectF bounds = getBoundingRect();
  const QPointF center = keepBounds ? bounds.center() : getCenter();

  qreal angle = 0;
  switch (rotation)
  {
  case Rotate90CW:  angle =  90; break;
  case Rotate90CCW: angle = -90; break;
  case Rotate180:   angle = 180; break;
  }

  // QTransform applies the last-added operation first: rotate about the
  // center, then (for quarter turns that keep bounds) stretch the rotated
  // bounding box (height x width) back to width x height.
  QTransform transform;
  transform.translate(center.x(), center.y());
  if (keepBounds && rotation != Rotate180
      && bounds.width() > 0 && bounds.height() > 0)
    transform.scale(bounds.width() / bounds.height(),
                    bounds.height() / bounds.width());
  transform.rotate(angle);
  transform.translate(-center.x(), -center.y());
  return transform;
}

QTransform MShape::imageFlipTransform(FlipDirection direction) const
{
  return flipTransform(direction, true);
}

QTransform MShape::imageRotateTransform(Rotation rotation) const
{
  return rotateTransform(inverseRotation(rotation), true);
}

MShape::Rotation MShape::inverseRotation(Rotation rotation)
{
  switch (rotation)
  {
  case Rotate90CW:  return Rotate90CCW;
  case Rotate90CCW: return Rotate90CW;
  case Rotate180:   return Rotate180;
  }
  return rotation;
}

QPointF MShape::getCenter() const
{
  QPointF center;
  for (QVector<QPointF>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
    center += (*it);
  center /= nVertices();
  return center;
}

void MShape::read(const QJsonObject& obj)
{
  Serializable::read(obj);

  // Read vertices.
  QJsonArray verticesArray = obj["vertices"].toArray();
  QVector<QPointF> vertices;
  for (const auto& v : verticesArray)
  {
    const QJsonArray vertex = v.toArray();
    if (vertex.size() < 2)
    {
      qWarning() << "Skipping malformed vertex (expected [x, y]) while reading shape.";
      continue;
    }
    vertices.append(QPointF(vertex.at(0).toDouble(), vertex.at(1).toDouble()));
  }

  setVertices(vertices);
  build();
}

void MShape::write(QJsonObject& obj)
{
  Serializable::write(obj);

  // Write vertices.
  QJsonArray verticesArray;
  for (int i=0; i<nVertices(); i++)
  {
    QJsonArray vertex;
    vertex.append(getVertex(i).x());
    vertex.append(getVertex(i).y());
    verticesArray.append(vertex);
  }

  obj["vertices"] = verticesArray;
}

}
