/*
 * Common.h
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


#ifndef COMMON_H_
#define COMMON_H_

#include <tr1/memory>

#include "Paint.h"
#include "Shape.h"
#include "Mapper.h"

#define LIBREMAPPING_VERSION "0.1"

/**
 * The main MapMap engine that deals with video mapping projects.
 */
class Common
{
public:
  static std::vector<Mapping::ptr> mappings;
  static std::vector<Mapper::ptr> mappers;
  static Mapping::ptr currentMapping;
  static Mapper::ptr currentMapper;

  static int currentSourceIdx;

  static int getCurrentSourceId() { return currentSourceIdx; }

  static Quad* createQuadForTexture(Texture* texture, int frameWidth, int frameHeight);
  static void addImage(const QString imagePath, int frameWidth, int frameHeight);
  static void initializeLibremapper(int frameWidth, int frameHeight);

  static void switchImage(int imageId);

  // deprecated
  static void nextImage();

  static int nImages()
  {
    return mappings.size();
  }
};

#endif /* COMMON_H_ */
