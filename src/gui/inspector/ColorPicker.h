/*
 * ColorPicker.h
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

#ifndef COLOR_PICKER_H_
#define COLOR_PICKER_H_

#include <QColor>
#include <QList>
#include <QWidget>

class QHBoxLayout;
class QLineEdit;
class QToolButton;

namespace mmp {

class SaturationValueSquare;
class HueBar;

/**
 * A colour picker unfolded in the inspector: a square of saturation and brightness, a bar of
 * hue, the code of the colour, a pipette that takes a colour in the windows of MapMap, and a
 * row of recent colours, kept in the preferences of the user.
 *
 * The picker keeps its own hue, so that the hue does not jump when the saturation or the
 * brightness falls to zero.
 */
class ColorPicker : public QWidget {
  Q_OBJECT

public:
  /// Most colours kept in the row of recent colours.
  static const int MAX_RECENT_COLORS = 8;

  /// Creates the picker.
  explicit ColorPicker(QWidget* parent = nullptr);

  /// Shows a colour; keeps the current hue when the colour has none (a grey).
  void setColor(const QColor& color);

  /// Returns the colour shown.
  QColor color() const;

  /// Shows or hides the field of the code, which a control already showing it hides.
  void setCodeShown(bool shown);

  /// Returns the hue kept by the picker, between 0 and 1.
  double hue() const { return _hue; }

  /// Returns the recent colours, the most recent first.
  static QList<QColor> recentColors();

  /// Puts a colour first in the recent colours.
  static void addRecentColor(const QColor& color);

  /// Returns the colour at a point given in global coordinates, if a window of MapMap is
  /// there; an invalid colour otherwise.
  static QColor colorAtGlobalPoint(const QPoint& point);

signals:
  /// The user changed the colour; sent during a gesture, at each move.
  void colorChanged(const QColor& color);

  /// The user finished a gesture: released the mouse, typed a code or picked a colour.
  void gestureFinished();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Sends the colour made of the hue, saturation and brightness kept here.
  void _emitColor();

  /// Shows the kept colour in the square, the bar and the code.
  void _showColor();

  /// Makes the buttons of the recent colours again.
  void _showRecentColors();

  /// Starts or stops taking a colour with the pipette.
  void _setPicking(bool picking);

  SaturationValueSquare* _square;
  HueBar* _hueBar;
  QLineEdit* _code;
  QToolButton* _pipette;
  QHBoxLayout* _recentLayout;
  double _hue;
  double _saturation;
  double _value;
  int _alpha;
  bool _picking;
};

} // namespace mmp

#endif /* COLOR_PICKER_H_ */
