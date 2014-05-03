/*
 * MM.cpp
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

#include "MM.h"

const QColor MM::WHITE("#f6f5f5");
const QColor MM::BLUE_GRAY("#323541");
const QColor MM::DARK_GRAY("#272a36");

const QColor MM::CONTROL_COLOR(BLUE_GRAY);
const QBrush MM::VERTEX_BACKGROUND(QColor(CONTROL_COLOR.red(), CONTROL_COLOR.green(), CONTROL_COLOR.blue(), 63));
const QPen MM::SHAPE_STROKE(QBrush(CONTROL_COLOR), SHAPE_STROKE_WIDTH);
const QPen MM::SHAPE_INNER_STROKE(QBrush(CONTROL_COLOR), SHAPE_INNER_STROKE_WIDTH);

