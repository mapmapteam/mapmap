/*
 * Paint.h
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

#ifndef VIDEO_IMPL_H_
#define VIDEO_IMPL_H_

#include <gst/gst.h>
#include <QtGlobal>
#include <QtOpenGL>
#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <tr1/memory>

/**
 * Private declaration of the video player.
 * This is done this way so that GStreamer header don't need to be 
 * included in the whole project. (just in this file)
 */
class VideoImpl
{
public:
  VideoImpl();
  ~VideoImpl();
  void setUri(const QString uri);
  static bool hasVideoSupport();
  void build();
  int getWidth() const;
  int getHeight() const;
  const uchar* getBits() const;
private:
  QString uri_;
};

#endif /* ifndef */
