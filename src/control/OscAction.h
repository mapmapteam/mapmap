/*
 * OscAction.h
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

#pragma once

#include <QString>
#include <QVariant>
#include <QVariantList>

namespace mmp {

/**
 * A parsed OSC command, ready to be applied to the model.
 *
 * Parsing an OSC address + arguments into an OscAction is a pure operation
 * (no access to the MainWindow or the model), which makes it unit-testable
 * without a running GUI. The side effects (resolving the target element and
 * mutating it) live in OscInterface::applyAction().
 */
struct OscAction {
  enum Type {
    Invalid,

    // Global transport.
    Quit,
    PlayAll,
    PauseAll,
    RewindAll,

    // Per-source commands. Target resolved via the selector below.
    SourcePlay,
    SourcePause,
    SourceRewind,
    SourceProperty,

    // Per-layer commands. Target resolved via the selector below.
    LayerProperty,
    LayerMove,       ///< Absolute: move the output shape so its center is (x, y).
    LayerTranslate,  ///< Relative: translate the output shape by (x, y).
    LayerVertex,     ///< Set one vertex of the input or output shape.
  };

  /// Which shape of a layer a vertex/move command applies to.
  enum ShapeRole {
    OutputShape, ///< The destination shape (where the content is projected).
    InputShape   ///< The source shape (the picked region of a texture).
  };

  Type type = Invalid;

  // --- Target selection (for Source*/Layer* commands). ---
  // The target is selected either by integer id, or by a name pattern
  // (wildcard) matching one or several elements.
  bool    selectByName = false;
  int     id = -1;
  QString name;

  // --- Payload for *Property commands. ---
  QString  property;
  QVariant value;

  // --- Payload for LayerMove / LayerTranslate / LayerVertex. ---
  ShapeRole shapeRole = OutputShape;
  int       vertexIndex = -1;
  double    x = 0.0;
  double    y = 0.0;

  // Human-readable reason, set when type == Invalid (for verbose logging / tests).
  QString error;

  bool isValid() const { return type != Invalid; }
};

/**
 * Parses an OSC address and its arguments into an OscAction.
 *
 * @param address the full OSC path, e.g. "/mapmap/layer/vertex/source/xy".
 * @param args    the OSC arguments, NOT including the address or the type tags.
 *
 * On failure, returns an OscAction whose type is Invalid and whose `error`
 * field explains why.
 */
OscAction parseOscAction(const QString& address, const QVariantList& args);

} // namespace mmp
