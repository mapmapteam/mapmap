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

#include "Shape.h"
#include "Paint.h"

#include "Element.h"

#include "UidAllocator.h"

#include "MetaObjectRegistry.h"

// TODO: replace by ProjectAttribute
//#include "ProjectWriter.h"

/**
 * Mapping is the central concept of this software.
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
class Mapping : public Element
{
  Q_OBJECT

  Q_PROPERTY(bool solo    READ isSolo    WRITE setSolo)
  Q_PROPERTY(bool visible READ isVisible WRITE setVisible)
  Q_PROPERTY(int  depth   READ getDepth  WRITE setDepth)

//  Q_PROPERTY(MShape::ptr shape READ getShape)
//  Q_PROPERTY(MShape::ptr inputShape READ getInputShape)

  Q_PROPERTY(bool hasInputShape READ hasInputShape STORED false)
//  Q_PROPERTY(Paint::ptr paint READ getPaint WRITE setPaint)

protected:
  /// The input Paint instance.
  Paint::ptr _paint;

  /// The output Shape instance.
  MShape::ptr _shape;

private:
  static UidAllocator allocator;

  bool _isSolo;
  bool _isVisible;
  int _depth; // depth of the layer

protected:
  /// Constructor.
  Mapping(Paint::ptr paint, MShape::ptr shape, uid id=NULL_UID);

public:
  typedef QSharedPointer<Mapping> ptr;

  virtual ~Mapping();

  static const UidAllocator& getUidAllocator() { return allocator; }

  /**
   * Sets up this Mapping: its Paint and its Shape.
   * Calls the build() method of its Paint and Shape.
   */
  virtual void build() {
    _paint->build();
    _shape->build();
  }

  /// The type of the mapping (expressed as a string).
  virtual QString getType() const = 0;

  /// Returns the paint.
  Paint::ptr getPaint() const { return _paint; }

  /// Returns the (output) shape.
  MShape::ptr getShape() const { return _shape; }

  /// Returns true iff the mapping possesses an input (source) shape.
  virtual bool hasInputShape() const { return false; }

  /// Returns the input (source) shape (if this mapping has one) or a null pointer if not.
  virtual MShape::ptr getInputShape() const { return MShape::ptr(); }

  void setSolo(bool solo)        { _isSolo = solo; }
  void setVisible(bool visible)  { _isVisible = visible; }
  void setDepth(int depth) { _depth = depth; }

  void toggleSolo()    { _isSolo = !_isSolo; }
  void toggleVisible() { _isVisible = !_isVisible; }

  bool isSolo() const      { return _isSolo; }
  bool isVisible() const   { return _isVisible; }
  int getDepth() const { return _depth; }

  float getComputedOpacity() const { return getOpacity() * _paint->getOpacity(); }

  void setPaint(Paint::ptr p) { _paint = p; }
  void setShape(MShape::ptr s) { _shape = s; }

  virtual void read(const QDomElement& obj);
  virtual void write(QDomElement& obj);

protected:
  virtual QList<QString> _propertiesAttributes() const
  { return Element::_propertiesAttributes() << "solo" << "visible" << "depth"; }

  void _readShape(const QDomElement& obj, bool isOutput);
  void _writeShape(QDomElement& obj, bool isOutput);
};

/**
 * Mapping of a Color paint into a shape.
 */
class ColorMapping : public Mapping
{
  Q_OBJECT
public:
  Q_INVOKABLE ColorMapping(Paint::ptr paint, MShape::ptr shape,
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
  Q_OBJECT
private:
  MShape::ptr _inputShape;

public:
  Q_INVOKABLE TextureMapping(
                 Paint::ptr paint,
                 MShape::ptr shape, uid id=NULL_UID)
    : Mapping(paint, shape, id),
      _inputShape()
  {

  }
  TextureMapping(Paint::ptr paint,
                 MShape::ptr shape,
                 MShape::ptr inputShape, uid id=NULL_UID)
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
  virtual bool hasInputShape() const { return true; }
  virtual MShape::ptr getInputShape() const { return _inputShape; }

};

#endif /* MAPPING_H_ */
