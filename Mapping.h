/*
 * Mapping.h
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


#ifndef MAPPING_H_
#define MAPPING_H_

#include <QtGlobal>
#include <tr1/memory>

#include "Shape.h"
#include "Paint.h"

#include "UidAllocator.h"

/**
 * Mapping is the central concept of Libremapping.
 *
 * A Mapping represents a relationship between an input Paint and
 * and output Shape where the paint (possibly modified by some other
 * attributes or an input Shape in the case of TextureMapping) is
 * projected on the output shape.
 *
 * Mapping instances are stacked as layers by the MappingManager. One
 * can thus change their opacity level, toggle their visibility, set
 * them in "solo" mode and lock them.
 */
class Mapping
{
protected:
  /// The input Paint instance.
  Paint::ptr _paint;

  /// The output Shape instance.
  Shape::ptr _shape;

private:
  static UidAllocator allocator;

  uid _id;

  bool _isLocked;
  bool _isSolo;
  bool _isVisible;
  float _opacity;

protected:
  Mapping(Paint::ptr paint, Shape::ptr shape, uid id=NULL_UID);

public:
  typedef std::tr1::shared_ptr<Mapping> ptr;

  virtual ~Mapping();

  static const UidAllocator& getUidAllocator() { return allocator; }

  virtual void build() {
    _paint->build();
    _shape->build();
  }

  virtual QString getType() const = 0;

  Paint::ptr getPaint() const { return _paint; }
  Shape::ptr getShape() const { return _shape; }

  uid getId() const { return _id; }

  void setLocked(bool locked)    { _isLocked = locked; }
  void setSolo(bool solo)        { _isSolo = solo; }
  void setVisible(bool visible)  { _isVisible = visible; }
  void setOpacity(float opacity) {
    Q_ASSERT(0.0f <= opacity && opacity <= 1.0f);
    _opacity = opacity;
  }

  void toggleLocked()  { _isLocked = !_isLocked; }
  void toggleSolo()    { _isSolo = !_isSolo; }
  void toggleVisible() { _isVisible = !_isVisible; }

  bool isLocked() const    { return _isLocked; }
  bool isSolo() const      { return _isSolo; }
  bool isVisible() const   { return _isVisible; }
  float getOpacity() const { return _opacity; }
};

class ColorMapping : public Mapping
{
public:
  ColorMapping(Paint::ptr paint, Shape::ptr shape,
               uid id=NULL_UID)
    : Mapping(paint, shape, id) {}

  virtual QString getType() const {
    return getShape()->getType() + "_color";
  }

};

/**
 * Object whose paint is an image texture. In the case of a texture mapping we require
 * an additional input shape to specify the area on the image where we pick the pixels.
 */
class TextureMapping : public Mapping
{
private:
  Shape::ptr _inputShape;

public:
  TextureMapping(Paint::ptr paint,
                 Shape::ptr shape,
                 Shape::ptr inputShape, uid id=NULL_UID)
    : Mapping(paint, shape, id),
      _inputShape(inputShape)
  {
    // Only supports shape of the same type (for now).
    Q_ASSERT(shape->getType() == inputShape->getType());
  }

  virtual void build() {
    Mapping::build();
    _inputShape->build();
  }

  virtual QString getType() const {
    return getShape()->getType() + "_texture";
  }

public:
  Shape::ptr getInputShape() const { return _inputShape; }
};

#endif /* MAPPING_H_ */
