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

/// A colour wheel that turns: the hue follows the angle, the saturation the distance to the centre.
class ColorWheelGenerator : public ShaderGenerator
{
  Q_OBJECT

public:
  Q_INVOKABLE ColorWheelGenerator(int id = NULL_UID);

  virtual QString generatorName() const { return tr("Color Wheel"); }
  virtual bool isAnimated() const { return true; }
  virtual QIcon getIcon() const;
  virtual void rewind();

protected:
  virtual QString _fragmentShader() const;
  virtual QString _extraUniforms() const;
  virtual void _setExtraUniforms(QOpenGLShaderProgram* program);

private:
  /// The turns made so far, and the time when they were counted.
  double _turns;
  double _turnsTime;
};

} // namespace mmp

#endif /* GENERATORS_H_ */
