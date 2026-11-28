/*
 * MetaObjectRegistryBuiltins.cpp
 *
 * Kept apart from MetaObjectRegistry.cpp so that the registry itself does not
 * depend on every source, layer and shape class.
 *
 * (c) 2026 Sofian Audry -- info(@)sofianaudry(.)com
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

#include "MetaObjectRegistry.h"
#include "Source.h"
#include "Generators.h"
#include "Layer.h"
#include "Shapes.h"

namespace mmp {

void MetaObjectRegistry::addBuiltinTypes()
{
  // Sources.
  add<Video>();
  add<Camera>();
  add<Image>();
  add<CheckerGenerator>();
  add<GridGenerator>();
  add<ColorWheelGenerator>();
  add<ColorGenerator>();

  // Layers (formerly Mappings).
  add<TextureLayer>();

  // Backward compatibility for old project files.
  addAlias("mmp::TextureMapping", &TextureLayer::staticMetaObject);
  // Colour sources and colour layers (formerly ColorMapping) are read as colour generators
  // and texture layers: see ProjectReader.

  // Shapes.
  add<Quad>();
  add<Mesh>();
  add<MM_PREPEND_NAMESPACE(Ellipse)>();
  add<Triangle>();
}

}
