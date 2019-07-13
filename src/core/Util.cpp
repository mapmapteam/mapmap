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
#include <QRegExp>

namespace mmp {

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

Mesh* createMeshForColor(int frameWidth, int frameHeight)
{
  return new Mesh(
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

void drawControlsVertex(QPainter* painter, const QPointF& vertex, bool major, bool selected, bool locked, MShape::ShapeMode shapeMode, qreal radius, qreal strokeWidth)
{
  // Init colors and stroke.
  if (locked)
    painter->setBrush(MM::VERTEX_LOCKED_BACKGROUND);
  else
    painter->setBrush(selected ? MM::VERTEX_SELECTED_BACKGROUND : MM::VERTEX_BACKGROUND);

  painter->setPen(locked ? QPen(MM::CONTROL_LOCKED_COLOR) : QPen(MM::CONTROL_COLOR, strokeWidth));

  QRect target((vertex.x() - radius) + 1,
               (vertex.y() - radius) - 1,
               radius * 2, radius * 2);

  if (locked)
  {
    // Draw ellipse.
    painter->drawEllipse(vertex, radius, radius);
  }
  else if (shapeMode == MShape::DefaultMode)
  {
    // Draw ellipse.
    painter->drawEllipse(vertex, radius, radius);

    // Draw cross.
    qreal offset = sin(M_PI/4) * radius;
    painter->drawLine( vertex + QPointF(offset, offset),  vertex + QPointF(-offset, -offset) );
    painter->drawLine( vertex + QPointF(offset, -offset), vertex + QPointF(-offset, offset) );
  }
  else if (!major)
  {
      painter->drawEllipse(vertex, radius, radius);
  }
  else if (shapeMode == MShape::ScaleMode)
  {
    painter->drawPixmap(target, QPixmap(":/vertex-scale"));
  }
  else // RotateMode
  {
    // Draw rotate icons
    painter->drawPixmap(target, QPixmap(":/vertex-rotate"));
  }

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
    qDebug() << "Erase MapMap settings.";
    settingsFile.close();
    return settingsFile.remove();
  }
}

bool isNumeric(const QString& text)
{
  QRegExp re("\\d*"); // a digit (\d), zero or more times (*)
  return (re.exactMatch(text));
}

} // end of namespace

}
