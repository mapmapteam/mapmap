/*
 * ProjectLabels.cpp
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

#include "ProjectLabels.h"

namespace mmp {

const char* ProjectLabels::FORMAT_VERSION = "1.0.0-beta.2";
const char* ProjectLabels::VERSION     = "version";
const char* ProjectLabels::URI = "uri";
const char* ProjectLabels::CLASS_NAME  = "className";
const char* ProjectLabels::SOURCES     = "sources";
const char* ProjectLabels::LAYERS      = "layers";
const char* ProjectLabels::OUTPUT_RESOLUTION = "outputResolution";
const char* ProjectLabels::LINKS       = "links";
const char* ProjectLabels::WIDTH       = "width";
const char* ProjectLabels::HEIGHT      = "height";
const char* ProjectLabels::ID          = "id";
const char* ProjectLabels::NAME        = "name";
const char* ProjectLabels::SOURCE      = "source";
const char* ProjectLabels::DESTINATION = "destination";

const char* ProjectLabels::SOURCE_ID            = "source_id";
const char* ProjectLabels::FORMER_SOURCE_ID     = "sourceId";
const char* ProjectLabels::N_COLUMNS            = "n_columns";
const char* ProjectLabels::FORMER_N_COLUMNS     = "nColumns";
const char* ProjectLabels::N_ROWS               = "n_rows";
const char* ProjectLabels::FORMER_N_ROWS        = "nRows";
const char* ProjectLabels::SERVER_UUID          = "server_uuid";
const char* ProjectLabels::FORMER_SERVER_UUID   = "serverUUID";
const char* ProjectLabels::SERVER_NAME          = "server_name";
const char* ProjectLabels::FORMER_SERVER_NAME   = "serverName";
const char* ProjectLabels::APP_NAME             = "app_name";
const char* ProjectLabels::FORMER_APP_NAME      = "appName";
const char* ProjectLabels::RESPECT_ALPHA        = "respect_alpha";
const char* ProjectLabels::FORMER_RESPECT_ALPHA = "respectAlpha";

bool ProjectLabels::isProjectKey(const QString& key)
{
  for (const char* known : { VERSION, OUTPUT_RESOLUTION, SOURCES, LAYERS, LINKS })
  {
    if (key == QLatin1String(known))
      return true;
  }
  return false;
}

}
