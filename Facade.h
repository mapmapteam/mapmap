/*
 * Common.cpp
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

#include "MappingManager.h"

/**
 * Facade to control the application.
 */
class Facade
{
    public:
        Facade(MappingManager *manager);
        /**
         * Clears all mappings and paints.
         */
        bool clearProject();
        /**
         * Create an image paint.
         */
        bool createImagePaint(const char *paint_id, const char *uri);
        /**
         * Sets the image file to use in an image paint.
         */
        bool updateImagePaintUri(const char *paint_id, const char *uri);
        /**
         * Creates a textured mesh.
         */
        bool createMeshTextureMapping(const char *mapping_id, const char *paint_id,
            int n_rows, int n_cols,
            const QList<QPointF> &src, const QList<QPointF> &dst);
        /**
         * Creates a textured triangle.
         */
        bool createTriangleTextureMapping(const char *mapping_id, const char *paint_id,
            const QList<QPointF> &src, const QList<QPointF> &dst);

        // TODO:
        // bool loadProject(const char *project_file);
        // bool saveProject(const char *project_file);
        // bool quit();
        // bool deleteMapping(const char *mapping_id);
        // bool deletePaint(const char *paint_id);
        // bool listMappings(QList<QString> &result) const;
        // bool listPaints(QList<QString> &result) const;
        // bool setMappingPoints(const char *mapping_id,
        //    const QList<QPointF> &src, const QList<QPointF> &dst);
    private:
        MappingManager *_manager;
};

