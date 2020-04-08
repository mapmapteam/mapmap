/*
 * MM.h
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

#ifndef MM_H_
#define MM_H_

#include <QtGlobal>
#include <QColor>
#include <QPen>
#include <QBrush>

#ifndef MM_NAMESPACE
#define MM_NAMESPACE mmp
#endif
#define MM_PREPEND_NAMESPACE(name) MM_NAMESPACE::name
#define MM_USE_NAMESPACE using namespace MM_NAMESPACE;
#define MM_BEGIN_NAMESPACE namespace MM_NAMESPACE {
#define MM_END_NAMESPACE }
#define MM_FORWARD_DECLARE_CLASS(name) \
  MM_BEGIN_NAMESPACE class name; MM_END_NAMESPACE \
  using MM_PREPEND_NAMESPACE(name);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace mmp {

/**
 * This class is a placeholder for predefined static variables that can be used
 * accross the software.
 */
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
  static const QString NAMESPACE_PREFIX;
  static const QString SUPPORTED_LANGUAGES;
  static const QString SUPPORTED_FILE_VERSIONS;

  // GUI.
  static const int DEFAULT_WINDOW_WIDTH = 640;
  static const int DEFAULT_WINDOW_HEIGHT = 480;
  static const int TOOLBAR_ICON_SIZE = 48;
  static const int ZOOM_TOOLBAR_ICON_SIZE = 16;
  static const int ZOOM_TOOLBAR_BUTTON_SIZE = 20;
  static const int MAPPING_LIST_ICON_SIZE = 16;
  static const int MAPPING_LIST_HIDE_COLUMN = 24;
  static const int MAPPING_LIST_NAME_COLUMN = 135;
  static const int MAPPING_LIST_BUTTONS_COLUMN = 128;

  // OSC
  static const int DEFAULT_OSC_PORT = 12345;

  // Default values
  static const bool DISPLAY_TEST_SIGNAL = false;
  static const bool DISPLAY_OUTPUT_WINDOW = false;
  static const bool DISPLAY_CONTROLS = true;
  static const bool DISPLAY_ALL_CONTROLS = true;
  static const bool DISPLAY_UNDO_HISTORY = false;
  static const bool DISPLAY_ZOOM_TOOLBAR = true;
  static const bool DISPLAY_MENU_BAR = true;
  static const bool STICKY_VERTICES = true;
  static const int DEFAULT_TEST_CARD = 1;
  static const bool SHOW_OUTPUT_RESOLUTION = true;
  static const QString DEFAULT_LANGUAGE;
  static const bool SHOW_OUTPUT_ON_MOUSE_HOVER = true;
  static const bool OSC_SAME_MEDIA_SOURCE = false;
  static const bool PLAY_IN_LOOP = true;

  // Style.
  static const QColor WHITE;
  static const QColor BLUE_GRAY;
  static const QColor DARK_GRAY;
  static const QColor DARK_BLUE;
  static const QColor LIGHT_RED;
  static const QColor DARK_RED;

  static const QColor CROSSHAIR_STROKE;
  static const QColor CONTROL_COLOR;
  static const QColor CONTROL_COLOR_NON_SELECTED;
  static const QColor CONTROL_LOCKED_COLOR;
  static const QBrush VERTEX_BACKGROUND;
  static const QBrush VERTEX_SELECTED_BACKGROUND;
  static const QBrush VERTEX_LOCKED_BACKGROUND;

  static const qreal SHAPE_STROKE_WIDTH;
  static const qreal SHAPE_INNER_STROKE_WIDTH;
  static const QPen  SHAPE_STROKE;
  static const QPen  SHAPE_INNER_STROKE;

  // Control.
  static const qreal VERTEX_STICK_RADIUS;
  static const qreal VERTEX_SELECT_RADIUS;
  static const qreal VERTEX_LOCKED_RADIUS;
  static const qreal VERTEX_SELECT_STROKE_WIDTH;

  // Time.
  static const qreal DEFAULT_FRAMES_PER_SECOND;

  // Zoom.
  static const qreal ZOOM_FACTOR;
  static const qreal ZOOM_MIN;
  static const qreal ZOOM_MAX;

  // Misc.
  static const int MESH_SUBDIVISION_MIN_AREA = 400;
  static const int MESH_SUBDIVISION_MAX_DEPTH_EDITING = 4;
  static const int MESH_SUBDIVISION_MAX_DEPTH         = (-1);
  static const int ELLIPSE_N_TRIANGLES = 100; // n triangles used to draw an ellipse

  // Enumerations
  enum ItemColumn {
    HideColumn,
    IconAndNameColum,
    GroupButtonColum
  };

  enum TestCard {
    Classic,
    PAL,
    NTSC
  };

  // Enumaration of vertex's differents moving step
  enum VertexMoveStep {
    SmallStep = 1,
    MediumStep = 2,
    BigStep = 20
  };

  enum MoveElement {
    Raise,
    Lower,
    Top,
    Bottom,
  };
};

}

#endif
