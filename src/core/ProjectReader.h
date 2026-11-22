/*
 * ProjectReader.h
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

#ifndef PROJECT_READER_H_
#define PROJECT_READER_H_

#include <functional>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "MappingManager.h"
#include "Layer.h"
#include "Source.h"

#include "MetaObjectRegistry.h"
#include "ProjectLabels.h"

namespace mmp {

/**
 * Reads a JSON project file into a MappingManager. Has no dependency on the
 * GUI, so it can be used by the tests.
 */
class ProjectReader
{
public:
    /**
     * Called for each image and video source with its uri and whether it is
     * an image. Returns the uri to use instead (eg. after asking the user to
     * locate a missing file), or the same uri (or an empty one, eg. when the
     * user cancels) to keep it.
     */
    typedef std::function<QString(const QString& uri, bool isImage)> MediaLocator;

    ProjectReader (MappingManager& manager, MediaLocator mediaLocator = MediaLocator());
    bool readFile (QIODevice *device);
    QString errorString() const;

private:
    void parseProject(const QJsonObject& project);
    Source::ptr   parseSource(const QJsonObject& obj);
    Layer::ptr parseLayer(const QJsonObject& obj);
    /**
     * Checks if the version attribute of the project tag matches the regex of supported versions of MapMap.
     * Some older versions of MapMap might not be supported by this version.
     */
    bool isValidVersion(const QString& versionString);

    void locateMedia(Source::ptr source);

    QString _errorString;
    MappingManager& _manager;
    MediaLocator _mediaLocator;
};

}

#endif
