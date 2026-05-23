/*
 * Mapping.h
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

#ifndef LAYER_H_
#define LAYER_H_

#include <QtGlobal>

#include "Shape.h"
#include "Source.h"

#include "Element.h"

#include "UidAllocator.h"

#include "MetaObjectRegistry.h"

#include "ProjectLabels.h"

namespace mmp {

// TODO: replace by ProjectAttribute
//#include "ProjectWriter.h"

/**
 * Mapping is the central concept of this software.
 *
 * A Mapping represents a relationship between an input Source and
 * and output Shape where the source (possibly modified by some other
 * attributes or an input Shape in the case of TextureMapping) is
 * projected on the output shape.
 *
 * Mapping instances are stacked as layers by the MappingManager. One
 * can thus change their opacity level, toggle their visibility, set
 * them in "solo" mode and lock them.
 */
class Layer : public Element
{
  Q_OBJECT

  Q_PROPERTY(bool solo    READ isSolo    WRITE setSolo)
  Q_PROPERTY(bool visible READ isVisible WRITE setVisible)
  Q_PROPERTY(int  depth   READ getDepth  WRITE setDepth)

//  Q_PROPERTY(MShape::ptr shape READ getShape)
//  Q_PROPERTY(MShape::ptr inputShape READ getInputShape)

  Q_PROPERTY(bool hasInputShape READ hasInputShape STORED false)
  Q_PROPERTY(uid  sourceId  READ getSourceId WRITE setSourceById STORED false)
//  Q_PROPERTY(Source::ptr source READ getSource WRITE setSource)

protected:
  /// The input Source instance.
  Source::ptr _source;

  /// The output Shape instance.
  MShape::ptr _shape;

  /// The (optional) input Shape instance.
  MShape::ptr _inputShape;

private:
  static UidAllocator allocator;

  bool _isSolo;
  bool _isVisible;
  int _depth; // depth of the layer

protected:
  Layer(int id=NULL_UID);
  Layer(Source::ptr source, uid id=NULL_UID);
  Layer(Source::ptr source, MShape::ptr shape, uid id=NULL_UID);
  Layer(Source::ptr source, MShape::ptr shape, MShape::ptr inputShape, uid id=NULL_UID);

public:
  typedef QSharedPointer<Layer> ptr;

  virtual ~Layer();

  static const UidAllocator& getUidAllocator() { return allocator; }

  /**
   * Sets up this Mapping: its Source and its Shape.
   * Calls the build() method of its Source and Shape.
   */
  virtual void build() {
    _source->build();
    _shape->build();
    if (hasInputShape())
      _inputShape->build();
  }

  /// The type of the mapping (expressed as a string).
  virtual MShape::ShapeType getType() const = 0;

  // Return copy of this mapping.
  virtual Layer* clone() const = 0;

	/// Returns true iff source is compatible with mapping.
	virtual bool sourceIsCompatible(Source::ptr source) const = 0;

  /// Returns the source.
  Source::ptr getSource() const { return _source; }

	/// Returns source id.
	uid getSourceId() const { return _source->getId(); }

  /// Returns the (output) shape.
  MShape::ptr getShape() const { return _shape; }

  /// Returns true iff the mapping possesses an input (source) shape.
  virtual bool hasInputShape() const { return !_inputShape.isNull(); }

  /// Returns the input (source) shape (if this mapping has one) or a null pointer if not.
  virtual MShape::ptr getInputShape() const { return _inputShape; }

  virtual void setSolo(bool solo);
  virtual void setVisible(bool visible);
  virtual void setDepth(int depth);

  virtual void setLocked(bool locked);

  virtual bool isSolo() const      { return _isSolo; }
  virtual bool isVisible() const   { return _isVisible; }
  virtual int getDepth() const { return _depth; }

  virtual void toggleSolo()    { setSolo(!isSolo()); }
  virtual void toggleVisible() { setVisible(!isVisible()); }

  virtual float getComputedOpacity() const { return getOpacity() * _source->getOpacity(); }

  virtual void setSource(Source::ptr source);
	virtual void setSourceById(uid sourceId);
  virtual void setShape(MShape::ptr s) { _shape = s; }
  virtual void setInputShape(MShape::ptr s) { _inputShape = s; }

  virtual void read(const QJsonObject& obj);
  virtual void write(QJsonObject& obj);

protected:
  void _readShape(const QJsonObject& obj, bool isOutput);
  void _writeShape(QJsonObject& obj, bool isOutput);
};

/**
 * Mapping of a Color source into a shape.
 */
class ColorLayer : public Layer
{
  Q_OBJECT
public:
  Q_INVOKABLE ColorLayer(int id=NULL_UID)
    : Layer(id) {}

  ColorLayer(Source::ptr source, MShape::ptr shape,
               uid id=NULL_UID)
    : Layer(source, shape, id) {}

  // Return copy of this mapping.
  virtual Layer* clone() const {
    MShape::ptr shape(_shape->clone());
    return new ColorLayer(_source, shape);
  }

  /// Returns true iff the mapping possesses an input (source) shape.
  virtual bool hasInputShape() const { return false; }

	/// Returns true iff source is compatible with mapping.
	virtual bool sourceIsCompatible(Source::ptr source) const;

  virtual MShape::ShapeType getType() const {
    return getShape()->getType();
  }

};

/**
 * Object whose source is an image texture. In the case of a texture mapping we require
 * an additional input shape to specify the area on the image where we pick the pixels.
 */
class TextureLayer : public Layer
{
  Q_OBJECT
public:
  Q_INVOKABLE TextureLayer(int id=NULL_UID)
    : Layer(id) {}

  TextureLayer(Source::ptr source,
                 MShape::ptr shape,
                 MShape::ptr inputShape, uid id=NULL_UID)
    : Layer(source, shape, inputShape, id)
  {
    // Only supports shape of the same type (for now).
    Q_ASSERT(shape->getType() == inputShape->getType());
  }

  // Return copy of this mapping.
  virtual Layer* clone() const {
    MShape::ptr shape(_shape->clone());
    MShape::ptr inputShape(_inputShape->clone());
    return new TextureLayer(_source, shape, inputShape);
  }

  /// Returns true iff the mapping possesses an input (source) shape.
  virtual bool hasInputShape() const { return true; }

	/// Returns true iff source is compatible with mapping.
	virtual bool sourceIsCompatible(Source::ptr source) const;

  virtual MShape::ShapeType getType() const {
    return getShape()->getType();
  }
};

}

#endif /* LAYER_H_ */
