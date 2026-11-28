/*
 * FrameCounter.h
 *
 * (c) 2026 MapMap contributors
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


#ifndef FRAME_COUNTER_H_
#define FRAME_COUNTER_H_

#include <QtGlobal>

namespace mmp {

/**
 * Numbers the frames of MapMap, so that what is drawn once per frame, such as the
 * image of a generator, is drawn by the first view that paints and only read by the
 * others. The main window advances it before it asks the views to paint.
 */
namespace FrameCounter {

/// Returns the number of the current frame; it starts at 0 and only grows.
quint64 current();

/// Starts a new frame.
void advance();

} // namespace FrameCounter

} // namespace mmp

#endif /* FRAME_COUNTER_H_ */
