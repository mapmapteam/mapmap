/*
 * SettingControl.cpp
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

#include "SettingControl.h"

#include <cmath>

#include <QButtonGroup>
#include <QColor>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "ColorPicker.h"
#include "SettingFormat.h"

namespace mmp {

namespace {

/// Height of the track of a slider or a range bar, in pixels.
const int TRACK_HEIGHT = 4;

/// Diameter of the handle of a slider, in pixels.
const int HANDLE_SIZE = 12;

/// Height of a slider, a range bar or a switch, in pixels.
const int BAR_HEIGHT = 20;

/// Width of a slider before it is stretched, in pixels.
const int BAR_WIDTH = 100;

/// Width of the field of a number, in pixels.
const int FIELD_WIDTH = 64;

/// Width of the fields of a two-handle slider, in pixels.
const int SMALL_FIELD_WIDTH = 52;

/// Width of the field of a counter, in pixels: enough for two or three digits.
const int COUNTER_FIELD_WIDTH = 40;

/// Width of the − and + buttons of a counter, in pixels: fixed, so that the width of a row
/// does not depend on the style and the fonts of the system.
const int COUNTER_BUTTON_WIDTH = 18;

/// Height of the live mark drawn across a track, in pixels.
const int LIVE_MARK_HEIGHT = 10;

/// Width of the live mark, in pixels.
const int LIVE_MARK_WIDTH = 2;

/// Width and height of a switch, in pixels.
const int SWITCH_WIDTH = 34;
const int SWITCH_HEIGHT = 18;

/// Gap between the knob of a switch and its edge, in pixels.
const int SWITCH_GAP = 2;

/// Side of a point pad, in pixels.
const int PAD_SIZE = 88;

/// Diameter of the point drawn on a pad, in pixels.
const int PAD_POINT_SIZE = 10;

/// Most options shown as buttons side by side.
const int MAX_BUTTON_OPTIONS = 4;

/// Most characters of all the labels shown as buttons side by side.
const int MAX_BUTTON_LABEL_CHARACTERS = 24;

/// Side of a colour swatch, in pixels.
const int SWATCH_SIZE = 22;

/// Spacing inside a control, in pixels.
const int CONTROL_SPACING = 6;

/// Steps moved by the arrow keys with Shift pressed.
const double BIG_STEPS = 10.0;

/// Steps moved by the arrow keys.
const double ONE_STEP = 1.0;

/// No live value to show.
const double NO_LIVE_POSITION = -1.0;

/// Returns the number of steps an arrow key moves, or 0 for another key.
double arrowSteps(const QKeyEvent* event)
{
  const double steps = (event->modifiers() & Qt::ShiftModifier) ? BIG_STEPS : ONE_STEP;
  if (event->key() == Qt::Key_Up)
    return steps;
  if (event->key() == Qt::Key_Down)
    return -steps;
  return 0.0;
}

/// Reads both bounds of a setting as numbers; returns false unless both are finite and min < max.
bool numberBounds(const SettingInfo& info, double* minimum, double* maximum)
{
  if (!info.minimum().isValid() || !info.maximum().isValid())
    return false;
  bool minimumOk = false;
  bool maximumOk = false;
  *minimum = info.minimum().toDouble(&minimumOk);
  *maximum = info.maximum().toDouble(&maximumOk);
  return minimumOk && maximumOk && std::isfinite(*minimum) && std::isfinite(*maximum)
      && *minimum < *maximum;
}

/// Reads the values at both ends of the slider of a number: its bounds, or else its slider
/// range; returns false when it has neither.
bool sliderBounds(const SettingInfo& info, double* minimum, double* maximum)
{
  if (numberBounds(info, minimum, maximum))
    return true;
  if (!info.sliderMinimum().isValid() || !info.sliderMaximum().isValid())
    return false;
  *minimum = info.sliderMinimum().toDouble();
  *maximum = info.sliderMaximum().toDouble();
  return std::isfinite(*minimum) && std::isfinite(*maximum) && *minimum < *maximum;
}

/// Returns the position of value between the bounds, kept between 0 and 1.
double positionOf(double value, double minimum, double maximum)
{
  return qBound(0.0, (value - minimum) / (maximum - minimum), 1.0);
}

/// Makes a field for a number.
QLineEdit* makeField(QWidget* parent, int width)
{
  QLineEdit* field = new QLineEdit(parent);
  field->setFixedWidth(width);
  field->setAlignment(Qt::AlignRight);
  return field;
}

/// Shows text in a field, unless the user is typing in it.
void showInField(QLineEdit* field, const QString& text)
{
  if (!field->hasFocus() || !field->isModified())
    field->setText(text);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// SettingControl

SettingControl::SettingControl(SettingGroup* root, const QString& path, QWidget* parent)
  : QWidget(parent),
    _root(root),
    _path(path)
{
}

Setting* SettingControl::setting() const
{
  return _root ? _root->find(_path) : nullptr;
}

QVariant SettingControl::liveValue() const
{
  Setting* current = setting();
  if (!current)
    return QVariant();
  return _liveValueReader ? _liveValueReader(current) : current->effectiveValue();
}

bool SettingControl::write(const QVariant& value)
{
  Setting* current = setting();
  if (!current)
    return false;
  const bool accepted = current->setValue(value);
  // A refused or adjusted value: show what the tree kept.
  if (setting())
    refresh();
  if (accepted)
    emit edited();
  return accepted;
}

bool SettingControl::writeDisplay(const QVariant& value)
{
  Setting* current = setting();
  if (!current)
    return false;
  const bool accepted = current->setDisplayValue(value);
  if (setting())
    refresh();
  if (accepted)
    emit edited();
  return accepted;
}

SettingControl* SettingControl::create(SettingGroup* root, Setting* setting, bool compact,
                                       QWidget* parent)
{
  const QString path = setting->path();
  switch (setting->info().type()) {
  case SettingInfo::Number:
    return new NumberControl(root, path, parent);
  case SettingInfo::Integer:
    if (compact)
      return new CounterControl(root, path, parent);
    return new NumberControl(root, path, parent);
  case SettingInfo::Boolean:
    return new SwitchControl(root, path, parent);
  case SettingInfo::Choice:
  case SettingInfo::Reference:
    return new ChoiceControl(root, path, parent);
  case SettingInfo::Point:
    return new PointControl(root, path, parent);
  case SettingInfo::Range:
    return new RangeControl(root, path, parent);
  case SettingInfo::Color:
    return new ColorControl(root, path, parent);
  case SettingInfo::Text:
    break;
  }
  return new TextControl(root, path, parent);
}

// ---------------------------------------------------------------------------------------------
// SliderBar

SliderBar::SliderBar(QWidget* parent)
  : QWidget(parent),
    _position(0.0),
    _origin(0.0),
    _livePosition(NO_LIVE_POSITION)
{
  setMinimumHeight(BAR_HEIGHT);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setCursor(Qt::PointingHandCursor);
}

void SliderBar::setPosition(double position)
{
  _position = qBound(0.0, position, 1.0);
  update();
}

void SliderBar::setOrigin(double origin)
{
  _origin = qBound(0.0, origin, 1.0);
  update();
}

void SliderBar::setLivePosition(double position)
{
  if (position == _livePosition)
    return;
  _livePosition = position;
  update();
}

QSize SliderBar::sizeHint() const
{
  return QSize(BAR_WIDTH, BAR_HEIGHT);
}

double SliderBar::_positionAt(int x) const
{
  const int usable = width() - HANDLE_SIZE;
  if (usable <= 0)
    return 0.0;
  return qBound(0.0, double(x - HANDLE_SIZE / 2) / usable, 1.0);
}

void SliderBar::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const int usable = width() - HANDLE_SIZE;
  const double centerY = height() / 2.0;
  const QRectF track(HANDLE_SIZE / 2.0, centerY - TRACK_HEIGHT / 2.0, usable, TRACK_HEIGHT);

  painter.setPen(Qt::NoPen);
  painter.setBrush(palette().color(QPalette::Mid));
  painter.drawRoundedRect(track, TRACK_HEIGHT / 2.0, TRACK_HEIGHT / 2.0);

  // The filling goes from the origin (0, or the position of zero) to the handle.
  const double handleX = track.left() + _position * usable;
  const double originX = track.left() + _origin * usable;
  painter.setBrush(isEnabled() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Dark));
  painter.drawRect(QRectF(qMin(handleX, originX), track.top(), std::fabs(handleX - originX), TRACK_HEIGHT));

  if (_livePosition >= 0.0) {
    const double liveX = track.left() + _livePosition * usable;
    painter.setBrush(palette().color(QPalette::BrightText));
    painter.drawRect(QRectF(liveX - LIVE_MARK_WIDTH / 2.0, centerY - LIVE_MARK_HEIGHT / 2.0,
                            LIVE_MARK_WIDTH, LIVE_MARK_HEIGHT));
  }

  painter.setBrush(palette().color(QPalette::Button));
  painter.setPen(palette().color(QPalette::Highlight));
  painter.drawEllipse(QPointF(handleX, centerY), HANDLE_SIZE / 2.0 - 1, HANDLE_SIZE / 2.0 - 1);
}

void SliderBar::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
    return;
  emit moved(_positionAt(event->position().toPoint().x()));
}

void SliderBar::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton)
    emit moved(_positionAt(event->position().toPoint().x()));
}

// ---------------------------------------------------------------------------------------------
// NumberControl

NumberControl::NumberControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _slider(nullptr),
    _field(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(CONTROL_SPACING);

  Setting* current = setting();
  double minimum = 0.0;
  double maximum = 0.0;
  if (current && sliderBounds(current->info(), &minimum, &maximum)) {
    _slider = new SliderBar(this);
    if (minimum < 0.0)
      _slider->setOrigin(positionOf(0.0, minimum, maximum));
    connect(_slider, &SliderBar::moved, this, [this](double position) {
      Setting* target = setting();
      if (!target)
        return;
      if (target->hasNormalized()) {
        if (target->setNormalized(position)) {
          refresh();
          emit edited();
        }
        return;
      }
      // A slider range without bounds: the slider writes the value at its position.
      double low = 0.0;
      double high = 0.0;
      if (!sliderBounds(target->info(), &low, &high))
        return;
      double value = low + position * (high - low);
      if (target->info().type() == SettingInfo::Integer)
        value = std::round(value);
      write(value);
    });
    layout->addWidget(_slider, 1);
  }
  else {
    layout->addStretch(1);
  }

  _field = makeField(this, FIELD_WIDTH);
  _field->installEventFilter(this);
  connect(_field, &QLineEdit::editingFinished, this, &NumberControl::_commitField);
  layout->addWidget(_field);

  refresh();
}

void NumberControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  showInField(_field, SettingFormat::formatNumber(current->displayValue().toDouble(), current->info()));
  double minimum = 0.0;
  double maximum = 0.0;
  if (_slider && sliderBounds(current->info(), &minimum, &maximum)) {
    // A value beyond the slider range puts the handle at the end.
    _slider->setPosition(positionOf(current->value().toDouble(), minimum, maximum));
    const QVariant live = liveValue();
    if (live.isValid() && live != current->value())
      _slider->setLivePosition(positionOf(live.toDouble(), minimum, maximum));
    else
      _slider->setLivePosition(NO_LIVE_POSITION);
  }
}

void NumberControl::stepBy(double steps)
{
  Setting* current = setting();
  if (!current)
    return;
  const double step = SettingFormat::displayStep(current->info());
  writeDisplay(current->displayValue().toDouble() + steps * step);
}

void NumberControl::_commitField()
{
  if (!_field->isModified())
    return;
  _field->setModified(false);
  Setting* current = setting();
  if (!current)
    return;
  double value = 0.0;
  if (SettingFormat::parseNumber(_field->text(), current->info(), &value))
    writeDisplay(value);
  else
    refresh();
}

bool NumberControl::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == _field && event->type() == QEvent::KeyPress) {
    const double steps = arrowSteps(static_cast<QKeyEvent*>(event));
    if (steps != 0.0) {
      _field->setModified(false);
      stepBy(steps);
      _field->selectAll();
      return true;
    }
  }
  return SettingControl::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------------------------
// CounterControl

CounterControl::CounterControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QToolButton* minus = new QToolButton(this);
  minus->setText(QStringLiteral("−"));
  minus->setAutoRepeat(true);
  minus->setFixedWidth(COUNTER_BUTTON_WIDTH);
  connect(minus, &QToolButton::clicked, this, [this]() { stepBy(-ONE_STEP); });

  _field = makeField(this, COUNTER_FIELD_WIDTH);
  _field->setAlignment(Qt::AlignCenter);
  _field->installEventFilter(this);
  connect(_field, &QLineEdit::editingFinished, this, &CounterControl::_commitField);

  QToolButton* plus = new QToolButton(this);
  plus->setText(QStringLiteral("+"));
  plus->setAutoRepeat(true);
  plus->setFixedWidth(COUNTER_BUTTON_WIDTH);
  connect(plus, &QToolButton::clicked, this, [this]() { stepBy(ONE_STEP); });

  layout->addWidget(minus);
  layout->addWidget(_field);
  layout->addWidget(plus);
  refresh();
}

void CounterControl::refresh()
{
  Setting* current = setting();
  if (current)
    showInField(_field, SettingFormat::formatNumber(current->displayValue().toDouble(), current->info()));
}

void CounterControl::stepBy(double steps)
{
  Setting* current = setting();
  if (!current)
    return;
  const double step = SettingFormat::displayStep(current->info());
  writeDisplay(current->displayValue().toDouble() + steps * step);
}

void CounterControl::_commitField()
{
  if (!_field->isModified())
    return;
  _field->setModified(false);
  Setting* current = setting();
  if (!current)
    return;
  double value = 0.0;
  if (SettingFormat::parseNumber(_field->text(), current->info(), &value))
    writeDisplay(value);
  else
    refresh();
}

bool CounterControl::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == _field && event->type() == QEvent::KeyPress) {
    const double steps = arrowSteps(static_cast<QKeyEvent*>(event));
    if (steps != 0.0) {
      _field->setModified(false);
      stepBy(steps);
      _field->selectAll();
      return true;
    }
  }
  return SettingControl::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------------------------
// SwitchControl

SwitchControl::SwitchControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _on(false)
{
  setFocusPolicy(Qt::StrongFocus);
  setCursor(Qt::PointingHandCursor);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  refresh();
}

void SwitchControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  _on = current->value().toBool();
  update();
}

QSize SwitchControl::sizeHint() const
{
  return QSize(SWITCH_WIDTH, SWITCH_HEIGHT);
}

void SwitchControl::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const QRectF frame(0.5, 0.5, SWITCH_WIDTH - 1, SWITCH_HEIGHT - 1);
  const double radius = frame.height() / 2.0;
  painter.setPen(hasFocus() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Mid));
  painter.setBrush(_on && isEnabled() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Base));
  painter.drawRoundedRect(frame, radius, radius);

  const double knob = frame.height() - 2 * SWITCH_GAP;
  const double x = _on ? frame.right() - SWITCH_GAP - knob : frame.left() + SWITCH_GAP;
  painter.setPen(Qt::NoPen);
  painter.setBrush(_on ? palette().color(QPalette::HighlightedText) : palette().color(QPalette::Mid));
  painter.drawEllipse(QRectF(x, frame.top() + SWITCH_GAP, knob, knob));
}

void SwitchControl::mousePressEvent(QMouseEvent* event)
{
  // Accepted, so that the release comes back to the switch.
  if (event->button() == Qt::LeftButton)
    event->accept();
  else
    SettingControl::mousePressEvent(event);
}

void SwitchControl::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
    _toggle();
}

void SwitchControl::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    _toggle();
  else
    SettingControl::keyPressEvent(event);
}

void SwitchControl::_toggle()
{
  write(!_on);
}

// ---------------------------------------------------------------------------------------------
// RefreshingComboBox

RefreshingComboBox::RefreshingComboBox(QWidget* parent)
  : QComboBox(parent)
{
}

void RefreshingComboBox::showPopup()
{
  if (_aboutToShow)
    _aboutToShow();
  QComboBox::showPopup();
}

// ---------------------------------------------------------------------------------------------
// ChoiceControl

bool ChoiceControl::fitsButtons(const QList<SettingOption>& options)
{
  if (options.isEmpty() || options.size() > MAX_BUTTON_OPTIONS)
    return false;
  int characters = 0;
  for (const SettingOption& option : options)
    characters += option.label.size();
  return characters <= MAX_BUTTON_LABEL_CHARACTERS;
}

ChoiceControl::ChoiceControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _buttons(nullptr),
    _menu(nullptr),
    _filling(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  Setting* current = setting();
  const QList<SettingOption> options = current ? current->info().options() : QList<SettingOption>();
  bool valueIsAnOption = false;
  for (const SettingOption& option : options)
    valueIsAnOption |= current && option.id == current->value().toString();

  // A reference names elements that come and go: always a menu.
  const bool buttons = current && current->info().type() == SettingInfo::Choice
      && valueIsAnOption && fitsButtons(options);
  if (buttons) {
    _buttons = new QButtonGroup(this);
    _buttons->setExclusive(true);
    for (int index = 0; index < options.size(); index++) {
      QPushButton* button = new QPushButton(options.at(index).label, this);
      button->setCheckable(true);
      button->setFocusPolicy(Qt::TabFocus);
      _buttons->addButton(button, index);
      _optionIds << options.at(index).id;
      layout->addWidget(button);
    }
    connect(_buttons, &QButtonGroup::idClicked, this, [this](int index) {
      if (index >= 0 && index < _optionIds.size())
        write(_optionIds.at(index));
    });
  }
  else {
    _menu = new RefreshingComboBox(this);
    _menu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // Options that come from a provider may change: ask for them each time the menu opens.
    _menu->setAboutToShow([this]() { _fillMenu(); });
    connect(_menu, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
      if (!_filling && index >= 0 && index < _optionIds.size())
        write(_optionIds.at(index));
    });
    layout->addWidget(_menu);
  }
  refresh();
}

void ChoiceControl::_fillMenu()
{
  Setting* current = setting();
  if (!current || !_menu)
    return;
  _filling = true;
  const QString value = current->value().toString();
  const QList<SettingOption> options = current->info().options();
  _menu->clear();
  _optionIds.clear();
  int selected = -1;
  for (const SettingOption& option : options) {
    if (option.id == value)
      selected = _optionIds.size();
    _menu->addItem(option.label);
    _optionIds << option.id;
  }
  // An entry that disappeared is still shown, so that the user sees what the setting holds.
  if (selected < 0 && !value.isEmpty()) {
    selected = _optionIds.size();
    _menu->addItem(tr("%1 (missing)").arg(value));
    _optionIds << value;
  }
  if (_menu->count() == 0) {
    _menu->addItem(tr("(none)"));
    _optionIds << QString();
    selected = 0;
  }
  _menu->setCurrentIndex(selected);
  _filling = false;
}

void ChoiceControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  if (_buttons) {
    const int index = _optionIds.indexOf(current->value().toString());
    if (index >= 0) {
      _buttons->button(index)->setChecked(true);
    }
    else if (QAbstractButton* checked = _buttons->checkedButton()) {
      // A value that is not among the buttons: none is checked.
      _buttons->setExclusive(false);
      checked->setChecked(false);
      _buttons->setExclusive(true);
    }
  }
  else {
    _fillMenu();
  }
}

// ---------------------------------------------------------------------------------------------
// TextControl

TextControl::TextControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _field(new QLineEdit(this)),
    _browse(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(CONTROL_SPACING);
  layout->addWidget(_field, 1);
  connect(_field, &QLineEdit::editingFinished, this, [this]() {
    if (!_field->isModified())
      return;
    _field->setModified(false);
    write(_field->text());
  });

  Setting* current = setting();
  if (current && current->info().isFilePath()) {
    _browse = new QToolButton(this);
    _browse->setText(QStringLiteral("…"));
    _browse->setToolTip(tr("Choose a file"));
    connect(_browse, &QToolButton::clicked, this, [this]() {
      Setting* target = setting();
      if (!target)
        return;
      const QString directory = QFileInfo(target->value().toString()).absolutePath();
      const QString file = QFileDialog::getOpenFileName(this, target->info().label(), directory,
                                                        target->info().fileFilter());
      if (!file.isEmpty())
        write(file);
    });
    layout->addWidget(_browse);
  }
  refresh();
}

void TextControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  showInField(_field, current->value().toString());
  // A path is long: show its end, where the file name is.
  if (current->info().isFilePath() && !_field->hasFocus())
    _field->setCursorPosition(_field->text().size());
  _field->setReadOnly(current->info().isReadOnly());
}

// ---------------------------------------------------------------------------------------------
// PointPad

PointPad::PointPad(QWidget* parent)
  : QWidget(parent)
{
  setFixedSize(PAD_SIZE, PAD_SIZE);
  setCursor(Qt::CrossCursor);
}

void PointPad::setPosition(const QPointF& position)
{
  _position = QPointF(qBound(0.0, position.x(), 1.0), qBound(0.0, position.y(), 1.0));
  update();
}

QSize PointPad::sizeHint() const
{
  return QSize(PAD_SIZE, PAD_SIZE);
}

QPointF PointPad::_positionAt(const QPoint& point) const
{
  return QPointF(qBound(0.0, double(point.x()) / qMax(1, width() - 1), 1.0),
                 qBound(0.0, double(point.y()) / qMax(1, height() - 1), 1.0));
}

void PointPad::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(palette().color(QPalette::Mid));
  painter.setBrush(palette().color(QPalette::Base));
  painter.drawRect(rect().adjusted(0, 0, -1, -1));

  const QPointF center(_position.x() * (width() - 1), _position.y() * (height() - 1));
  painter.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DotLine));
  painter.drawLine(QPointF(center.x(), 0), QPointF(center.x(), height()));
  painter.drawLine(QPointF(0, center.y()), QPointF(width(), center.y()));
  painter.setPen(palette().color(QPalette::HighlightedText));
  painter.setBrush(palette().color(QPalette::Highlight));
  painter.drawEllipse(center, PAD_POINT_SIZE / 2.0, PAD_POINT_SIZE / 2.0);
}

void PointPad::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
    emit moved(_positionAt(event->position().toPoint()));
}

void PointPad::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton)
    emit moved(_positionAt(event->position().toPoint()));
}

// ---------------------------------------------------------------------------------------------
// PointControl

PointControl::PointControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _pad(nullptr),
    _x(nullptr),
    _y(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(CONTROL_SPACING);

  Setting* current = setting();
  double minimum = 0.0;
  double maximum = 0.0;
  if (current && numberBounds(current->info(), &minimum, &maximum)) {
    _pad = new PointPad(this);
    connect(_pad, &PointPad::moved, this, [this](const QPointF& position) {
      Setting* target = setting();
      double low = 0.0;
      double high = 0.0;
      if (!target || !numberBounds(target->info(), &low, &high))
        return;
      write(QPointF(low + position.x() * (high - low), low + position.y() * (high - low)));
    });
    layout->addWidget(_pad);
  }
  layout->addStretch(1);

  QVBoxLayout* fields = new QVBoxLayout;
  fields->setSpacing(CONTROL_SPACING / 2);
  _x = makeField(this, FIELD_WIDTH);
  _y = makeField(this, FIELD_WIDTH);
  _x->setPlaceholderText(QStringLiteral("X"));
  _y->setPlaceholderText(QStringLiteral("Y"));
  _x->setToolTip(QStringLiteral("X"));
  _y->setToolTip(QStringLiteral("Y"));
  for (QLineEdit* field : { _x, _y })
    field->installEventFilter(this);
  connect(_x, &QLineEdit::editingFinished, this, [this]() { _commitField(0); });
  connect(_y, &QLineEdit::editingFinished, this, [this]() { _commitField(1); });
  fields->addWidget(_x);
  fields->addWidget(_y);
  if (!_pad)
    fields->setDirection(QBoxLayout::LeftToRight);
  layout->addLayout(fields);
  refresh();
}

void PointControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  const QPointF point = current->value().toPointF();
  const double scale = current->info().displayScale();
  showInField(_x, SettingFormat::formatNumber(point.x() * scale, current->info()));
  showInField(_y, SettingFormat::formatNumber(point.y() * scale, current->info()));
  double minimum = 0.0;
  double maximum = 0.0;
  if (_pad && numberBounds(current->info(), &minimum, &maximum))
    _pad->setPosition(QPointF(positionOf(point.x(), minimum, maximum), positionOf(point.y(), minimum, maximum)));
}

void PointControl::_writeCoordinate(int coordinate, double displayValue)
{
  Setting* current = setting();
  if (!current)
    return;
  const double scale = current->info().displayScale();
  if (scale == 0.0)
    return;
  QPointF point = current->value().toPointF();
  if (coordinate == 0)
    point.setX(displayValue / scale);
  else
    point.setY(displayValue / scale);
  write(point);
}

void PointControl::_commitField(int coordinate)
{
  QLineEdit* edited = field(coordinate);
  if (!edited->isModified())
    return;
  edited->setModified(false);
  Setting* current = setting();
  double value = 0.0;
  if (current && SettingFormat::parseNumber(edited->text(), current->info(), &value))
    _writeCoordinate(coordinate, value);
  else
    refresh();
}

bool PointControl::eventFilter(QObject* watched, QEvent* event)
{
  if ((watched == _x || watched == _y) && event->type() == QEvent::KeyPress) {
    const double steps = arrowSteps(static_cast<QKeyEvent*>(event));
    Setting* current = setting();
    if (steps != 0.0 && current) {
      const int coordinate = watched == _x ? 0 : 1;
      const QPointF point = current->value().toPointF();
      const double scale = current->info().displayScale();
      const double shown = (coordinate == 0 ? point.x() : point.y()) * scale;
      field(coordinate)->setModified(false);
      _writeCoordinate(coordinate, shown + steps * SettingFormat::displayStep(current->info()));
      field(coordinate)->selectAll();
      return true;
    }
  }
  return SettingControl::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------------------------
// RangeBar

RangeBar::RangeBar(QWidget* parent)
  : QWidget(parent),
    _low(0.0),
    _high(1.0),
    _dragged(0)
{
  setMinimumHeight(BAR_HEIGHT);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setCursor(Qt::PointingHandCursor);
}

void RangeBar::setPositions(double low, double high)
{
  _low = qBound(0.0, low, 1.0);
  _high = qBound(_low, high, 1.0);
  update();
}

QSize RangeBar::sizeHint() const
{
  return QSize(BAR_WIDTH, BAR_HEIGHT);
}

double RangeBar::_positionAt(int x) const
{
  const int usable = width() - HANDLE_SIZE;
  if (usable <= 0)
    return 0.0;
  return qBound(0.0, double(x - HANDLE_SIZE / 2) / usable, 1.0);
}

void RangeBar::_drag(double position)
{
  if (_dragged == 0)
    _low = qMin(position, _high);
  else
    _high = qMax(position, _low);
  update();
  emit moved(_low, _high);
}

void RangeBar::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const int usable = width() - HANDLE_SIZE;
  const double centerY = height() / 2.0;
  const QRectF track(HANDLE_SIZE / 2.0, centerY - TRACK_HEIGHT / 2.0, usable, TRACK_HEIGHT);

  painter.setPen(Qt::NoPen);
  painter.setBrush(palette().color(QPalette::Mid));
  painter.drawRoundedRect(track, TRACK_HEIGHT / 2.0, TRACK_HEIGHT / 2.0);

  const double lowX = track.left() + _low * usable;
  const double highX = track.left() + _high * usable;
  painter.setBrush(isEnabled() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Dark));
  painter.drawRect(QRectF(lowX, track.top(), highX - lowX, TRACK_HEIGHT));

  painter.setBrush(palette().color(QPalette::Button));
  painter.setPen(palette().color(QPalette::Highlight));
  for (double x : { lowX, highX })
    painter.drawEllipse(QPointF(x, centerY), HANDLE_SIZE / 2.0 - 1, HANDLE_SIZE / 2.0 - 1);
}

void RangeBar::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
    return;
  const double position = _positionAt(event->position().toPoint().x());
  // The nearest handle; when both are on the same spot, the one on the side of the click.
  if (std::fabs(position - _low) < std::fabs(position - _high))
    _dragged = 0;
  else if (std::fabs(position - _low) > std::fabs(position - _high))
    _dragged = 1;
  else
    _dragged = position < _low ? 0 : 1;
  _drag(position);
}

void RangeBar::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton)
    _drag(_positionAt(event->position().toPoint().x()));
}

// ---------------------------------------------------------------------------------------------
// RangeControl

RangeControl::RangeControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _bar(nullptr),
    _low(nullptr),
    _high(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(CONTROL_SPACING);

  _low = makeField(this, SMALL_FIELD_WIDTH);
  layout->addWidget(_low);

  Setting* current = setting();
  double minimum = 0.0;
  double maximum = 0.0;
  if (current && numberBounds(current->info(), &minimum, &maximum)) {
    _bar = new RangeBar(this);
    connect(_bar, &RangeBar::moved, this, [this](double low, double high) {
      Setting* target = setting();
      double lowest = 0.0;
      double highest = 0.0;
      if (!target || !numberBounds(target->info(), &lowest, &highest))
        return;
      write(QPointF(lowest + low * (highest - lowest), lowest + high * (highest - lowest)));
    });
    layout->addWidget(_bar, 1);
  }
  else {
    layout->addStretch(1);
  }

  _high = makeField(this, SMALL_FIELD_WIDTH);
  layout->addWidget(_high);
  connect(_low, &QLineEdit::editingFinished, this, [this]() { _commitField(0); });
  connect(_high, &QLineEdit::editingFinished, this, [this]() { _commitField(1); });
  refresh();
}

void RangeControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  const QPointF range = current->value().toPointF();
  const double scale = current->info().displayScale();
  showInField(_low, SettingFormat::formatNumber(range.x() * scale, current->info()));
  showInField(_high, SettingFormat::formatNumber(range.y() * scale, current->info()));
  double minimum = 0.0;
  double maximum = 0.0;
  if (_bar && numberBounds(current->info(), &minimum, &maximum))
    _bar->setPositions(positionOf(range.x(), minimum, maximum), positionOf(range.y(), minimum, maximum));
}

void RangeControl::_commitField(int bound)
{
  QLineEdit* edited = field(bound);
  if (!edited->isModified())
    return;
  edited->setModified(false);
  Setting* current = setting();
  double value = 0.0;
  const double scale = current ? current->info().displayScale() : 0.0;
  if (!current || scale == 0.0 || !SettingFormat::parseNumber(edited->text(), current->info(), &value)) {
    refresh();
    return;
  }
  // A bound typed past the other one stops at it: the handles do not cross.
  QPointF range = current->value().toPointF();
  if (bound == 0)
    range.setX(qMin(value / scale, range.y()));
  else
    range.setY(qMax(value / scale, range.x()));
  write(range);
}

// ---------------------------------------------------------------------------------------------
// ColorControl

ColorControl::ColorControl(SettingGroup* root, const QString& path, QWidget* parent)
  : SettingControl(root, path, parent),
    _swatch(new QToolButton(this)),
    _field(new QLineEdit(this)),
    _picker(nullptr)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(CONTROL_SPACING);

  QHBoxLayout* line = new QHBoxLayout;
  line->setSpacing(CONTROL_SPACING);
  _swatch->setFixedSize(SWATCH_SIZE, SWATCH_SIZE);
  _swatch->setCheckable(true);
  _swatch->setToolTip(tr("Show the colour picker"));
  connect(_swatch, &QToolButton::toggled, this, &ColorControl::setPickerShown);
  _field->setFixedWidth(FIELD_WIDTH + SWATCH_SIZE);
  connect(_field, &QLineEdit::editingFinished, this, [this]() {
    if (!_field->isModified())
      return;
    _field->setModified(false);
    const QColor color(_field->text().trimmed());
    if (!color.isValid()) {
      refresh();
      return;
    }
    // The code has no alpha: keep the one of the setting.
    QColor kept = color;
    if (Setting* current = setting())
      kept.setAlpha(current->value().value<QColor>().alpha());
    if (write(kept))
      ColorPicker::addRecentColor(kept);
  });
  line->addWidget(_swatch);
  line->addWidget(_field);
  line->addStretch(1);
  layout->addLayout(line);
  refresh();
}

void ColorControl::setPickerShown(bool shown)
{
  if (shown && !_picker) {
    _picker = new ColorPicker(this);
    // The code is already on the line of the swatch.
    _picker->setCodeShown(false);
    connect(_picker, &ColorPicker::colorChanged, this, [this](const QColor& color) { write(color); });
    connect(_picker, &ColorPicker::gestureFinished, this, [this]() {
      if (Setting* current = setting())
        ColorPicker::addRecentColor(current->value().value<QColor>());
    });
    layout()->addWidget(_picker);
    refresh();
  }
  if (_picker)
    _picker->setVisible(shown);
  if (_swatch->isChecked() != shown)
    _swatch->setChecked(shown);
}

void ColorControl::refresh()
{
  Setting* current = setting();
  if (!current)
    return;
  const QColor color = current->value().value<QColor>();
  _swatch->setStyleSheet(QStringLiteral("QToolButton { background-color: %1; border: 1px solid palette(mid); }")
                         .arg(color.name()));
  showInField(_field, color.name().toUpper());
  if (_picker)
    _picker->setColor(color);
}

} // namespace mmp
