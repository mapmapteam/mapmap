/*
 * ColorPicker.cpp
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

#include "ColorPicker.h"

#include <functional>

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

namespace mmp {

namespace {

/// Key of the recent colours in the preferences of the user.
const QString RECENT_COLORS_KEY = QStringLiteral("inspector/recentColors");

/// Height of the square of saturation and brightness, in pixels.
const int SQUARE_HEIGHT = 120;

/// Height of the bar of hue, in pixels.
const int HUE_BAR_HEIGHT = 14;

/// Diameter of the ring showing the colour in the square, in pixels.
const int RING_SIZE = 10;

/// Side of a recent colour, in pixels.
const int RECENT_SIZE = 18;

/// Spacing inside the picker, in pixels.
const int PICKER_SPACING = 6;

/// Width of the field of the code, in pixels.
const int CODE_WIDTH = 86;

/// Number of stops of the gradient of the hue bar.
const int HUE_STOPS = 6;

/// Width of the marker of the hue bar, in pixels.
const int HUE_MARKER_WIDTH = 3;

} // namespace

/// The square in which the user chooses the saturation (across) and the brightness (up).
class SaturationValueSquare : public QWidget {
public:
  explicit SaturationValueSquare(QWidget* parent)
    : QWidget(parent), hue(0.0), saturation(0.0), value(0.0)
  {
    setMinimumHeight(SQUARE_HEIGHT);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::CrossCursor);
  }

  double hue;
  double saturation;
  double value;
  /// Called with the saturation and brightness under the mouse.
  std::function<void(double, double)> moved;
  /// Called when the mouse is released.
  std::function<void()> released;

protected:
  void paintEvent(QPaintEvent*) override
  {
    QPainter painter(this);
    const QRect area = rect();
    // Hue from white to the pure colour, then darkened from the top down.
    QLinearGradient across(area.topLeft(), area.topRight());
    across.setColorAt(0, Qt::white);
    across.setColorAt(1, QColor::fromHsvF(float(hue), 1.0f, 1.0f));
    painter.fillRect(area, across);
    QLinearGradient down(area.topLeft(), area.bottomLeft());
    down.setColorAt(0, QColor(0, 0, 0, 0));
    down.setColorAt(1, Qt::black);
    painter.fillRect(area, down);

    painter.setRenderHint(QPainter::Antialiasing);
    const QPointF center(saturation * (width() - 1), (1.0 - value) * (height() - 1));
    painter.setPen(QPen(value > 0.5 ? Qt::black : Qt::white, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, RING_SIZE / 2.0, RING_SIZE / 2.0);
  }

  void mousePressEvent(QMouseEvent* event) override { _move(event->position().toPoint()); }
  void mouseMoveEvent(QMouseEvent* event) override
  {
    if (event->buttons() & Qt::LeftButton)
      _move(event->position().toPoint());
  }
  void mouseReleaseEvent(QMouseEvent*) override
  {
    if (released)
      released();
  }

private:
  void _move(const QPoint& point)
  {
    const double s = qBound(0.0, double(point.x()) / qMax(1, width() - 1), 1.0);
    const double v = 1.0 - qBound(0.0, double(point.y()) / qMax(1, height() - 1), 1.0);
    if (moved)
      moved(s, v);
  }
};

/// The bar in which the user chooses the hue.
class HueBar : public QWidget {
public:
  explicit HueBar(QWidget* parent) : QWidget(parent), hue(0.0)
  {
    setMinimumHeight(HUE_BAR_HEIGHT);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
  }

  double hue;
  /// Called with the hue under the mouse.
  std::function<void(double)> moved;
  /// Called when the mouse is released.
  std::function<void()> released;

protected:
  void paintEvent(QPaintEvent*) override
  {
    QPainter painter(this);
    QLinearGradient gradient(rect().topLeft(), rect().topRight());
    for (int stop = 0; stop <= HUE_STOPS; stop++)
      gradient.setColorAt(double(stop) / HUE_STOPS,
                          QColor::fromHsvF(float(stop % HUE_STOPS) / HUE_STOPS, 1.0f, 1.0f));
    painter.fillRect(rect(), gradient);
    const double x = hue * (width() - 1);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(x - HUE_MARKER_WIDTH / 2.0, 0, HUE_MARKER_WIDTH, height() - 1));
  }

  void mousePressEvent(QMouseEvent* event) override { _move(event->position().toPoint()); }
  void mouseMoveEvent(QMouseEvent* event) override
  {
    if (event->buttons() & Qt::LeftButton)
      _move(event->position().toPoint());
  }
  void mouseReleaseEvent(QMouseEvent*) override
  {
    if (released)
      released();
  }

private:
  void _move(const QPoint& point)
  {
    if (moved)
      moved(qBound(0.0, double(point.x()) / qMax(1, width() - 1), 1.0));
  }
};

ColorPicker::ColorPicker(QWidget* parent)
  : QWidget(parent),
    _square(new SaturationValueSquare(this)),
    _hueBar(new HueBar(this)),
    _code(new QLineEdit(this)),
    _pipette(new QToolButton(this)),
    _recentLayout(new QHBoxLayout),
    _hue(0.0),
    _saturation(0.0),
    _value(0.0),
    _alpha(255),
    _picking(false)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(PICKER_SPACING);
  layout->addWidget(_square);
  layout->addWidget(_hueBar);

  QHBoxLayout* line = new QHBoxLayout;
  line->setSpacing(PICKER_SPACING);
  _code->setFixedWidth(CODE_WIDTH);
  _code->setToolTip(tr("Code of the colour, such as #FF8800"));
  _pipette->setText(tr("Pick"));
  _pipette->setCheckable(true);
  _pipette->setToolTip(tr("Take a colour in a window of MapMap, such as the output"));
  line->addWidget(_code);
  line->addWidget(_pipette);
  line->addStretch(1);
  layout->addLayout(line);

  _recentLayout->setSpacing(PICKER_SPACING / 2);
  layout->addLayout(_recentLayout);

  _square->moved = [this](double s, double v) {
    _saturation = s;
    _value = v;
    _showColor();
    _emitColor();
  };
  _square->released = [this]() { emit gestureFinished(); };
  _hueBar->moved = [this](double h) {
    _hue = h;
    _showColor();
    _emitColor();
  };
  _hueBar->released = [this]() { emit gestureFinished(); };

  connect(_code, &QLineEdit::editingFinished, this, [this]() {
    if (!_code->isModified())
      return;
    _code->setModified(false);
    QColor typed(_code->text().trimmed());
    if (!typed.isValid()) {
      _showColor();
      return;
    }
    typed.setAlpha(_alpha);
    setColor(typed);
    _emitColor();
    emit gestureFinished();
  });
  connect(_pipette, &QToolButton::toggled, this, &ColorPicker::_setPicking);
  // The control puts the colour in the recent colours at the end of a gesture: show them
  // again once it has.
  connect(this, &ColorPicker::gestureFinished, this, &ColorPicker::_showRecentColors, Qt::QueuedConnection);

  _showColor();
  _showRecentColors();
}

void ColorPicker::setColor(const QColor& color)
{
  if (!color.isValid())
    return;
  const QColor hsv = color.toHsv();
  // A grey has no hue: keep the one shown, so that the hue does not jump.
  if (hsv.hsvHueF() >= 0 && hsv.hsvSaturationF() > 0 && hsv.valueF() > 0)
    _hue = hsv.hsvHueF();
  _saturation = hsv.hsvSaturationF();
  _value = hsv.valueF();
  _alpha = color.alpha();
  _showColor();
}

void ColorPicker::setCodeShown(bool shown)
{
  _code->setVisible(shown);
}

QColor ColorPicker::color() const
{
  QColor made = QColor::fromHsvF(float(_hue), float(_saturation), float(_value));
  made.setAlpha(_alpha);
  return made;
}

void ColorPicker::_emitColor()
{
  emit colorChanged(color());
}

void ColorPicker::_showColor()
{
  _square->hue = _hue;
  _square->saturation = _saturation;
  _square->value = _value;
  _square->update();
  _hueBar->hue = _hue;
  _hueBar->update();
  if (!_code->hasFocus() || !_code->isModified())
    _code->setText(color().name().toUpper());
}

QList<QColor> ColorPicker::recentColors()
{
  QList<QColor> colors;
  const QStringList names = QSettings().value(RECENT_COLORS_KEY).toStringList();
  for (const QString& name : names) {
    const QColor color(name);
    if (color.isValid())
      colors << color;
  }
  return colors;
}

void ColorPicker::addRecentColor(const QColor& color)
{
  if (!color.isValid())
    return;
  QStringList names = QSettings().value(RECENT_COLORS_KEY).toStringList();
  const QString name = color.name(QColor::HexArgb);
  names.removeAll(name);
  names.prepend(name);
  while (names.size() > MAX_RECENT_COLORS)
    names.removeLast();
  QSettings().setValue(RECENT_COLORS_KEY, names);
}

void ColorPicker::_showRecentColors()
{
  while (QLayoutItem* item = _recentLayout->takeAt(0)) {
    delete item->widget();
    delete item;
  }
  const QList<QColor> colors = recentColors();
  for (const QColor& recent : colors) {
    QToolButton* button = new QToolButton(this);
    button->setFixedSize(RECENT_SIZE, RECENT_SIZE);
    button->setToolTip(recent.name().toUpper());
    button->setStyleSheet(QStringLiteral("QToolButton { background-color: %1; border: 1px solid palette(mid); }")
                          .arg(recent.name()));
    connect(button, &QToolButton::clicked, this, [this, recent]() {
      setColor(recent);
      _emitColor();
      emit gestureFinished();
    });
    _recentLayout->addWidget(button);
  }
  _recentLayout->addStretch(1);
}

QColor ColorPicker::colorAtGlobalPoint(const QPoint& point)
{
  QWidget* widget = QApplication::widgetAt(point);
  if (!widget)
    return QColor();
  QWidget* window = widget->window();
  const QPoint local = window->mapFromGlobal(point);
  const QImage image = window->grab(QRect(local, QSize(1, 1))).toImage();
  if (image.isNull())
    return QColor();
  return image.pixelColor(0, 0);
}

void ColorPicker::_setPicking(bool picking)
{
  if (picking == _picking)
    return;
  _picking = picking;
  if (picking) {
    qApp->installEventFilter(this);
    QApplication::setOverrideCursor(Qt::CrossCursor);
  }
  else {
    qApp->removeEventFilter(this);
    QApplication::restoreOverrideCursor();
  }
  if (_pipette->isChecked() != picking)
    _pipette->setChecked(picking);
}

bool ColorPicker::eventFilter(QObject* watched, QEvent* event)
{
  if (_picking) {
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
      _setPicking(false);
      return true;
    }
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent* press = static_cast<QMouseEvent*>(event);
      // A click on the pipette itself stops picking, through its own toggle.
      if (watched == _pipette)
        return false;
      const QColor picked = colorAtGlobalPoint(press->globalPosition().toPoint());
      _setPicking(false);
      if (picked.isValid()) {
        QColor kept = picked;
        kept.setAlpha(_alpha);
        setColor(kept);
        _emitColor();
        emit gestureFinished();
      }
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

} // namespace mmp
