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

#include <QXmlStreamReader>
#include <QDomDocument>
#include "MainWindow.h"
#include "Mapping.h"
#include "Paint.h"

class ProjectReader
{
public:
    ProjectReader (MainWindow* window);
    bool readFile (QIODevice *device);
    QString errorString() const;

private:
    void readProject();
    void parseProject(const QDomElement& project);
    void parsePaint(const QDomElement& paint);
    void parseMapping(const QDomElement& mapping);

    void _parseStandardShape(const QString& type, int nVertices, const QDomElement& shape, QVector<QPointF>& points);
    void _parseQuad(const QDomElement& quad, QVector<QPointF>& points);
    void _parseTriangle(const QDomElement& triangle, QVector<QPointF>& points);
    void _parseMesh(const QDomElement& mesh, QVector<QPointF>& points, int& nColumns, int& nRows);
    QPointF _parseVertex(const QDomElement& vertex);

//    void readPaint(); //Paint *item);
//    void readMapping(); //Mapping *item);

    QXmlStreamReader _xml;
    MainWindow *_window;
};
