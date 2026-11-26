/*
 * ProjectWriter.h
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
#ifndef PROJECT_WRITER_H_
#define PROJECT_WRITER_H_

#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "MappingManager.h"
#include "Layer.h"
#include "Source.h"

#include "ProjectLabels.h"

#include "Shapes.h"

namespace mmp {

/**
 * Writes the contents of a MappingManager to a JSON project file. Has no
 * dependency on the GUI, so it can be used by the tests.
 */
class ProjectWriter
{
public:
    ProjectWriter (MappingManager& manager);
    /// Writes a project, spelling local media paths relative to its destination directory.
    bool writeFile(QIODevice* device, const QString& projectDirectory = {});

  private:
    MappingManager& _manager;
};

}
#endif
