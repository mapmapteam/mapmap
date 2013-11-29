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

std::vector< std::tr1::shared_ptr<Mapping> > Common::mappings;
std::vector< std::tr1::shared_ptr<Mapper> > Common::mappers;
std::tr1::shared_ptr<Mapping> Common::currentMapping;
std::tr1::shared_ptr<Mapper> Common::currentMapper;

int Common::currentSourceIdx = 0;

Quad* Common::createQuadForTexture(Texture* texture, int frameWidth, int frameHeight)
{
  return new Quad(
    Point(texture->getX(), texture->getY()),
    Point(texture->getX() + texture->getWidth(), texture->getY()),
    Point(texture->getX() + texture->getWidth(), texture->getY() + texture->getHeight()),
    Point(texture->getX(), texture->getY() + texture->getHeight())
  );
}

void Common::addImage(const QString imagePath, int frameWidth, int frameHeight)
{
  Image* img  = new Image(imagePath);

  img->setPosition( (frameWidth  - img->getWidth() ) / 2.0f,
                    (frameHeight - img->getHeight()) / 2.0f );

  TextureMapping* tm = new TextureMapping(
      img,
      // Destination.
      createQuadForTexture(img, frameWidth, frameHeight),
      // Input.
      createQuadForTexture(img, frameWidth, frameHeight)
  );

  // Add to data structures.
  mappings.push_back( std::tr1::shared_ptr<Mapping>(tm) );
  mappers.push_back( std::tr1::shared_ptr<Mapper>(new QuadTextureMapper( tm )) );
}

void Common::initializeLibremapper(int frameWidth, int frameHeight)
{
  addImage("example.png", frameWidth, frameHeight);
  addImage("pnggrad8rgb.png", frameWidth, frameHeight);


  currentMapping   = mappings[currentSourceIdx];
  currentMapper    = mappers[currentSourceIdx];
}

void Common::switchImage(int imageId)
{
  currentSourceIdx = imageId;
  currentMapping   = mappings[currentSourceIdx];
  currentMapper    = mappers[currentSourceIdx];
}

void Common::nextImage()
{
  currentSourceIdx = (currentSourceIdx + 1) % mappings.size();
  currentMapping   = mappings[currentSourceIdx];
  currentMapper    = mappers[currentSourceIdx];
}
