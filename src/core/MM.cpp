/*
 * MM.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2015 Dame Diongue -- baydamd(@)gmail(.)com
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

#include "MM.h"

namespace mmp {

const QString MM::APPLICATION_NAME = "MapMap";
const QString MM::VERSION = "0.6.3";
const QString MM::COPYRIGHT_OWNERS = "Sofian Audry, Dame Diongue, Alexandre Quessy, Mike Latona, Vasilis Liaskovitis";
const QString MM::ORGANIZATION_NAME = "MapMap";
const QString MM::ORGANIZATION_DOMAIN = "mapmap.info";
const QString MM::FILE_EXTENSION = "mmp";
const QString MM::VIDEO_FILES_FILTER = "*.mov *.mp4 *.avi *.ogg *.ogv *.mpeg *.mpeg1 *.mpeg4 *.mpg *.mpg2 *.mp2 *.mjpq *.mjp *.wmv *.webm *sock";
const QString MM::IMAGE_FILES_FILTER = "*.jpg *.jpeg *.gif *.png *.tiff *.tif *.bmp";
const QString MM::NAMESPACE_PREFIX = QString("%1::").arg(TOSTRING(MM_NAMESPACE));
const QString MM::SUPPORTED_LANGUAGES = "en, es, fr, zh_CN, zh_TW";
const QString MM::SUPPORTED_FILE_VERSIONS = "\\d+\\.\\d+\\.\\d+"; // regex

const QColor MM::WHITE("#f6f5f5");
const QColor MM::BLUE_GRAY("#323541");
const QColor MM::DARK_GRAY("#272a36");
const QColor MM::DARK_BLUE("#0C0927");
const QColor MM::LIGHT_RED("#69020B");
const QColor MM::DARK_RED("#380b19");

const QColor MM::CROSSHAIR_STROKE(QColor(0, 0, 0, 63));
const QColor MM::CONTROL_COLOR(WHITE);
const QColor MM::CONTROL_COLOR_NON_SELECTED(QColor(CONTROL_COLOR.red(), CONTROL_COLOR.green(), CONTROL_COLOR.blue(), 63));
const QColor MM::CONTROL_LOCKED_COLOR(LIGHT_RED);
const QBrush MM::VERTEX_BACKGROUND(QColor(CROSSHAIR_STROKE.red(), CROSSHAIR_STROKE.green(), CROSSHAIR_STROKE.blue(), 63));
const QBrush MM::VERTEX_SELECTED_BACKGROUND(QColor(CONTROL_COLOR.red(), CONTROL_COLOR.green(), CONTROL_COLOR.blue(), 192));
//const QBrush MM::VERTEX_LOCKED_BACKGROUND(QColor(CONTROL_LOCKED_COLOR.red(), CONTROL_LOCKED_COLOR.green(), CONTROL_LOCKED_COLOR.blue(), 63));
const QBrush MM::VERTEX_LOCKED_BACKGROUND(CONTROL_LOCKED_COLOR);

const qreal MM::SHAPE_STROKE_WIDTH = 1.5;
const qreal MM::SHAPE_INNER_STROKE_WIDTH = 0.5;
const QPen MM::SHAPE_STROKE(QBrush(CONTROL_COLOR), SHAPE_STROKE_WIDTH);
const QPen MM::SHAPE_INNER_STROKE(QBrush(CONTROL_COLOR), SHAPE_INNER_STROKE_WIDTH);

const qreal MM::VERTEX_STICK_RADIUS = 20;
const qreal MM::VERTEX_SELECT_RADIUS = 12;
const qreal MM::VERTEX_LOCKED_RADIUS = 6;
const qreal MM::VERTEX_SELECT_STROKE_WIDTH = 1.5;

// Time.
const qreal MM::DEFAULT_FRAMES_PER_SECOND = 29.97f;

// Zoom.
const qreal MM::ZOOM_FACTOR = 1.4f;
const qreal MM::ZOOM_MIN    = 0.1f;
const qreal MM::ZOOM_MAX    = 5.0f;

// Default values
const QString MM::DEFAULT_LANGUAGE = "en";

}
