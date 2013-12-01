/*
 * Mapper.h
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


#ifndef MAPPER_H_
#define MAPPER_H_

#include <QtGlobal>

#include <tr1/memory>
#include <GL/gl.h>

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Paint.h"
#include "Mapping.h"

#include "Util.h"

/**
 * A way to draw on some kind of shape.
 * 
 * This is an abstract class for specific ways to draw
 * a given paint of a shape.
 * 
 * Each shape x paint combination that is possible in 
 * this software are implemented using a child of this
 * class.
 */
class Mapper
{
protected:
  Mapping::ptr _mapping;
  Mapper(Mapping::ptr mapping) : _mapping(mapping) {}
  virtual ~Mapper() {}

public:
  typedef std::tr1::shared_ptr<Mapper> ptr;
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

/**
 * Draws a texture on a quad.
 * 
 * A Quad has four vertices.
 * They are in counterclockwise order, just like for all shapes.
 */
class QuadTextureMapper : public Mapper
{
public:
  QuadTextureMapper(std::tr1::shared_ptr<TextureMapping> mapping) : Mapper(mapping) {}
  virtual ~QuadTextureMapper() {}

  virtual void draw();
};


#endif /* MAPPER_H_ */
