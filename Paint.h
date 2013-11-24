/*
 * Paint.h
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


#ifndef PAINT_H_
#define PAINT_H_

#include <QtGlobal>
#include <QtOpenGL>

#include <string>

#include <GL/gl.h>

class Paint
{
protected:
  Paint() {}
public:
  virtual ~Paint() {}
  virtual void build() {}
};

class Texture : public Paint
{
protected:
  GLuint textureId;
  int x;
  int y;

  Texture(GLuint textureId_=0) : textureId(textureId_), x(-1), y(-1) {}
  virtual ~Texture() {}

public:
  GLuint getTextureId() const { return textureId; }
  virtual void loadTexture() = 0;
  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;
  virtual const uchar* getBits() const = 0;

  virtual void setPosition(int xPos, int yPos) {
    x = xPos;
    y = yPos;
  }
  virtual int getX() const { return x; }
  virtual int getY() const { return y; }
};

class Image : public Texture
{
protected:
  QImage image;

public:
  Image(const std::string imagePath_) : Texture() {
    image = QGLWidget::convertToGLFormat(QImage(imagePath_.c_str()));
  }

  virtual ~Image() {
  }

  virtual void loadTexture() {
    if (textureId == 0)
    {
      glGenTextures(1, &textureId);
    }
//    textureId = SOIL_create_OGL_texture ( imageData, width, height, 3, textureId, 0);
    // TODO: free data?
  }

  virtual void build() {
  }

  virtual int getWidth() const { return image.width(); }
  virtual int getHeight() const { return image.height(); }
  virtual const uchar* getBits() const { return image.bits(); }

};

#endif /* PAINT_H_ */
