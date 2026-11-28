/*
 * FrameCounter.cpp
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


#include "FrameCounter.h"

#include <atomic>

namespace mmp {

namespace FrameCounter {

namespace {

/// The number of the current frame.
std::atomic<quint64> frame(0);

} // namespace

quint64 current()
{
  return frame.load();
}

void advance()
{
  frame++;
}

} // namespace FrameCounter

} // namespace mmp
