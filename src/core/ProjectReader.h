/*
 * MainWindow.cpp
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "MainWindow.h"
#include "Mapping.h"
#include "Paint.h"

#include "MetaObjectRegistry.h"
#include "ProjectLabels.h"

namespace mmp {

class ProjectReader
{
public:
    ProjectReader (MainWindow* window);
    bool readFile (QIODevice *device);
    QString errorString() const;

private:
    void parseProject(const QJsonObject& project);
    Paint::ptr   parsePaint(const QJsonObject& obj);
    Mapping::ptr parseMapping(const QJsonObject& obj);
    /**
     * Checks if the version attribute of the project tag matches the regex of supported versions of MapMap.
     * Some older versions of MapMap might not be supported by this version.
     */
    bool isValidVersion(const QString& versionString);

    QString _errorString;
    MainWindow *_window;
};

}
