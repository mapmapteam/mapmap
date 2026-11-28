/*
 * SettingControl.h
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

#ifndef SETTING_CONTROL_H_
#define SETTING_CONTROL_H_

#include <functional>

#include <QComboBox>
#include <QPointF>
#include <QStringList>
#include <QWidget>

#include "Setting.h"

class QAbstractButton;
class QButtonGroup;
class QLineEdit;
class QToolButton;

namespace mmp {

/**
 * A widget that shows and edits one setting of a tree: the "setting editor" of the kit.
 *
 * A control remembers the topmost group of the tree and the path of its setting, and looks the
 * setting up each time, since a node may disappear when the structure changes. It writes to
 * the tree only on a gesture of the user, and shows the value of the tree on refresh().
 */
class SettingControl : public QWidget {
  Q_OBJECT

public:
  /// Reads the value used for rendering, shown as a mark when it differs from the base value.
  typedef std::function<QVariant(const Setting*)> LiveValueReader;

  /// Creates a control for the setting at path, relative to root.
  SettingControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  /// Returns the path of the setting, relative to the topmost group.
  QString path() const { return _path; }

  /// Returns the topmost group of the tree of the setting.
  SettingGroup* root() const { return _root; }

  /// Returns the setting, or nullptr when the tree no longer has it.
  Setting* setting() const;

  /// Shows the current value of the setting.
  virtual void refresh() = 0;

  /// Returns true when the control can move its value by steps: the label of its row then
  /// adjusts it when dragged.
  virtual bool acceptsSteps() const { return false; }

  /// Moves the value by a number of steps of the interface; may be fractional.
  virtual void stepBy(double steps) { Q_UNUSED(steps); }

  /// Sets the function reading the value used for rendering; Setting::effectiveValue() by default.
  void setLiveValueReader(LiveValueReader reader) { _liveValueReader = reader; }

  /// Returns the value used for rendering.
  QVariant liveValue() const;

  /// Makes the control fitting the type of the setting. A compact control, for a row shared with
  /// other settings, gives an integer a counter instead of a slider.
  static SettingControl* create(SettingGroup* root, Setting* setting, bool compact,
                                QWidget* parent = nullptr);

signals:
  /// The user changed the value and the tree accepted it.
  void edited();

protected:
  /// Writes a value to the setting; emits edited() when accepted. Shows what the tree kept.
  bool write(const QVariant& value);

  /// Writes a value as shown to the user; see Setting::setDisplayValue().
  bool writeDisplay(const QVariant& value);

private:
  SettingGroup* _root;
  QString _path;
  LiveValueReader _liveValueReader;
};

/// A horizontal bar showing a position between 0 and 1, which the user drags.
class SliderBar : public QWidget {
  Q_OBJECT

public:
  /// Creates a bar.
  explicit SliderBar(QWidget* parent = nullptr);

  /// Sets the position of the handle, between 0 and 1.
  void setPosition(double position);

  /// Sets where the filling starts: 0, or the position of zero when the minimum is negative.
  void setOrigin(double origin);

  /// Sets the position of the live value, or a negative number to show none.
  void setLivePosition(double position);

  /// Returns the position of the handle.
  double position() const { return _position; }

  /// Returns the position of the live value, or a negative number when none is shown.
  double livePosition() const { return _livePosition; }

  QSize sizeHint() const override;

signals:
  /// The user moved the handle to a position.
  void moved(double position);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  /// Returns the position under a horizontal coordinate.
  double _positionAt(int x) const;

  double _position;
  double _origin;
  double _livePosition;
};

/// A number or an integer: a slider when the setting has bounds, and a field with its unit.
class NumberControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  NumberControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;
  bool acceptsSteps() const override { return true; }
  void stepBy(double steps) override;

  /// Returns the field, for tests.
  QLineEdit* field() const { return _field; }

  /// Returns the slider, or nullptr when the setting has no bounds.
  SliderBar* slider() const { return _slider; }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Writes the number typed in the field.
  void _commitField();

  SliderBar* _slider;
  QLineEdit* _field;
};

/// An integer in a shared row, such as the columns and rows of a mesh: a counter.
class CounterControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  CounterControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;
  bool acceptsSteps() const override { return true; }
  void stepBy(double steps) override;

  /// Returns the field, for tests.
  QLineEdit* field() const { return _field; }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Writes the number typed in the field.
  void _commitField();

  QLineEdit* _field;
};

/// A switch for a boolean.
class SwitchControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  SwitchControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;
  QSize sizeHint() const override;

  /// Returns true when the switch shows "on".
  bool isOn() const { return _on; }

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  /// Writes the opposite value.
  void _toggle();

  bool _on;
};

/// A menu that asks for its entries each time it opens.
class RefreshingComboBox : public QComboBox {
  Q_OBJECT

public:
  /// Creates the menu.
  explicit RefreshingComboBox(QWidget* parent = nullptr);

  /// Sets the function called before the menu opens.
  void setAboutToShow(std::function<void()> function) { _aboutToShow = function; }

  void showPopup() override;

private:
  std::function<void()> _aboutToShow;
};

/// A choice or a reference: buttons side by side when the labels are short, a menu otherwise.
/// An entry that disappeared is still shown in the menu.
class ChoiceControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  ChoiceControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;

  /// Returns true when the control shows buttons side by side.
  bool showsButtons() const { return _buttons != nullptr; }

  /// Returns the menu, or nullptr when the control shows buttons.
  QComboBox* menu() const { return _menu; }

  /// Returns the ids of the entries, in the order shown.
  QStringList optionIds() const { return _optionIds; }

  /// Returns true when the options fit on buttons side by side.
  static bool fitsButtons(const QList<SettingOption>& options);

private:
  /// Fills the menu with the current options, and the value when it is not one of them.
  void _fillMenu();

  QButtonGroup* _buttons;
  RefreshingComboBox* _menu;
  QStringList _optionIds;
  /// Set while the menu is filled, so that its changes are not written.
  bool _filling;
};

/// A text; written when the field loses the focus or the user presses Enter.
class TextControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  TextControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;

  /// Returns the field, for tests.
  QLineEdit* field() const { return _field; }

private:
  QLineEdit* _field;
  QToolButton* _browse;
};

/// A square in which the user drags a point.
class PointPad : public QWidget {
  Q_OBJECT

public:
  /// Creates the pad.
  explicit PointPad(QWidget* parent = nullptr);

  /// Sets the position of the point, each coordinate between 0 and 1, y going down.
  void setPosition(const QPointF& position);

  QSize sizeHint() const override;

signals:
  /// The user moved the point.
  void moved(const QPointF& position);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  /// Returns the position under a point of the widget.
  QPointF _positionAt(const QPoint& point) const;

  QPointF _position;
};

/// A point: a pad when the setting has bounds, and two fields X and Y.
class PointControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  PointControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;

  /// Returns the field of a coordinate: 0 for X, 1 for Y.
  QLineEdit* field(int coordinate) const { return coordinate == 0 ? _x : _y; }

  /// Returns the pad, or nullptr when the setting has no bounds.
  PointPad* pad() const { return _pad; }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  /// Writes the number typed in the field of a coordinate.
  void _commitField(int coordinate);

  /// Writes one coordinate, given as shown to the user.
  void _writeCoordinate(int coordinate, double displayValue);

  PointPad* _pad;
  QLineEdit* _x;
  QLineEdit* _y;
};

/// A bar with two handles that do not cross.
class RangeBar : public QWidget {
  Q_OBJECT

public:
  /// Creates the bar.
  explicit RangeBar(QWidget* parent = nullptr);

  /// Sets the positions of the two handles, between 0 and 1.
  void setPositions(double low, double high);

  QSize sizeHint() const override;

signals:
  /// The user moved a handle.
  void moved(double low, double high);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  /// Returns the position under a horizontal coordinate.
  double _positionAt(int x) const;

  /// Moves the handle being dragged to a position, keeping it on its side of the other.
  void _drag(double position);

  double _low;
  double _high;
  /// The handle being dragged: 0 for the low one, 1 for the high one.
  int _dragged;
};

/// An interval: a bar with two handles when the setting has bounds, and two fields.
class RangeControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  RangeControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;

  /// Returns the field of a bound: 0 for the low one, 1 for the high one.
  QLineEdit* field(int bound) const { return bound == 0 ? _low : _high; }

  /// Returns the bar, or nullptr when the setting has no bounds.
  RangeBar* bar() const { return _bar; }

private:
  /// Writes the number typed in the field of a bound.
  void _commitField(int bound);

  RangeBar* _bar;
  QLineEdit* _low;
  QLineEdit* _high;
};

class ColorPicker;

/// A colour: a swatch and its code. The swatch unfolds a picker under the control.
class ColorControl : public SettingControl {
  Q_OBJECT

public:
  /// Creates the control.
  ColorControl(SettingGroup* root, const QString& path, QWidget* parent = nullptr);

  void refresh() override;

  /// Returns the swatch, for tests.
  QToolButton* swatch() const { return _swatch; }

  /// Returns the field of the code, for tests.
  QLineEdit* field() const { return _field; }

  /// Returns the picker, or nullptr when it was never unfolded.
  ColorPicker* picker() const { return _picker; }

  /// Unfolds or folds the picker.
  void setPickerShown(bool shown);

private:
  QToolButton* _swatch;
  QLineEdit* _field;
  ColorPicker* _picker;
};

} // namespace mmp

#endif /* SETTING_CONTROL_H_ */
