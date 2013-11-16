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

class Common {
public:
  static std::vector< std::tr1::shared_ptr<Mapping> > mappings;
  static std::vector< std::tr1::shared_ptr<Mapper> > mappers;
  static std::tr1::shared_ptr<Mapping> currentMapping;
  static std::tr1::shared_ptr<Mapper> currentMapper;

  static int currentSourceIdx;

  static Quad* createQuadForTexture(Texture* texture, int frameWidth, int frameHeight);
  static void addImage(const std::string imagePath, int frameWidth, int frameHeight);
  static void initializeLibremapper(int frameWidth, int frameHeight);
  static void nextImage();

  static int nImages() { return mappings.size(); }
};

#endif /* COMMON_H_ */
