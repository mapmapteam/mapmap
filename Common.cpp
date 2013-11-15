/*
 * Common.cpp
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

#include "Common.h"

std::tr1::shared_ptr<Mapping> Common::currentMapping;
std::tr1::shared_ptr<Mapper>  Common::currentMapper;

void Common::initializeLibremapper(int frameWidth, int frameHeight) {
  Image* img = new Image("example.png");

  float centerX = frameWidth / 2;
  float centerY = frameHeight / 2;
  float textureHalfWidth  = img->getWidth()  / 2;
  float textureHalfHeight = img->getHeight() / 2;
//  printf("Common: %f %f %f %f\n", centerX, centerY, textureHalfWidth, textureHalfHeight);

  TextureMapping* tm = new TextureMapping(
      img,

      // Destination.
      new Quad(Point(centerX-textureHalfWidth, centerY-textureHalfHeight),
               Point(centerX+textureHalfWidth, centerY-textureHalfHeight),
               Point(centerX+textureHalfWidth, centerY+textureHalfHeight),
               Point(centerX-textureHalfWidth, centerY+textureHalfHeight)),

      // Input.
      new Quad(Point(centerX-textureHalfWidth, centerY-textureHalfHeight),
               Point(centerX+textureHalfWidth, centerY-textureHalfHeight),
               Point(centerX+textureHalfWidth, centerY+textureHalfHeight),
               Point(centerX-textureHalfWidth, centerY+textureHalfHeight))
            );
  Common::currentMapping.reset( tm );
  Common::currentMapper.reset( new QuadTextureMapper( tm ) );
}

