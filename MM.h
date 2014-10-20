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
#include <QPen>
#include <QBrush>

class MM
{
public:
  // General.
  static const QString APPLICATION_NAME;
  static const QString VERSION;
  static const QString COPYRIGHT_OWNERS;
  static const QString ORGANIZATION_NAME;
  static const QString ORGANIZATION_DOMAIN;
  static const QString FILE_EXTENSION;
  static const QString VIDEO_FILES_FILTER;
  static const QString IMAGE_FILES_FILTER;

  // GUI.
  static const int DEFAULT_WINDOW_WIDTH = 640;
  static const int DEFAULT_WINDOW_HEIGHT = 480;
  static const int TOP_TOOLBAR_ICON_SIZE = 64;
  static const int BOTTOM_TOOLBAR_ICON_SIZE = 32;

  // Style.
  static const QColor WHITE;
  static const QColor BLUE_GRAY;
  static const QColor DARK_GRAY;

  static const QColor CONTROL_COLOR;
  static const QBrush VERTEX_BACKGROUND;

  static const qreal SHAPE_STROKE_WIDTH = 2;
  static const qreal SHAPE_INNER_STROKE_WIDTH = 0.5;
  static const QPen  SHAPE_STROKE;
  static const QPen  SHAPE_INNER_STROKE;

  // Control.
  static const qreal VERTEX_STICK_RADIUS = 10;
  static const qreal VERTEX_SELECT_RADIUS = 10;
  static const qreal VERTEX_SELECT_STROKE_WIDTH = 1;

  // Time.
  static const float FRAMES_PER_SECOND = 29.97f;
};

#endif


