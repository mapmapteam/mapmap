/*
 * Mapper.h
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


#ifndef MAPPER_H_
#define MAPPER_H_

#include <QtGlobal>

#include "Shape.h"
#include "Paint.h"

#include "Util.h"

#include <tr1/memory>

#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <stdlib.h>
#include <stdio.h>

class Mapping
{
protected:
  std::tr1::shared_ptr<Paint> _paint;
  std::tr1::shared_ptr<Shape> _shape;

public:
  Mapping(Paint* paint, Shape* shape)
    : _paint(paint), _shape(shape)
  {}

  virtual void build() {
    _paint->build();
    _shape->build();
  }

public:
  std::tr1::shared_ptr<Paint> getPaint() const { return _paint; }
  std::tr1::shared_ptr<Shape> getShape() const { return _shape; }
};

class TextureMapping : public Mapping
{
private:
  std::tr1::shared_ptr<Shape> _inputShape;

public:
  TextureMapping(Paint* paint,
                 Shape* shape,
                 Shape* inputShape)
    : Mapping(paint, shape),
      _inputShape(inputShape)
  {}

  virtual void build() {
    Mapping::build();
    _inputShape->build();
  }
public:
  std::tr1::shared_ptr<Shape> getInputShape() const { return _inputShape; }
};

class Mapper
{
protected:
  std::tr1::shared_ptr<Mapping> _mapping;
  Mapper(Mapping* mapping) : _mapping(mapping) {}
  virtual ~Mapper() {}

public:
  virtual void draw() = 0;
};

//class ShapeDrawer
//{
//protected:
//  std:tr1::shared_ptr<Shape> _shape;
//
//public:
//  ShapeDrawer(const std:tr1::shared_ptr<Shape>& shape) : _shape(shape) {}
//  virtual ~ShapeDrawer() {}
//
//  virtual void draw() = 0;
//};
//
//class QuadDrawer
//{
//public:
//  QuadDrawer(const std:tr1::shared_ptr<Quad>& quad) : ShapeDrawer(quad) {}
//
//  virtual void draw() {
//    std::tr1::shared_ptr<Quad> quad = std::tr1::static_pointer_cast<Quad>(_shape);
//    wxASSERT(quad != NULL);
//
//    glColor4f (1, 1, 1, 1);
//
//    // Source quad.
//    glLineWidth(5);
//    glBegin (GL_LINE_STRIP);
//    {
//      for (int i=0; i<5; i++) {
//        glVertex3f(quad->getVertex(i % 4).x / (GLfloat)texture->getWidth(),
//                   quad->getVertex(i % 4).y / (GLfloat)texture->getHeight(),
//                   0);
//      }
//    }
//    glEnd ();
//  }
//};

class QuadTextureMapper : public Mapper
{
public:
  QuadTextureMapper(TextureMapping* mapping) : Mapper(mapping) {}
  virtual ~QuadTextureMapper() {}

  virtual void draw();
};


#endif /* MAPPER_H_ */
