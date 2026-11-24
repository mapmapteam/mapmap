/*
 * MacCursor.h
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef MACCURSOR_H_
#define MACCURSOR_H_

namespace mmp {

/**
 * Hides the mouse cursor until the mouse moves, or shows it again (macOS only).
 *
 * A blank cursor is not enough there. Qt gives a cursor to macOS only when it
 * changes, and macOS can still show the arrow in a fullscreen window that has
 * had a blank cursor since before it was shown.
 */
void setMacCursorHiddenUntilMouseMoves(bool hidden);

}

#endif /* MACCURSOR_H_ */
