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

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <tr1/memory>

#include <stdlib.h>
#include <stdio.h>

#include "Shape.h"
#include "Paint.h"
#include "Mapping.h"

#include "Util.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

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
class Mapper : public QObject
{
  Q_OBJECT

protected:
  Mapping::ptr _mapping;

public:
  typedef std::tr1::shared_ptr<Mapper> ptr;

  Mapper(Mapping::ptr mapping) : _mapping(mapping) {}
  virtual ~Mapper() {}

  virtual QWidget* getPropertiesEditor() = 0;
  virtual void draw() = 0;

public slots:
  virtual void updateShape(Shape* shape)
  {
    Q_UNUSED(shape);
  }
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
 * Draws a texture.
 */
class TextureMapper : public Mapper
{
  Q_OBJECT

public:
  TextureMapper(std::tr1::shared_ptr<TextureMapping> mapping);
  virtual ~TextureMapper() {}

  virtual QWidget* getPropertiesEditor();

  virtual void draw();

signals:
  void valueChanged();

public slots:
  void setValue(QtProperty* property, const QVariant& value);
  void updateShape(Shape* shape);

private:
  QtAbstractPropertyBrowser* _propertyBrowser;
  QtVariantEditorFactory* _variantFactory;
  QtVariantPropertyManager* _variantManager;
  QtProperty* _topItem;
  QtProperty* _inputItem;
  QtProperty* _outputItem;

  std::map<QtProperty*, std::pair<Shape*, int> > _propertyToVertex;

  void _buildShapeProperty(QtProperty* shapeItem, Shape* shape);
  void _updateShapeProperty(QtProperty* shapeItem, Shape* shape);
};


#endif /* MAPPER_H_ */
