/*
 * Inspector.cpp
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

#include "Inspector.h"

#include <algorithm>
#include <limits>

#include <QHBoxLayout>
#include <QContextMenuEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "LearnMode.h"

namespace mmp {

namespace {

/// Position of a display group used by settings but described nowhere: after the others.
const int UNDESCRIBED_DISPLAY_ORDER = std::numeric_limits<int>::max();

/// Prefix of the keys of the folded groups in the preferences.
const QString FOLDED_KEY_PREFIX = QStringLiteral("inspector/folded/");

/// Key of a group made for a group of the tree, followed by its path.
const QString TREE_GROUP_KEY = QStringLiteral("group:");

/// Key of a group made for a display group, followed by the path of its tree group.
const QString DISPLAY_GROUP_KEY = QStringLiteral("display:");

/// Separates the path of the tree group from the id of a display group in a key.
const QChar DISPLAY_GROUP_KEY_SEPARATOR('#');

/// Steps moved per pixel when the label is dragged with Shift pressed.
const double FINE_STEPS_PER_PIXEL = 0.1;

/// Room kept at the end of a row for the button of a link, in pixels.
const int LINK_BUTTON_ROOM = 20;

/// The text of the button of a link: a keyboard.
const QString LINK_BUTTON_TEXT = QStringLiteral("\u2328");

/// Spacing between rows and inside a row, in pixels.
const int ROW_SPACING = 6;

/// Indentation of the rows of a group, in pixels.
const int GROUP_INDENT = 8;

/// Margins of the content of a page, in pixels.
const int PAGE_MARGIN = 8;

/// White of MapMap's style sheet: text, and the filled part of the controls.
const QColor MAPMAP_WHITE(0xf6, 0xf5, 0xf5);

/// Dark gray of MapMap's style sheet: the handles, and the text on white.
const QColor MAPMAP_DARK_GRAY(0x32, 0x35, 0x41);

/// Darker gray of MapMap's style sheet: the inside of fields, switches and pads.
const QColor MAPMAP_DARKER_GRAY(0x27, 0x2a, 0x36);

/// Lighter gray of MapMap's style sheet: the empty part of the tracks, and the borders.
const QColor MAPMAP_LIGHTER_GRAY(0x4c, 0x4f, 0x5b);

/// Opacity of the white of disabled text and controls, between 0 and 255.
const int DISABLED_WHITE_ALPHA = 110;

/// Opacity of the white of secondary text, such as the kind of the element.
const int SECONDARY_WHITE_ALPHA = 150;

/// Colour of the outline of a control one can link, while learning.
const QColor LEARN_OUTLINE_COLOR(0x3d, 0xae, 0xe9);

/// Opacity of the filling of the control chosen while learning, between 0 and 255.
const int LEARN_CHOSEN_ALPHA = 70;

/// Width of the outline of a control one can link, in pixels.
const int LEARN_OUTLINE_WIDTH = 2;

/// Radius of the corners of the outline, in pixels.
const int LEARN_OUTLINE_RADIUS = 3;

/// Room around a control for its outline, in pixels.
const int LEARN_OUTLINE_MARGIN = 2;

/// Horizontal padding of the trigger written on the outline, in pixels.
const int LEARN_BADGE_PADDING = 4;

/// Joins the labels of settings sharing a row.
const QString SHARED_LABEL_SEPARATOR = QStringLiteral(" × ");

/// Returns how the panel shows the display group id used in group, described on group or on
/// a group holding it; a display group described nowhere is titled with its id, open, last.
SettingDisplayGroup describeDisplayGroup(SettingGroup* group, const QString& id)
{
  for (SettingGroup* holder = group; holder; holder = holder->parent()) {
    const QList<SettingDisplayGroup> groups = holder->displayGroups();
    for (const SettingDisplayGroup& description : groups) {
      if (description.id == id)
        return description;
    }
  }
  return { id, id, UNDESCRIBED_DISPLAY_ORDER, false };
}

/// Returns true when the setting can be changed by the user.
bool isEditable(const Setting* setting)
{
  return setting && !setting->info().isReadOnly() && setting->isAvailable();
}

} // namespace

// ---------------------------------------------------------------------------------------------
// InspectorRowLabel

InspectorRowLabel::InspectorRowLabel(const QString& text, QWidget* parent)
  : QWidget(parent),
    _text(text),
    _control(nullptr),
    _lastX(0),
    _dragging(false)
{
  setFixedWidth(InspectorRow::LABEL_WIDTH);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  setToolTip(text);
}

void InspectorRowLabel::setControl(SettingControl* control)
{
  _control = control;
  setCursor(control && control->acceptsSteps() ? Qt::SizeHorCursor : Qt::ArrowCursor);
}

QSize InspectorRowLabel::sizeHint() const
{
  return QSize(InspectorRow::LABEL_WIDTH, fontMetrics().height());
}

void InspectorRowLabel::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
  const QString shown = fontMetrics().elidedText(_text, Qt::ElideRight, width());
  painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, shown);
}

void InspectorRowLabel::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && _control && _control->acceptsSteps() && _control->isEnabled()) {
    _dragging = true;
    _lastX = event->position().toPoint().x();
    return;
  }
  QWidget::mousePressEvent(event);
}

void InspectorRowLabel::mouseMoveEvent(QMouseEvent* event)
{
  if (!_dragging || !_control)
    return;
  const int x = event->position().toPoint().x();
  const int pixels = (x - _lastX) / PIXELS_PER_STEP;
  if (pixels == 0)
    return;
  _lastX = x;
  // Shift for a fine adjustment: a tenth of a step per pixel.
  const double perPixel = (event->modifiers() & Qt::ShiftModifier) ? FINE_STEPS_PER_PIXEL : 1.0;
  _control->stepBy(pixels * perPixel);
}

void InspectorRowLabel::mouseReleaseEvent(QMouseEvent* event)
{
  _dragging = false;
  QWidget::mouseReleaseEvent(event);
}

// ---------------------------------------------------------------------------------------------
// LearnOverlay

namespace {

/**
 * The outline of a control one can link, shown on top of it while learning: it names the key
 * linked to the setting, and a click on it chooses the setting for the next key.
 */
class LearnOverlay : public QWidget {
public:
  LearnOverlay(SettingControl* control, QWidget* parent)
    : QWidget(parent),
      _control(control)
  {
    control->installEventFilter(this);
    LearnMode* learn = LearnMode::current();
    connect(learn, &LearnMode::changed, this, [this]() { _show(); });
    connect(learn, &LearnMode::linksChanged, this, [this]() { update(); });
    _show();
  }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override
  {
    if (watched == _control && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
      _place();
    return QWidget::eventFilter(watched, event);
  }

  void paintEvent(QPaintEvent*) override
  {
    LearnMode* learn = LearnMode::current();
    LinkTarget target;
    if (!learn || !learn->targetOf(_control->root(), _control->path(), &target))
      return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF frame = QRectF(rect()).adjusted(1, 1, -1, -1);
    if (learn->isChosen(target)) {
      QColor fill = LEARN_OUTLINE_COLOR;
      fill.setAlpha(LEARN_CHOSEN_ALPHA);
      painter.setBrush(fill);
    }
    painter.setPen(QPen(LEARN_OUTLINE_COLOR, LEARN_OUTLINE_WIDTH));
    painter.drawRoundedRect(frame, LEARN_OUTLINE_RADIUS, LEARN_OUTLINE_RADIUS);

    const QString trigger = learn->triggerText(target);
    if (!trigger.isEmpty()) {
      const int width = fontMetrics().horizontalAdvance(trigger) + 2 * LEARN_BADGE_PADDING;
      const QRect badge(rect().right() - width, rect().top(), width, rect().height());
      painter.fillRect(badge, LEARN_OUTLINE_COLOR);
      painter.setPen(MAPMAP_DARK_GRAY);
      painter.drawText(badge, Qt::AlignCenter, trigger);
    }
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    LearnMode* learn = LearnMode::current();
    LinkTarget target;
    if (event->button() == Qt::LeftButton && learn
        && learn->targetOf(_control->root(), _control->path(), &target))
      learn->choose(target);
    event->accept();
  }

private:
  /// Follows the control, with room for the outline.
  void _place()
  {
    setGeometry(_control->geometry().adjusted(-LEARN_OUTLINE_MARGIN, -LEARN_OUTLINE_MARGIN,
                                              LEARN_OUTLINE_MARGIN, LEARN_OUTLINE_MARGIN));
  }

  /// Shown while learning, over the control.
  void _show()
  {
    LearnMode* learn = LearnMode::current();
    const bool shown = learn && learn->isActive() && canBeLinked(_control->setting());
    if (shown) {
      _place();
      raise();
    }
    setVisible(shown);
    update();
  }

  SettingControl* _control;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// InspectorRow

InspectorRow::InspectorRow(const QString& label, const QList<SettingControl*>& controls, QWidget* parent)
  : QWidget(parent),
    _label(new InspectorRowLabel(label, this)),
    _controls(controls),
    _linkButton(new QToolButton(this))
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(ROW_SPACING);
  layout->addWidget(_label, 0, Qt::AlignTop);

  QHBoxLayout* controlLayout = new QHBoxLayout;
  controlLayout->setSpacing(ROW_SPACING);
  for (SettingControl* control : _controls) {
    control->setParent(this);
    controlLayout->addWidget(control, 1);
    connect(control, &SettingControl::edited, this, &InspectorRow::edited);
  }
  layout->addLayout(controlLayout, 1);
  // The button of a link, shown when a setting of the row is linked; its room is kept.
  _linkButton->setFixedWidth(LINK_BUTTON_ROOM);
  _linkButton->setAutoRaise(true);
  _linkButton->setText(LINK_BUTTON_TEXT);
  QSizePolicy keptRoom = _linkButton->sizePolicy();
  keptRoom.setRetainSizeWhenHidden(true);
  _linkButton->setSizePolicy(keptRoom);
  _linkButton->setVisible(false);
  connect(_linkButton, &QToolButton::clicked, this, [this]() {
    QMenu* menu = createLinkMenu(this);
    if (!menu)
      return;
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(_linkButton->mapToGlobal(QPoint(0, _linkButton->height())));
  });
  layout->addWidget(_linkButton, 0, Qt::AlignTop);

  // Dragging the label adjusts the first setting of the row that moves by steps.
  for (SettingControl* control : _controls) {
    if (control->acceptsSteps()) {
      _label->setControl(control);
      break;
    }
  }

  // The sentence saying what the setting does, on the label and the controls.
  QStringList descriptions;
  for (SettingControl* control : _controls) {
    if (Setting* setting = control->setting()) {
      if (!setting->info().description().isEmpty())
        descriptions << setting->info().description();
    }
  }
  _description = descriptions.join(QChar('\n'));
  if (!_description.isEmpty())
    setToolTip(_description);
  updateEnabled();

  // While learning, the controls one can link are outlined, and a click chooses one. The
  // keys linked to the settings are named by the tooltip and the button of the link.
  if (LearnMode* learn = LearnMode::current()) {
    for (SettingControl* control : _controls) {
      if (canBeLinked(control->setting()))
        new LearnOverlay(control, this);
    }
    connect(learn, &LearnMode::linksChanged, this, &InspectorRow::_showLinks);
    _showLinks();
  }
}

void InspectorRow::_showLinks()
{
  LearnMode* learn = LearnMode::current();
  QStringList lines;
  for (SettingControl* control : _controls) {
    LinkTarget target;
    if (!learn || !learn->targetOf(control->root(), control->path(), &target))
      continue;
    const QStringList triggers = learn->triggerLines(target);
    // A row shared by several settings says which one each key drives.
    for (const QString& trigger : triggers) {
      lines << (_controls.size() > 1 && control->setting()
                ? tr("%1, %2").arg(control->setting()->info().label(), trigger) : trigger);
    }
  }
  QStringList tip;
  if (!_description.isEmpty())
    tip << _description;
  tip << lines;
  setToolTip(tip.join(QChar('\n')));
  _linkButton->setToolTip(lines.join(QChar('\n')));
  _linkButton->setVisible(!lines.isEmpty());
}

QMenu* InspectorRow::createLinkMenu(QWidget* parent)
{
  LearnMode* learn = LearnMode::current();
  if (!learn)
    return nullptr;

  QMenu* menu = new QMenu(parent);
  for (SettingControl* control : _controls) {
    LinkTarget target;
    if (!canBeLinked(control->setting()) || !learn->targetOf(control->root(), control->path(), &target))
      continue;
    // A row shared by several settings has a part of the menu for each.
    QMenu* part = menu;
    if (_controls.size() > 1)
      part = menu->addMenu(control->setting()->info().label());
    part->addAction(tr("Learn a Key"), learn, [learn, target]() { learn->startWith(target); });
    QAction* remove = part->addAction(tr("Remove the Key"), learn, [learn, target]() {
      if (learn->links())
        learn->links()->remove(target, LinkTrigger::Key);
    });
    remove->setEnabled(!learn->triggerText(target).isEmpty());
  }
  if (menu->isEmpty()) {
    delete menu;
    return nullptr;
  }
  return menu;
}

void InspectorRow::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu* menu = createLinkMenu(this);
  if (!menu) {
    QWidget::contextMenuEvent(event);
    return;
  }
  menu->setAttribute(Qt::WA_DeleteOnClose);
  menu->popup(event->globalPos());
  event->accept();
}

void InspectorRow::updateEnabled()
{
  bool any = false;
  for (SettingControl* control : _controls) {
    const bool editable = isEditable(control->setting());
    control->setEnabled(editable);
    any |= editable;
  }
  _label->setEnabled(any);
}

void InspectorRow::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton) {
    QWidget::mouseDoubleClickEvent(event);
    return;
  }
  // Back to the default value.
  bool changed = false;
  for (SettingControl* control : _controls) {
    Setting* setting = control->setting();
    if (!isEditable(setting))
      continue;
    const QVariant before = setting->value();
    setting->reset();
    control->refresh();
    changed |= setting->value() != before;
  }
  if (changed)
    emit edited();
}

// ---------------------------------------------------------------------------------------------
// InspectorGroup

InspectorGroup::InspectorGroup(const QString& key, const QString& title, bool foldedByDefault, QWidget* parent)
  : QWidget(parent),
    _key(key),
    _header(new QToolButton(this)),
    _body(new QWidget(this)),
    _bodyLayout(new QVBoxLayout(_body))
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(ROW_SPACING / 2);

  _header->setText(title);
  _header->setCheckable(true);
  _header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  _header->setAutoRaise(true);
  _header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QFont font = _header->font();
  font.setBold(true);
  _header->setFont(font);
  layout->addWidget(_header);

  _bodyLayout->setContentsMargins(GROUP_INDENT, 0, 0, 0);
  _bodyLayout->setSpacing(ROW_SPACING);
  layout->addWidget(_body);

  const bool folded = QSettings().value(preferenceKey(key), foldedByDefault).toBool();
  _header->setChecked(!folded);
  _header->setArrowType(folded ? Qt::RightArrow : Qt::DownArrow);
  _body->setVisible(!folded);
  connect(_header, &QToolButton::toggled, this, [this](bool open) { setFolded(!open); });
}

QString InspectorGroup::preferenceKey(const QString& key)
{
  return FOLDED_KEY_PREFIX + key;
}

void InspectorGroup::addWidget(QWidget* widget)
{
  _bodyLayout->addWidget(widget);
}

bool InspectorGroup::isFolded() const
{
  return !_header->isChecked();
}

void InspectorGroup::setFolded(bool folded)
{
  if (_header->isChecked() == folded)
    _header->setChecked(!folded);
  _header->setArrowType(folded ? Qt::RightArrow : Qt::DownArrow);
  _body->setVisible(!folded);
  QSettings().setValue(preferenceKey(_key), folded);
}

// ---------------------------------------------------------------------------------------------
// InspectorPage

InspectorPage::InspectorPage(SettingGroup* root, const QString& key, QWidget* parent)
  : QScrollArea(parent),
    _root(root),
    _key(key),
    _notice(nullptr),
    _content(new QWidget),
    _extras(nullptr),
    _rowsLayout(nullptr),
    _rows(nullptr),
    _refreshTimer(new QTimer(this)),
    _liveTimer(new QTimer(this)),
    _rebuildPending(false),
    _buildCount(0)
{
  setWidgetResizable(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);

  QVBoxLayout* contentLayout = new QVBoxLayout(_content);
  contentLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);
  contentLayout->setSpacing(ROW_SPACING);
  _notice = new QLabel(_content);
  _notice->setWordWrap(true);
  _notice->setVisible(false);
  contentLayout->addWidget(_notice);
  _extras = new QWidget(_content);
  QVBoxLayout* extrasLayout = new QVBoxLayout(_extras);
  extrasLayout->setContentsMargins(0, 0, 0, 0);
  extrasLayout->setSpacing(ROW_SPACING);
  _extras->setVisible(false);
  contentLayout->addWidget(_extras);
  contentLayout->addStretch(1);
  setWidget(_content);
  // Show the background of the window, set by the style sheet, rather than the window colour
  // of the system, which may be light.
  _content->setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);

  _refreshTimer->setSingleShot(true);
  _refreshTimer->setInterval(REFRESH_INTERVAL_MS);
  connect(_refreshTimer, &QTimer::timeout, this, &InspectorPage::_showPending);
  _liveTimer->setInterval(REFRESH_INTERVAL_MS);
  connect(_liveTimer, &QTimer::timeout, this, &InspectorPage::_showLiveValues);

  _build();
}

void InspectorPage::setGroupLabel(const QString& path, const QString& label)
{
  _groupLabels.insert(path, label);
  rebuild();
}

void InspectorPage::setRoot(SettingGroup* root)
{
  if (root == _root)
    return;
  _root = root;
  rebuild();
}

void InspectorPage::addExtraRow(const QString& label, QWidget* widget)
{
  QWidget* row = new QWidget(_extras);
  QHBoxLayout* layout = new QHBoxLayout(row);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(ROW_SPACING);
  layout->addWidget(new InspectorRowLabel(label, row), 0, Qt::AlignTop);
  widget->setParent(row);
  layout->addWidget(widget, 1);
  layout->addSpacing(LINK_BUTTON_ROOM);
  _extras->layout()->addWidget(row);
  _extras->setVisible(true);
  Inspector::useMapMapColors(row);
}

void InspectorPage::setNotice(const QString& text)
{
  _notice->setText(text);
  _notice->setVisible(!text.isEmpty());
}

QString InspectorPage::notice() const
{
  return _notice->text();
}

QList<SettingControl*> InspectorPage::controls() const
{
  QList<SettingControl*> all;
  for (InspectorRow* row : _rowList)
    all << row->controls();
  return all;
}

SettingControl* InspectorPage::control(const QString& path) const
{
  return _controls.value(path, nullptr);
}

InspectorRow* InspectorPage::row(const QString& path) const
{
  for (InspectorRow* row : _rowList) {
    for (SettingControl* control : row->controls()) {
      if (control->path() == path)
        return row;
    }
  }
  return nullptr;
}

void InspectorPage::setLiveValueReader(SettingControl::LiveValueReader reader)
{
  _liveValueReader = reader;
  for (SettingControl* control : controls())
    control->setLiveValueReader(reader);
}

void InspectorPage::rebuild()
{
  // A control may be sending its new value: delete it once the event is over.
  if (_rebuildPending)
    return;
  _rebuildPending = true;
  QTimer::singleShot(0, this, [this]() {
    _rebuildPending = false;
    _build();
  });
}

void InspectorPage::refresh(const QString& path)
{
  _pending.insert(path);
  // The first change is shown at once; the next ones at most every REFRESH_INTERVAL_MS.
  if (!_refreshTimer->isActive()) {
    _showPending();
    _refreshTimer->start();
  }
}

void InspectorPage::_showPending()
{
  const QSet<QString> paths = _pending;
  _pending.clear();
  for (const QString& path : paths) {
    if (SettingControl* found = _controls.value(path, nullptr)) {
      found->refresh();
      continue;
    }
    // A list: every element shown.
    const QString prefix = path + QChar('/');
    for (auto it = _controls.constBegin(); it != _controls.constEnd(); ++it) {
      if (it.key().startsWith(prefix))
        it.value()->refresh();
    }
  }
  for (InspectorRow* row : _rowList)
    row->updateEnabled();
}

void InspectorPage::_showLiveValues()
{
  for (SettingControl* control : controls()) {
    Setting* setting = control->setting();
    if (setting && setting->info().isModulable())
      control->refresh();
  }
}

void InspectorPage::_build()
{
  _buildCount++;

  // Remember which groups are open; the preferences keep it, but a group made for the first
  // time reads its default there.
  if (_rows) {
    _rows->hide();
    _rows->deleteLater();
  }
  _groups.clear();
  _rowList.clear();
  _controls.clear();

  _rows = new QWidget(_content);
  _rowsLayout = new QVBoxLayout(_rows);
  _rowsLayout->setContentsMargins(0, 0, 0, 0);
  _rowsLayout->setSpacing(ROW_SPACING);
  if (_root)
    _buildGroup(_root, _rowsLayout);

  // After the notice, before the stretch.
  static_cast<QVBoxLayout*>(_content->layout())->insertWidget(1, _rows);
  Inspector::useMapMapColors(_rows);

  bool modulable = false;
  for (SettingControl* control : controls()) {
    if (_liveValueReader)
      control->setLiveValueReader(_liveValueReader);
    if (Setting* setting = control->setting())
      modulable |= setting->info().isModulable();
  }
  if (modulable)
    _liveTimer->start();
  else
    _liveTimer->stop();
}

InspectorRow* InspectorPage::_buildRow(const QString& label, const QList<Setting*>& settings)
{
  QList<SettingControl*> made;
  const bool compact = settings.size() > 1;
  for (Setting* setting : settings) {
    SettingControl* control = SettingControl::create(_root, setting, compact);
    _controls.insert(control->path(), control);
    made << control;
  }
  InspectorRow* row = new InspectorRow(label, made);
  connect(row, &InspectorRow::edited, this, &InspectorPage::edited);
  _rowList << row;
  return row;
}

void InspectorPage::_buildGroup(SettingGroup* group, QVBoxLayout* layout)
{
  QStringList displayGroupIds;
  QHash<QString, QList<QWidget*> > displayGroupWidgets;

  // Settings sharing a row id are shown on one row, where the first of them is declared.
  QHash<QString, QList<Setting*> > sharedRows;
  const QList<SettingNode*> children = group->children();
  for (SettingNode* child : children) {
    if (child->kind() != SettingNode::SettingKind)
      continue;
    Setting* setting = static_cast<Setting*>(child);
    if (!setting->info().isHidden() && !setting->info().displayRow().isEmpty())
      sharedRows[setting->info().displayRow()] << setting;
  }
  QSet<QString> sharedRowsMade;

  for (SettingNode* child : children) {
    QList<QWidget*> made;
    QString displayGroupId;

    switch (child->kind()) {
    case SettingNode::SettingKind: {
      Setting* setting = static_cast<Setting*>(child);
      const SettingInfo& info = setting->info();
      if (info.isHidden())
        continue;
      displayGroupId = info.displayGroup();
      const QString rowId = info.displayRow();
      if (!rowId.isEmpty() && sharedRows.value(rowId).size() > 1) {
        if (sharedRowsMade.contains(rowId))
          continue;
        sharedRowsMade.insert(rowId);
        QStringList labels;
        for (Setting* shared : sharedRows.value(rowId))
          labels << shared->info().label();
        made << _buildRow(labels.join(SHARED_LABEL_SEPARATOR), sharedRows.value(rowId));
      }
      else {
        made << _buildRow(info.label(), { setting });
      }
      break;
    }
    case SettingNode::ListKind: {
      SettingList* list = static_cast<SettingList*>(child);
      const SettingInfo& info = list->elementInfo();
      if (info.isHidden())
        continue;
      for (int index = 0; index < list->count(); index++) {
        if (list->isShownInPanel(index))
          made << _buildRow(tr("%1 %2").arg(info.label()).arg(index), { list->at(index) });
      }
      displayGroupId = info.displayGroup();
      break;
    }
    case SettingNode::GroupKind: {
      SettingGroup* subgroup = static_cast<SettingGroup*>(child);
      const QString path = subgroup->path();
      // A group of the tree, such as a shape of a layer, starts folded.
      InspectorGroup* fold = new InspectorGroup(_key + QChar('/') + TREE_GROUP_KEY + path,
                                                _groupLabels.value(path, subgroup->id()), true);
      QVBoxLayout* inner = new QVBoxLayout;
      inner->setContentsMargins(0, 0, 0, 0);
      QWidget* holder = new QWidget;
      holder->setLayout(inner);
      const int rowsBefore = _rowList.size();
      _buildGroup(subgroup, inner);
      if (_rowList.size() == rowsBefore) {
        delete holder;
        delete fold;
        continue;
      }
      fold->addWidget(holder);
      _groups << fold;
      made << fold;
      break;
    }
    }

    made.removeAll(nullptr);
    if (made.isEmpty())
      continue;
    if (displayGroupId.isEmpty()) {
      for (QWidget* widget : made)
        layout->addWidget(widget);
    }
    else {
      if (!displayGroupWidgets.contains(displayGroupId))
        displayGroupIds << displayGroupId;
      displayGroupWidgets[displayGroupId] << made;
    }
  }

  // Display groups come after the other rows, by their order, then by first use.
  QList<SettingDisplayGroup> displayGroups;
  for (const QString& id : displayGroupIds)
    displayGroups << describeDisplayGroup(group, id);
  std::stable_sort(displayGroups.begin(), displayGroups.end(),
                   [](const SettingDisplayGroup& a, const SettingDisplayGroup& b) { return a.order < b.order; });

  for (const SettingDisplayGroup& displayGroup : displayGroups) {
    const QString key = _key + QChar('/') + DISPLAY_GROUP_KEY + group->path()
        + DISPLAY_GROUP_KEY_SEPARATOR + displayGroup.id;
    InspectorGroup* fold = new InspectorGroup(key, displayGroup.label, displayGroup.collapsedByDefault);
    const QList<QWidget*> members = displayGroupWidgets.value(displayGroup.id);
    for (QWidget* member : members)
      fold->addWidget(member);
    _groups << fold;
    layout->addWidget(fold);
  }
}

// ---------------------------------------------------------------------------------------------
// Inspector

void Inspector::useMapMapColors(QWidget* widget)
{
  QColor disabledWhite = MAPMAP_WHITE;
  disabledWhite.setAlpha(DISABLED_WHITE_ALPHA);
  QColor secondaryWhite = MAPMAP_WHITE;
  secondaryWhite.setAlpha(SECONDARY_WHITE_ALPHA);

  // The background of the page (Window) stays the one of the style sheet; Base is the inside
  // of the fields, switches and pads, under white text.
  QPalette colors = widget->palette();
  colors.setColor(QPalette::Base, MAPMAP_DARKER_GRAY);
  for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText,
                                   QPalette::BrightText, QPalette::Highlight})
    colors.setColor(role, MAPMAP_WHITE);
  colors.setColor(QPalette::HighlightedText, MAPMAP_DARK_GRAY);
  colors.setColor(QPalette::Button, MAPMAP_DARK_GRAY);
  colors.setColor(QPalette::Mid, MAPMAP_LIGHTER_GRAY);
  colors.setColor(QPalette::Dark, disabledWhite);
  colors.setColor(QPalette::PlaceholderText, secondaryWhite);
  for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
    colors.setColor(QPalette::Disabled, role, disabledWhite);

  // A style sheet stops a palette from passing to the children: each one gets it.
  widget->setPalette(colors);
  for (QWidget* child : widget->findChildren<QWidget*>())
    child->setPalette(colors);
}

Inspector::Inspector(QWidget* parent)
  : QWidget(parent),
    _name(new QLabel(this)),
    _kind(new QLabel(this)),
    _tabs(new QTabBar(this)),
    _pages(new QStackedWidget(this))
{
  useMapMapColors(this);
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QHBoxLayout* header = new QHBoxLayout;
  header->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN / 2);
  QFont font = _name->font();
  font.setBold(true);
  _name->setFont(font);
  _name->setTextInteractionFlags(Qt::TextSelectableByMouse);
  _kind->setForegroundRole(QPalette::PlaceholderText);
  header->addWidget(_name, 1);
  header->addWidget(_kind);
  layout->addLayout(header);

  _tabs->setExpanding(false);
  _tabs->setDrawBase(false);
  layout->addWidget(_tabs);
  layout->addWidget(_pages, 1);
  connect(_tabs, &QTabBar::currentChanged, _pages, &QStackedWidget::setCurrentIndex);
}

void Inspector::setHeader(const QString& name, const QString& kind)
{
  _name->setText(name);
  _kind->setText(kind);
}

QString Inspector::headerName() const
{
  return _name->text();
}

InspectorPage* Inspector::addPage(const QString& title, SettingGroup* root, const QString& key)
{
  InspectorPage* page = new InspectorPage(root, key, _pages);
  _pages->addWidget(page);
  _tabs->addTab(title);
  connect(page, &InspectorPage::edited, this, &Inspector::edited);
  return page;
}

void Inspector::clearPages()
{
  while (_tabs->count() > 0)
    _tabs->removeTab(0);
  while (_pages->count() > 0) {
    QWidget* page = _pages->widget(0);
    _pages->removeWidget(page);
    page->deleteLater();
  }
}

int Inspector::pageCount() const
{
  return _pages->count();
}

InspectorPage* Inspector::page(int index) const
{
  return qobject_cast<InspectorPage*>(_pages->widget(index));
}

} // namespace mmp
