/*
 * ShapeHandles.h
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

#ifndef SHAPE_HANDLES_H_
#define SHAPE_HANDLES_H_

#include <QPointF>
#include <QTransform>

#include "Shape.h"

namespace mmp {

/**
 * The handle drawn outside the current shape in the editors rotates and
 * scales it. It sits on the top of the image, as given by the order of the
 * vertices, and its pictogram is drawn the way the image is set in the shape,
 * so that it also shows whether the image is seen in mirror.
 */

/// Where the handle is attached to a shape.
struct HandleAnchor {
  QPointF point;   ///< Point of the shape the handle is attached to.
  QPointF outward; ///< Unit vector pointing away from the shape: the "up" of the image.
  QPointF along;   ///< Unit vector along the edge that carries the handle: the "right" of the image.

  /// True when the image is seen in mirror: along points to the left of outward.
  bool isMirrored() const;
};

/**
 * Returns the anchor of the handle of a shape, in the shape's
 * coordinates: the middle of side 0-1 (quad), of the top row (mesh), of side
 * 0-2 (triangle), or vertex 1 (ellipse, along pointing to vertex 2).
 */
HandleAnchor handleAnchor(const MShape& shape);

/// Returns the center of the handle's icon, distance away from the shape.
QPointF handlePosition(const MShape& shape, qreal distance);

/// What a drag on a handle changes: rotation and scale at once, or only one of them.
enum class HandleMode { Free, Rotate, Scale };

/// Returns the mode a click on the handle gives: Free, then Rotate, then Scale, then Free again.
HandleMode nextHandleMode(HandleMode mode);

/**
 * Follows the pointer during a drag on a handle and gives the transform to
 * apply to the shape as it was when the drag started. Everything turns or
 * scales around the center of the shape.
 *
 * Each move is added to the previous ones, so that pressing or releasing a
 * modifier key during the drag does not make the shape jump.
 */
class HandleDrag
{
public:
  /// Steps of the rotation with Shift, in degrees.
  static constexpr qreal ROTATION_STEP_DEGREES = 15;
  /// Steps of the scale factor with Shift.
  static constexpr qreal SCALE_STEP = 0.1;
  /// Part of a move that counts with Ctrl (fine motion).
  static constexpr qreal FINE_FACTOR = 0.1;
  /// Smallest scale factor, so that a shape never shrinks to a point.
  static constexpr qreal MIN_SCALE = 0.05;

  /// Starts a drag around center, pressed at pressPoint, in the given mode.
  HandleDrag(const QPointF& center, const QPointF& pressPoint, HandleMode mode);

  /**
   * Changes the mode during the drag. A value the new mode no longer changes
   * is frozen at the value shown, so that the shape does not jump.
   */
  void setMode(HandleMode mode);

  /// With stepped (Shift), what the mode changes is rounded to the steps.
  void setStepped(bool stepped);

  /// Feeds a new pointer position; when fine is true, this move counts for FINE_FACTOR of itself.
  void moveTo(const QPointF& point, bool fine);

  /// Rotation in degrees, clockwise on screen, as shown.
  qreal rotation() const;

  /// Scale factor as shown, never below MIN_SCALE.
  qreal scale() const;

  /// The rotation and scaling around the center, as shown.
  QTransform transform() const;

private:
  QPointF _center;
  QPointF _lastPoint;
  qreal _rotation; // Degrees, clockwise on screen, before steps.
  qreal _scale;    // Before steps.
  HandleMode _mode;
  bool _stepped;
};

}

#endif /* SHAPE_HANDLES_H_ */
