/*
 * Util.cpp
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

#include "Util.h"
#include <glib.h>
#include <algorithm>
#include <QFile>
#include <QDir>
#include <iostream>

namespace Util {

void correctGlTexCoord(GLfloat x, GLfloat y)
{
  glTexCoord2f (x, y);
}

void setGlTexPoint(const Texture& texture, const QPointF& inputPoint, const QPointF& outputPoint)
{
  // Set point in texture.
  correctGlTexCoord(
    (inputPoint.x() - texture.getX()) / (GLfloat) texture.getWidth(),
    (inputPoint.y() - texture.getY()) / (GLfloat) texture.getHeight());
  // Add point in output.
  glVertex2f(
    outputPoint.x(),
    outputPoint.y()
  );
}

/**
 * Convenience function to map a variable from one coordinate space
 * to another.
 * The result is clipped in the range [ostart, ostop]
 * Make sure ostop is bigger than ostart.
 *
 * To map a MIDI control value into the [0,1] range:
 * map(value, 0.0, 1.0, 0. 127.);
 *
 * Depends on: #include <algorithm>
 */
float map_float(float value, float istart, float istop, float ostart, float ostop)
{
    float ret = ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
    // In Processing, they don't do the following: (clipping)
    return std::max(std::min(ret, ostop), ostart);
}

/**
 * See map_float
 */
int map_int(int value, int istart, int istop, int ostart, int ostop)
{
    float ret = ostart + (ostop - ostart) * ((value - istart) / float(istop - istart));
    //g_print("%f = %d + (%d-%d) * ((%d-%d) / (%d-%d))", ret, ostart, ostop, ostart, value, istart, istop, istart);
    // In Processing, they don't do the following: (clipping)
    return std::max(std::min(int(ret), ostop), ostart);
}

Mesh* createMeshForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  Q_UNUSED(frameHeight);
  Q_UNUSED(frameWidth);

  return new Mesh(
    QPointF(texture->getX(), texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX(), texture->getY() + texture->getHeight())
  );
}

Triangle* createTriangleForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  Q_UNUSED(frameHeight);
  Q_UNUSED(frameWidth);

  return new Triangle(
    QPointF(texture->getX(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    QPointF(texture->getX() + texture->getWidth() / 2, texture->getY())
  );
}

Ellipse* createEllipseForTexture(Texture* texture, int frameWidth,
    int frameHeight)
{
  Q_UNUSED(frameHeight);
  Q_UNUSED(frameWidth);

  qreal halfWidth  = texture->getWidth() / 2;
  qreal halfHeight = texture->getHeight() / 2;

  return new Ellipse(
    QPointF(texture->getX(), texture->getY() + halfHeight),
    QPointF(texture->getX() + halfWidth, texture->getY()),
    QPointF(texture->getX() + texture->getWidth(), texture->getY() + halfHeight),
    QPointF(texture->getX() + halfWidth, texture->getY() + texture->getHeight()),
    true
  );
}



Quad* createQuadForColor(int frameWidth, int frameHeight)
{
  return new Quad(
    QPointF(frameWidth / 4, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight * 3/ 4),
    QPointF(frameWidth / 4, frameHeight * 3 / 4)
  );
}

Triangle* createTriangleForColor(int frameWidth, int frameHeight)
{
  return new Triangle(
    QPointF(frameWidth / 4, frameHeight * 3 / 4),
    QPointF(frameWidth * 3 / 4, frameHeight * 3 / 4),
    QPointF(frameWidth / 2, frameHeight / 4)
  );
}

Ellipse* createEllipseForColor(int frameWidth, int frameHeight)
{
  return new Ellipse(
    QPointF(frameWidth / 4, frameHeight / 2),
    QPointF(frameWidth / 2, frameHeight / 4),
    QPointF(frameWidth * 3 / 4, frameHeight / 2),
    QPointF(frameWidth / 2, frameHeight * 3 / 4),
    false
  );
}

void drawControlsVertices(QPainter* painter, const Shape& shape)
{
  for (int i=0; i<shape.nVertices(); i++)
    drawControlsVertex(painter, shape.getVertex(i));
}

void drawControlsVertex(QPainter* painter, const QPointF& vertex, qreal radius, qreal strokeWidth)
{
  // Init colors and stroke.
  painter->setBrush(MM::VERTEX_BACKGROUND);
  painter->setPen(QPen(MM::CONTROL_COLOR, strokeWidth));

  // Draw ellipse.
  painter->drawEllipse(vertex, radius, radius);

  // Draw cross.
  qreal offset = sin(M_PI/4) * radius;
  painter->drawLine( vertex + QPointF(offset, offset),  vertex + QPointF(-offset, -offset) );
  painter->drawLine( vertex + QPointF(offset, -offset), vertex + QPointF(-offset, offset) );
}

void drawControlsEllipse(QPainter* painter, const Ellipse& ellipse)
{
  // Init colors and stroke.
  painter->setPen(MM::SHAPE_STROKE);
  painter->setBrush(Qt::NoBrush);

  // Draw ellipse contour.
  qreal rotation = ellipse.getRotation();

  painter->save(); // save painter state

  painter->resetTransform();
  const QPointF& center = ellipse.getCenter();
  painter->translate(center);
  painter->rotate(rotation);
  painter->drawEllipse(QPointF(0,0), ellipse.getHorizontalRadius(), ellipse.getVerticalRadius());

  painter->restore(); // restore saved painter state

  // Draw control points.
  drawControlsVertices(painter, ellipse);
}

void drawControlsQuad(QPainter* painter, const Quad& quad)
{
  // Init colors and stroke.
  painter->setPen(MM::SHAPE_STROKE);

  // Draw quad.
  painter->drawPolygon(quad.toPolygon());

  // Draw control points.
  drawControlsVertices(painter, quad);
}

void drawControlsMesh(QPainter* painter, const Mesh& mesh)
{
  // Init colors and stroke.
  painter->setPen(MM::SHAPE_INNER_STROKE);

  // Draw inner quads.
  QVector<Quad> quads = mesh.getQuads();
  for (QVector<Quad>::const_iterator it = quads.begin(); it != quads.end(); ++it)
  {
    painter->drawPolygon(it->toPolygon());
  }

  // Draw outer quad.
  painter->setPen(MM::SHAPE_STROKE);
  painter->drawPolygon(mesh.toPolygon());

  // Draw control points.
  drawControlsVertices(painter, mesh);
}

void drawControlsPolygon(QPainter* painter, const Polygon& polygon)
{
  // Init colors and stroke.
   painter->setPen(MM::SHAPE_STROKE);

   // Draw inner quads.
   painter->drawPolygon(polygon.toPolygon());

   // Draw control points.
   drawControlsVertices(painter, polygon);
}

bool fileExists(const QString& filename)
{
  return QFile::exists(filename);
  // gchar* filetestpath = (gchar*) filename.toUtf8().constData();
  // if (FALSE == g_file_test(filetestpath, G_FILE_TEST_EXISTS))
  // {
  //     //std::cout << "File " << filetestpath << " does not exist" << std::endl;
  //     return false;
  // }
  // return true;
}

bool eraseFile(const QString& filename)
{
  if (! fileExists(filename))
  {
    return false;
  }
  QFile file(filename);
  file.close();
  return file.remove();
}

bool eraseSettings()
{
  QString homePath = QDir::homePath();
  QString settingsFilePath = QDir(homePath).filePath(".config/MapMap/MapMap.conf");
  QFile settingsFile(settingsFilePath);
  if (! settingsFile.exists())
  {
    return false;
  }
  else
  {
    std::cout << "Erase MapMap settings." << std::endl;
    settingsFile.close();
    return settingsFile.remove();
  }
}

} // end of namespace

