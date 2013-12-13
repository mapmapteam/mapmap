/*
 * Layer.h
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

#ifndef LAYER_H_
#define LAYER_H_

#include "Mapping.h"

class Layer
{
public:
  typedef std::tr1::shared_ptr<Layer> ptr;

  Layer();
  virtual ~Layer();

  void setMapping(Mapping::ptr mapping) { _mapping = mapping; }
  Mapping::ptr getMapping() const { return _mapping; }

  void setLocked(bool locked)    { _isLocked = locked; }
  void setSolo(bool solo)        { _isSolo = solo; }
  void setVisible(bool visible)  { _isVisible = visible; }
  void setOpacity(float opacity) { _opacity = opacity; }

  void toggleLocked()  { _isLocked = !_isLocked; }
  void toggleSolo()    { _isSolo = !_isSolo; }
  void toggleVisible() { _isVisible = !_isVisible; }

  bool isLocked() const    { return _isLocked; }
  bool isSolo() const      { return _isSolo; }
  bool isVisible() const   { return _isVisible; }
  float getOpacity() const { return _opacity; }

  uint getId() const { return _id; }

private:
  Mapping::ptr _mapping;
  bool _isLocked;
  bool _isSolo;
  bool _isVisible;
  float _opacity;

  uint _id;
};

#endif /* LAYER_H_ */
