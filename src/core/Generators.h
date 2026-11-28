/*
 * Generators.h
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


#ifndef GENERATORS_H_
#define GENERATORS_H_

#include "Generator.h"

namespace mmp {

/// A checkerboard of two colours; the square at the top left has the front colour.
class CheckerGenerator : public ShaderGenerator
{
  Q_OBJECT

public:
  /// Default number of squares on each side.
  static const int DEFAULT_COUNT = 4;

  Q_INVOKABLE CheckerGenerator(int id = NULL_UID);

  virtual QString generatorName() const { return tr("Checker"); }
  virtual QIcon getIcon() const;

protected:
  virtual QString _fragmentShader() const;
};

/// Lines of a colour on a background, along the borders and between columns and rows.
class GridGenerator : public ShaderGenerator
{
  Q_OBJECT

public:
  /// Default number of columns and of rows.
  static const int DEFAULT_COUNT = 4;

  Q_INVOKABLE GridGenerator(int id = NULL_UID);

  virtual QString generatorName() const { return tr("Grid"); }
  virtual QIcon getIcon() const;

protected:
  virtual QString _fragmentShader() const;
};

/**
 * Colour wheels, spirals, concentric rings and polar checkerboards, drawn by one shader
 * from one set of settings: there are no modes, each pattern is a configuration.
 *
 * - Angular divisions cut the circle in segments; radial divisions cut the radius in
 *   rings, or bend the segments into spirals; grid blend crossfades from bands to cells.
 * - The colours come from a cyclic palette, spread so that it closes around the circle
 *   (see paletteScale()), turned by the hue shift.
 * - Radial saturation and lightness change the colours with the distance to the centre.
 * - Outside the disk, inscribed in the image, the image is transparent.
 * - The pattern turns at a speed in revolutions per minute; the angle is counted here,
 *   so that a change of speed or direction never makes it jump.
 */
class ColorWheelGenerator : public ShaderGenerator
{
  Q_OBJECT

public:
  /// Defaults of the geometry: twelve segments.
  static const int DEFAULT_ANGULAR_DIVISIONS = 12;

  /// Id of the geometry that matches no preset.
  static const QString CUSTOM_GEOMETRY;

  Q_INVOKABLE ColorWheelGenerator(int id = NULL_UID);

  virtual QString generatorName() const { return tr("Color Wheel"); }

  /// The image changes with time only while the pattern turns.
  virtual bool isAnimated() const;

  virtual QIcon getIcon() const;
  virtual void rewind();

  /// Reads the settings; the geometry read is remembered as the custom one when it
  /// matches no preset.
  virtual void read(const QJsonObject& obj);

  /// Returns the colours of the palette.
  QVector<QColor> paletteColors() const { return _colors; }

  /// Returns the angle the pattern has turned, in turns, between 0 and 1.
  double phase() const { return _phase; }

  /// Returns the id of the geometry preset that the geometry settings match, or
  /// CUSTOM_GEOMETRY. The Preset setting may show Custom while they match a preset, after
  /// Custom was chosen.
  QString geometryPreset() const;

  /**
   * Returns the palette entries to advance at each angular division, so that the palette
   * closes around the circle: it goes round a whole number of times, at least one entry
   * per division. Twelve divisions and a palette of twelve colours give one colour per
   * segment; a palette of two colours alternates them; a palette of twelve on six
   * divisions takes every other colour. When the divisions are not a multiple of the
   * palette, some divisions skip an entry, spread evenly, so that two neighbours never
   * have the same colour; only two colours on an odd number of divisions cannot avoid
   * it. Without angular divisions, one entry per band.
   */
  static double paletteScale(int angularDivisions, int paletteSize);

protected:
  virtual QString _fragmentShader() const;
  virtual QString _extraUniforms() const;
  virtual void _setExtraUniforms(QOpenGLShaderProgram* program);

private:
  /// Applies a geometry preset. CUSTOM_GEOMETRY brings back the last geometry of the
  /// user's own, or keeps the current one when there is none. Nothing for an unknown id.
  void _selectGeometry(const QString& id);

  /// Shows the preset that the geometry matches, and remembers it when it is custom.
  void _followGeometry();

  /// Returns, and sets, the four geometry settings, in the order of GEOMETRY_SETTINGS.
  QVector<double> _geometryValues() const;
  void _setGeometryValues(const QVector<double>& values);

  /// Selects a palette preset, whose colours replace the palette; an unknown id keeps them.
  void _setPaletteId(const QString& id);

  /// Counts the turns made since the last call at the speed of then, and takes the speed of now.
  void _advancePhase();

  /// Returns the turns per second of the speed and direction settings.
  double _turnsPerSecond() const;

  /// The preset shown, and the last geometry of the user's own (empty when none).
  QString _geometryId;
  QVector<double> _customGeometry;

  /// Set while a preset or the custom geometry is applied.
  bool _applyingGeometry;

  QString _paletteId;
  QVector<QColor> _colors;

  /// The angle turned, the time when it was counted, and the speed since then.
  double _phase;
  double _phaseTime;
  double _phaseRate;
};

/// A plain colour, alpha included, that shapes read as an image.
class ColorGenerator : public ShaderGenerator
{
  Q_OBJECT

public:
  Q_INVOKABLE ColorGenerator(int id = NULL_UID);

  /// Creates a colour generator of the given colour.
  ColorGenerator(const QColor& color, uid id = NULL_UID);

  virtual QString generatorName() const { return tr("Color"); }
  virtual QIcon getIcon() const;

  /// Returns the colour.
  QColor color() const;

protected:
  virtual QString _fragmentShader() const;
};

} // namespace mmp

#endif /* GENERATORS_H_ */
