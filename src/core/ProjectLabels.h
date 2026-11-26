/*
 * ProjectLabels.h
 *
 * (c) 2016 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef PROJECTLABELS_H_
#define PROJECTLABELS_H_

#include "MM.h"

namespace mmp {

class ProjectLabels {
public:
  /// File format identifier, independent of the application release.
  static const char* FORMAT_VERSION;
  /// Image or video path, or a camera device identifier.
  static const char* URI;
  static const char* CLASS_NAME;
  static const char* SOURCES;
  static const char* LAYERS;
  static const char* OUTPUT_RESOLUTION;
  static const char* WIDTH;
  static const char* HEIGHT;

  static const char* ID;
  static const char* NAME;

  static const char* DESTINATION;
  static const char* SOURCE;

  // Ids of the settings that several files name. The id of a setting is its key in project
  // files and its name in OSC addresses and in MCP. A FORMER_ constant is the key of the
  // same setting in the project files of MapMap 1.0.0-beta.1 and earlier: it is declared
  // with SettingInfo::formerId(), read when the id is absent, and never written.

  /// The source shown by a layer.
  static const char* SOURCE_ID;
  static const char* FORMER_SOURCE_ID;

  /// The number of columns of vertices of a mesh.
  static const char* N_COLUMNS;
  static const char* FORMER_N_COLUMNS;
  /// The number of rows of vertices of a mesh.
  static const char* N_ROWS;
  static const char* FORMER_N_ROWS;

  /// The system-wide identifier of the server of a Syphon source.
  static const char* SERVER_UUID;
  static const char* FORMER_SERVER_UUID;
  /// The name of the server of a Syphon source.
  static const char* SERVER_NAME;
  static const char* FORMER_SERVER_NAME;
  /// The name of the application publishing the server of a Syphon source.
  static const char* APP_NAME;
  static const char* FORMER_APP_NAME;
  /// Whether a Syphon source keeps the transparency sent by its server.
  static const char* RESPECT_ALPHA;
  static const char* FORMER_RESPECT_ALPHA;
};

}

#endif /* PROJECTLABELS_H_ */
