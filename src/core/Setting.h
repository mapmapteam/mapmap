/*
 * Setting.h
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

/**
 * The settings model: what an object of the project lets its users adjust.
 *
 * An object declares its settings in a tree. A Setting is a leaf with a
 * description (SettingInfo) and a value; a SettingGroup is a folder of named
 * children; a SettingList is a variable number of settings sharing one
 * description, numbered from 0. The path of a node, its ids joined by "/"
 * from the topmost group, is its address for project files, OSC and MCP.
 *
 * Nodes are plain C++ objects. The topmost group of a tree reports changes
 * to one SettingListener. Settings are written from the main thread only.
 */

#ifndef SETTING_H_
#define SETTING_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>

namespace mmp {

/// One entry of a Choice or Reference setting.
struct SettingOption {
  /// Stable identifier, kept as the value of the setting.
  QString id;
  /// Translated text shown to the user.
  QString label;
};

/// Description of a setting. Built with the static factories, refined with the chained setters.
class SettingInfo {
public:
  /// What kind of value a setting holds.
  enum Type { Number, Integer, Boolean, Color, Text, Choice, Point, Range, Reference };

  /// The unit of a value, which lets interfaces format it.
  enum Unit { NoUnit, Percent, Degrees, Hertz, Seconds, Pixels };

  /// A real number, kept as a double.
  static SettingInfo number(const QString& id, const QString& label);
  /// A whole number, kept as an int; written values are rounded to the step.
  static SettingInfo integer(const QString& id, const QString& label);
  /// True or false.
  static SettingInfo boolean(const QString& id, const QString& label);
  /// A QColor, alpha included.
  static SettingInfo color(const QString& id, const QString& label);
  /// A text.
  static SettingInfo text(const QString& id, const QString& label);
  /// A text naming a file; filter lists the accepted extensions, as for QFileDialog.
  static SettingInfo filePath(const QString& id, const QString& label, const QString& filter);
  /// The id of one of the options; an id missing from the options is kept.
  static SettingInfo choice(const QString& id, const QString& label, const QList<SettingOption>& options);
  /// A QPointF; the bounds apply to each coordinate.
  static SettingInfo point(const QString& id, const QString& label);
  /// A QPointF(low, high), low <= high; the bounds apply to both.
  static SettingInfo range(const QString& id, const QString& label);
  /// The id, as a text, of another element of the project; options come from a provider.
  static SettingInfo reference(const QString& id, const QString& label);

  /// Sets the lowest and highest values; an invalid QVariant leaves that side open.
  SettingInfo& bounds(const QVariant& minimum, const QVariant& maximum);
  /// Sets the increment of the interface; a strict step also rounds every written value.
  SettingInfo& step(double step, bool strict = false);
  /// Sets the value of a new setting, and the one reset() restores.
  SettingInfo& defaultValue(const QVariant& value);
  /// Sets the unit of the value.
  SettingInfo& unit(Unit unit);
  /// Sets the factor between the value and the number shown: 100 for a 0..1 value shown in %.
  SettingInfo& displayScale(double scale);
  /// Puts the setting in a display group of the panel; not part of the path.
  SettingInfo& displayGroup(const QString& groupId);
  /// Sets the type the value is stored as, e.g. QMetaType::Float for opacity.
  SettingInfo& valueType(QMetaType::Type type);
  /// Sets a function computing the options when they are asked for.
  SettingInfo& optionsProvider(std::function<QList<SettingOption>()> provider);
  /// Lets the setting be modulated; by default it is not.
  SettingInfo& modulable();
  /// Keeps the value out of project files; by default it is saved.
  SettingInfo& notStored();
  /// Refuses every write.
  SettingInfo& readOnly();
  /// Hides the setting from the panel; it can still be reached by path.
  SettingInfo& hidden();
  /// Settings sharing a row id may be shown on one line.
  SettingInfo& displayRow(const QString& rowId);
  /// Sets one translated sentence saying what the setting does; empty by default.
  SettingInfo& description(const QString& text);

  /// Returns the id: one segment of the path.
  QString id() const { return _id; }
  /// Returns the translated label.
  QString label() const { return _label; }
  /// Returns the sentence saying what the setting does, or an empty text.
  QString description() const { return _description; }
  /// Returns the kind of value.
  Type type() const { return _type; }
  /// Returns the lowest value, or an invalid QVariant when there is none.
  QVariant minimum() const { return _minimum; }
  /// Returns the highest value, or an invalid QVariant when there is none.
  QVariant maximum() const { return _maximum; }
  /// Returns the increment of the interface, 0 when there is none.
  double step() const { return _step; }
  /// Returns true when every written value is rounded to the step.
  bool isStrictStep() const { return _strictStep; }
  /// Returns the value of a new setting.
  QVariant defaultValue() const { return _defaultValue; }
  /// Returns the unit.
  Unit unit() const { return _unit; }
  /// Returns the factor between the value and the number shown.
  double displayScale() const { return _displayScale; }
  /// Returns the id of the display group, or an empty text.
  QString displayGroup() const { return _displayGroup; }
  /// Returns the id of the display row, or an empty text.
  QString displayRow() const { return _displayRow; }
  /// Returns the type the value is stored as.
  QMetaType::Type valueType() const { return _valueType; }
  /// Returns true for a text naming a file.
  bool isFilePath() const { return _filePath; }
  /// Returns the extensions a file path accepts.
  QString fileFilter() const { return _fileFilter; }
  /// Returns true when the setting may be modulated.
  bool isModulable() const { return _modulable; }
  /// Returns true when the value is saved in project files.
  bool isStored() const { return _stored; }
  /// Returns true when every write is refused.
  bool isReadOnly() const { return _readOnly; }
  /// Returns true when the panel does not show the setting.
  bool isHidden() const { return _hidden; }
  /// Returns the options: the provider's result when there is one.
  QList<SettingOption> options() const;

private:
  SettingInfo(Type type, const QString& id, const QString& label,
              QMetaType::Type valueType, const QVariant& defaultValue);

  QString _id;
  QString _label;
  QString _description;
  Type _type;
  QVariant _minimum;
  QVariant _maximum;
  double _step;
  bool _strictStep;
  QVariant _defaultValue;
  Unit _unit;
  double _displayScale;
  QString _displayGroup;
  QString _displayRow;
  QMetaType::Type _valueType;
  bool _filePath;
  QString _fileFilter;
  QList<SettingOption> _options;
  std::function<QList<SettingOption>()> _optionsProvider;
  bool _modulable;
  bool _stored;
  bool _readOnly;
  bool _hidden;
};

/// Receives the notifications of a tree. Implemented by Element.
class SettingListener {
public:
  virtual ~SettingListener() {}
  /// Called when the value of the node at path, relative to the topmost group, changed.
  virtual void settingValueChanged(const QString& path, const QVariant& value) = 0;
  /// Called when a node was added or removed, a list resized or a setting made (un)available.
  virtual void settingStructureChanged() = 0;
};

class SettingGroup;

/// Base of the three kinds of nodes.
class SettingNode {
public:
  /// The three kinds of nodes.
  enum Kind { SettingKind, GroupKind, ListKind };

  virtual ~SettingNode();

  /// Returns the kind of this node.
  virtual Kind kind() const = 0;

  /// Returns the id: the last segment of the path.
  QString id() const { return _id; }

  /// Returns the nearest group holding this node (for an element of a list, the group
  /// holding the list), or nullptr for a topmost group.
  SettingGroup* parent() const;

  /// Returns the path relative to the topmost group; "" for that group.
  QString path() const;

protected:
  /// Creates a node with an id and no parent.
  explicit SettingNode(const QString& id);

  /// Calls the listener of the topmost group with the path of this node and a value.
  void _notifyValueChanged(const QVariant& value) const;

  /// Calls the listener of the topmost group about a change of structure.
  void _notifyStructureChanged() const;

private:
  friend class SettingGroup;
  friend class SettingList;

  SettingNode(const SettingNode&) = delete;
  SettingNode& operator=(const SettingNode&) = delete;

  /// Returns the group at the top of the tree holding this node.
  const SettingGroup* _topmost() const;

  QString _id;
  SettingNode* _parentNode;
};

/// A leaf of the tree: a description and a value, kept here or in its owner's storage.
class Setting : public SettingNode {
public:
  Kind kind() const override { return SettingKind; }

  /// Returns the description.
  const SettingInfo& info() const { return _info; }

  /// Returns the base value: the one the user sets, saved in project files.
  QVariant value() const;

  /// Returns the value used for rendering; equals value() in this slice.
  QVariant effectiveValue() const;

  /// Sanitizes, writes and notifies; returns false when refused. Writing the current value
  /// returns true and notifies nothing.
  bool setValue(const QVariant& value);

  /// Returns the value converted to the value type and kept within the bounds; sets ok to
  /// false, and returns an invalid QVariant, when the value cannot be converted or is not finite.
  /// A Boolean accepts true or false, the numbers 0 and 1, and the texts "true", "false", "1"
  /// and "0" in any case; nothing else.
  QVariant sanitized(const QVariant& value, bool* ok = nullptr) const;

  /// Sets the value back to the default value.
  void reset();

  /// Returns true when the value can be read and written as a position between 0 and 1:
  /// for a Number or an Integer with both bounds, minimum < maximum; a Boolean; a Choice
  /// with options.
  bool hasNormalized() const;

  /// Returns the position of the value between 0 and 1: (value - minimum) / (maximum - minimum)
  /// for a number, 0 or 1 for a Boolean, index / (count - 1) for a Choice. Returns 0 when
  /// hasNormalized() is false or a Choice holds an id missing from its options.
  double normalized() const;

  /// Writes the value at a position, kept between 0 and 1. A Boolean becomes true from
  /// position 0.5; a Choice splits the positions into equal steps, one per option, in
  /// their order. Returns false, with a warning, when hasNormalized() is false.
  bool setNormalized(double position);

  /// Returns the value as shown to the user: value() * displayScale for a Number or an
  /// Integer, value() otherwise.
  QVariant displayValue() const;

  /// Writes a value given as shown to the user; the inverse of displayValue().
  bool setDisplayValue(const QVariant& value);

  /// Returns the text written in project files: the value converted to the value type, then
  /// to a QString, as the project writer does for a Qt property. A Point or a Range has no
  /// text: the result is empty.
  QString toText() const;

  /// Writes a value read from a project file; the inverse of toText().
  bool setFromText(const QString& text);

  /// Writes a value received over OSC or MCP. A Point or a Range takes two numbers; every
  /// other type takes one argument, as accepted by sanitized(): a colour may be a text such as
  /// "#ff0000" or "red", a Boolean 0 or 1. Returns false, with a warning, when the number of
  /// arguments is wrong or the value is refused.
  bool setFromArguments(const QVariantList& arguments);

  /// Returns false when the owner made the setting meaningless for now.
  bool isAvailable() const { return _available; }

  /// Makes the setting available or not; notifies a structure change.
  void setAvailable(bool available);

  /// Keeps the value in the owner's storage: read returns it, write stores a sanitized
  /// value and returns false to refuse it. Two null functions keep the value here again;
  /// one null function without the other is refused, with a warning.
  void bind(std::function<QVariant()> read, std::function<bool(const QVariant&)> write);

  /// Notifies the current value; for owners whose storage changed outside setValue().
  void notifyChanged();

private:
  friend class SettingGroup;
  friend class SettingList;

  /// Creates a setting with the id of its description.
  explicit Setting(const SettingInfo& info);

  /// Creates a setting with its own id, for the elements of a list.
  Setting(const QString& id, const SettingInfo& info);

  /// Sanitizes one number: finite, rounded to the step when needed, within the bounds.
  bool _sanitizeNumber(double* number) const;

  /// Returns true for a Number or an Integer.
  bool _isNumber() const;

  /// Reads the bounds of a number; returns false unless both are finite and minimum < maximum.
  bool _numberBounds(double* minimum, double* maximum) const;

  SettingInfo _info;
  QVariant _value;
  std::function<QVariant()> _read;
  std::function<bool(const QVariant&)> _write;
  bool _available;
  /// Set when notifyChanged() runs, so that setValue() does not notify twice when the
  /// owner's write function notifies by itself.
  bool _notified;
};

/// A variable number of settings sharing one description, kept in the owner's storage.
class SettingList : public SettingNode {
public:
  ~SettingList() override;

  Kind kind() const override { return ListKind; }

  /// Returns the description shared by the elements.
  const SettingInfo& elementInfo() const { return _elementInfo; }

  /// Returns the number of elements.
  int count() const;

  /// Returns the element at index; nullptr when out of range.
  Setting* at(int index);

  /// Call after the element count changed; notifies a structure change when it did.
  void sync();

  /// One notification for the whole list, value = QVariantList.
  void notifyChanged();

private:
  friend class SettingGroup;

  SettingList(const QString& id, const SettingInfo& elementInfo,
              std::function<int()> count,
              std::function<QVariant(int)> read,
              std::function<bool(int, const QVariant&)> write);

  /// Returns the element at index; nullptr when out of range.
  Setting* _element(int index) const;

  SettingInfo _elementInfo;
  std::function<int()> _count;
  std::function<QVariant(int)> _read;
  std::function<bool(int, const QVariant&)> _write;
  std::vector<std::unique_ptr<Setting>> _elements;
};

/// How the panel shows a display group.
struct SettingDisplayGroup {
  /// The id given to SettingInfo::displayGroup().
  QString id;
  /// Translated title.
  QString label;
  /// Position among the display groups, smallest first.
  int order;
  /// True when the group starts folded.
  bool collapsedByDefault;
};

/// A folder of named children: settings, groups and lists. Owns what it adds, not what it mounts.
class SettingGroup : public SettingNode {
public:
  /// Creates an empty group, the top of a tree until it is mounted.
  SettingGroup();

  /// Destroys the children this group owns and lets go of the groups mounted on it.
  /// A group destroyed while it is mounted leaves the group it is mounted on silently,
  /// without a structure notification: its owner should unmount it first.
  ~SettingGroup() override;

  Kind kind() const override { return GroupKind; }

  /// Adds a setting kept by this group; nullptr, with a warning, when the id is taken or invalid.
  Setting* addSetting(const SettingInfo& info);

  /// Adds a group; nullptr, with a warning, when the id is taken or invalid.
  SettingGroup* addGroup(const QString& id);

  /// Adds a list whose elements are read and written through the owner's storage;
  /// nullptr, with a warning, when the id is taken or invalid or a function is null.
  SettingList* addList(const QString& id, const SettingInfo& elementInfo,
                       std::function<int()> count,
                       std::function<QVariant(int)> read,
                       std::function<bool(int, const QVariant&)> write);

  /// Shows a group under id without owning it; refused, with a warning, when the id is
  /// taken or the group is already in a tree. The owner of the mounted group unmounts it
  /// before destroying it; destroyed while mounted, it leaves silently (see ~SettingGroup).
  void mount(const QString& id, SettingGroup* group);

  /// Takes back a mounted group, which is not destroyed.
  void unmount(const QString& id);

  /// Removes a child: destroys it when owned, unmounts it otherwise. Returns false when missing.
  bool remove(const QString& id);

  /// Returns the children in declaration order.
  QList<SettingNode*> children() const;

  /// Returns the node at path, relative to this group; nullptr when there is none.
  SettingNode* findNode(const QString& path) const;

  /// Returns the setting at path, relative to this group; nullptr unless a Setting.
  Setting* find(const QString& path) const;

  /// Describes how the panel shows a display group; replaces an earlier description.
  void describeDisplayGroup(const QString& groupId, const QString& label, int order, bool collapsedByDefault);

  /// Returns the display groups described on this group.
  QList<SettingDisplayGroup> displayGroups() const { return _displayGroups; }

  /// Sets the object notified of changes; only the listener of the topmost group is called.
  void setListener(SettingListener* listener) { _listener = listener; }

  /// Sets a function called with each setting that addSetting() adds to this group, once it
  /// is in the tree, so that the owner can give it a value it kept for it (a project file
  /// read before the setting existed). Only this group's function is called; a null
  /// function, the default, calls nothing.
  void setSettingAddedHandler(std::function<void(Setting*)> handler) { _settingAddedHandler = std::move(handler); }

private:
  friend class SettingNode;

  /// A child and whether this group destroys it.
  struct Child {
    SettingNode* node;
    bool owned;
  };

  /// Creates an empty group with an id.
  explicit SettingGroup(const QString& id);

  /// Returns true when id can name a new child; warns otherwise.
  bool _acceptsId(const QString& id) const;

  /// Returns the index of the child named id, or -1.
  int _indexOf(const QString& id) const;

  /// Appends a child and notifies a structure change.
  void _append(SettingNode* node, bool owned);

  /// Removes the child at index, destroying it when owned.
  void _removeAt(int index);

  QList<Child> _children;
  QList<SettingDisplayGroup> _displayGroups;
  SettingListener* _listener;
  std::function<void(Setting*)> _settingAddedHandler;
};

} // namespace mmp

#endif /* SETTING_H_ */
