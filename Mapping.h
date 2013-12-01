/*
 * Mapping.h
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


#ifndef MAPPING_H_
#define MAPPING_H_

#include <QtGlobal>
#include <tr1/memory>

#include "Shape.h"
#include "Paint.h"

/**
 * One object in the scene that is a shape with some paint on it.
 *
 * A Mapping is an area of the rendering window that is drawn with
 * either some texture, or any special effect that might animate a
 * polygon or a line.
 */
class Mapping
{
protected:
  Paint::ptr _paint;
  Shape::ptr _shape;

public:
  typedef std::tr1::shared_ptr<Mapping> ptr;
  Mapping(Paint::ptr paint, Shape::ptr shape)
    : _paint(paint), _shape(shape)
  {}
  virtual ~Mapping() {}

  virtual void build() {
    _paint->build();
    _shape->build();
  }

public:
  Paint::ptr getPaint() const { return _paint; }
  Shape::ptr getShape() const { return _shape; }
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
                 Shape::ptr inputShape)
    : Mapping(paint, shape),
      _inputShape(inputShape)
  {}

  virtual void build() {
    Mapping::build();
    _inputShape->build();
  }
public:
  Shape::ptr getInputShape() const { return _inputShape; }
};

#endif /* MAPPING_H_ */
