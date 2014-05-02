/*
 * MM.h
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

#ifndef MM_H_
#define MM_H_

#include <QtGlobal>
#include <QColor>

class MM
{
public:
  // GUI.
  static const int DEFAULT_WINDOW_WIDTH = 640;
  static const int DEFAULT_WINDOW_HEIGHT = 480;

  // Style.
  static const QColor WHITE;
  static const QColor BLUE_GRAY;
  static const QColor DARK_GRAY;

  // Control.
  static const qreal VERTEX_STICK_RADIUS = 10;
};

#endif


