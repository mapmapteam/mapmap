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
#include <algorithm>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <iostream>
#include <QRegularExpression>

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

void glUploadFormat(QImage::Format imageFormat, GLenum& format, GLenum& type)
{
  switch (imageFormat) {
  case QImage::Format_RGB32:
  case QImage::Format_ARGB32:
  case QImage::Format_ARGB32_Premultiplied:
    format = GL_BGRA;
    type   = GL_UNSIGNED_INT_8_8_8_8_REV;
    break;
  default:
    format = GL_RGBA;
    type   = GL_UNSIGNED_BYTE;
    break;
  }
}

void uploadTextureBits(int width, int height, GLenum format, GLenum type,
                       const uchar* bits, int& allocWidth, int& allocHeight)
{
  if (!bits || width <= 0 || height <= 0)
    return;

  if (width != allocWidth || height != allocHeight)
  {
    // (Re)allocate texture storage.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, type, bits);
    allocWidth  = width;
    allocHeight = height;
  }
  else
  {
    // Same size: update in place, avoiding a GPU reallocation.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, type, bits);
  }
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

qreal fitScale(const QSizeF& content, const QSizeF& area)
{
  if (content.isEmpty() || area.isEmpty())
    return 1.0;
  return qMin(area.width() / content.width(), area.height() / content.height());
}

QRectF fitRect(const QSizeF& content, const QRectF& frame)
{
  if (content.isEmpty())
    return frame;
  const QSizeF size = content * fitScale(content, frame.size());
  QRectF rect(QPointF(0, 0), size);
  rect.moveCenter(frame.center());
  return rect;
}

QVector<QPointF> mapRectToRect(const QVector<QPointF>& points, const QRectF& from, const QRectF& to)
{
  if (from.isEmpty())
    return points;
  const qreal sx = to.width()  / from.width();
  const qreal sy = to.height() / from.height();
  QVector<QPointF> mapped;
  mapped.reserve(points.size());
  for (const QPointF& p : points)
    mapped.append(QPointF(to.left() + (p.x() - from.left()) * sx,
                          to.top()  + (p.y() - from.top())  * sy));
  return mapped;
}

Mesh* createMeshForRect(const QRectF& rect)
{
  return new Mesh(rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft());
}

Triangle* createTriangleForRect(const QRectF& rect)
{
  return new Triangle(rect.bottomLeft(), rect.bottomRight(),
                      QPointF(rect.center().x(), rect.top()));
}

Ellipse* createEllipseForRect(const QRectF& rect, bool hasCenterControl)
{
  const QPointF center = rect.center();
  return new Ellipse(QPointF(rect.left(), center.y()),
                     QPointF(center.x(), rect.top()),
                     QPointF(rect.right(), center.y()),
                     QPointF(center.x(), rect.bottom()),
                     hasCenterControl);
}

void drawControlsVertex(QPainter* painter, const QPointF& vertex, VertexMark mark, bool locked, qreal radius, qreal strokeWidth)
{
  // Init colors and stroke.
  if (locked)
    painter->setBrush(MM::VERTEX_LOCKED_BACKGROUND);
  else if (mark == VertexMark::Selected)
    painter->setBrush(MM::VERTEX_SELECTED_BACKGROUND);
  else
    painter->setBrush(MM::VERTEX_BACKGROUND);

  painter->setPen(locked ? QPen(MM::CONTROL_LOCKED_COLOR) : QPen(MM::CONTROL_COLOR, strokeWidth));

  // Draw ellipse.
  painter->drawEllipse(vertex, radius, radius);

  // Draw the dot of the linked vertex: the background of a selected vertex,
  // in the center only.
  if (!locked && mark == VertexMark::Linked)
  {
    const qreal dotRadius = radius * MM::VERTEX_LINKED_DOT_SCALE;
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(MM::VERTEX_SELECTED_BACKGROUND);
    painter->drawEllipse(vertex, dotRadius, dotRadius);
    painter->restore();
  }

  // Draw cross.
  if (!locked)
  {
    qreal offset = sin(M_PI/4) * radius;
    painter->drawLine( vertex + QPointF(offset, offset),  vertex + QPointF(-offset, -offset) );
    painter->drawLine( vertex + QPointF(offset, -offset), vertex + QPointF(-offset, offset) );
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
  QRegularExpression re("^\\d*$"); // a digit (\d), zero or more times (*)
  return re.match(text).hasMatch();
}

} // end of namespace

}
