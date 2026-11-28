/*
 * Link.h
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

/**
 * Links: what a trigger, such as a key of the computer keyboard, does to a setting.
 *
 * A link joins a trigger to a target, the setting of a source or of a layer, and says what
 * the trigger does to it (its action). The links of a project are kept in a LinkTable, saved
 * with the project. A trigger drives one setting only, and a setting has at most one trigger
 * of each kind for each action: one key that toggles it, one that turns it on, one that turns
 * it off. The kind of a trigger lets other devices (MIDI, game controllers) be added without
 * changing the table.
 */

#ifndef LINK_H_
#define LINK_H_

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "UidAllocator.h"

namespace mmp {

class Setting;

/// What sets a link off.
struct LinkTrigger {
  /// The devices a trigger comes from.
  enum Kind {
    /// A key of the computer keyboard, with its modifiers.
    Key
  };

  /// The device.
  Kind kind = Key;

  /// For a key: its name and modifiers in the portable text of QKeySequence, such as "M"
  /// or "Shift+M", which does not depend on the language of the system.
  QString key;

  /// Returns a trigger for a key, given in the portable text of QKeySequence.
  static LinkTrigger forKey(const QString& key);

  /// Returns true when the trigger names something: a key with a name.
  bool isValid() const;

  /// Returns the trigger as shown to the user, such as "M" or "Shift+M" (translated keys).
  QString text() const;

  /// Returns the translated name of the kind, such as "Keyboard".
  QString kindText() const;

  /// Returns the trigger as written in project files.
  QJsonObject toJson() const;

  /// Reads a trigger written by toJson(); returns an invalid trigger when it cannot.
  static LinkTrigger fromJson(const QJsonObject& object);

  bool operator==(const LinkTrigger& other) const;
  bool operator!=(const LinkTrigger& other) const { return !(*this == other); }
};

/// The setting a link drives.
struct LinkTarget {
  /// The kinds of elements whose settings can be linked.
  enum ElementKind { SourceElement, LayerElement };

  /// The kind of the element.
  ElementKind element = LayerElement;

  /// The id of the element.
  uid id = NULL_UID;

  /// The path of the setting in the tree of the element, such as "visible" or "opacity".
  QString path;

  /// Returns a target.
  static LinkTarget of(ElementKind element, uid id, const QString& path);

  /// Returns true when the target names an element and a setting.
  bool isValid() const;

  /// Returns the target as written in project files.
  QJsonObject toJson() const;

  /// Reads a target written by toJson(); returns an invalid target when it cannot.
  static LinkTarget fromJson(const QJsonObject& object);

  /// Returns the target as text for warnings, such as "layer 3, visible".
  QString describe() const;

  bool operator==(const LinkTarget& other) const;
  bool operator!=(const LinkTarget& other) const { return !(*this == other); }
};

/// What a trigger does to its setting.
enum class LinkAction {
  /// Switches a Boolean, goes to the next option of a Choice, sends a number from one end of
  /// its range to the other (to its maximum from below the middle, else to its minimum).
  Toggle,
  /// Turns a Boolean on, or sends a number or a Choice to its maximum (its last option).
  SwitchOn,
  /// Turns a Boolean off, or sends a number or a Choice to its minimum (its first option).
  SwitchOff
};

/// Returns the action as written in project files: "toggle", "on" or "off".
QString linkActionName(LinkAction action);

/// Reads an action written by linkActionName(); returns false when the name is unknown.
bool linkActionFromName(const QString& name, LinkAction* action);

/// Returns the translated name of an action for a setting of the type of setting, such as
/// "Toggle", "Turn on" or "Set to maximum".
QString linkActionText(LinkAction action, const Setting* setting);

/// Returns true when a trigger can drive the setting: it has a position between 0 and 1
/// (see Setting::hasNormalized()) and accepts writes.
bool canBeLinked(const Setting* setting);

/// Applies an action to a setting. Returns false, writing nothing, when the setting cannot
/// be linked.
bool applyLinkAction(Setting* setting, LinkAction action);

/// A trigger, the setting it drives, and what it does to it.
struct Link {
  LinkTrigger trigger;
  LinkTarget target;
  LinkAction action = LinkAction::Toggle;

  /// Returns the link as written in project files.
  QJsonObject toJson() const;

  /// Reads a link written by toJson(); returns false, with a reason, when it cannot.
  static bool fromJson(const QJsonObject& object, Link* link, QString* reason);
};

/**
 * The links of a project.
 *
 * A trigger drives one setting only, and a setting has at most one trigger of each kind for
 * each action: adding a link removes the links it would contradict. Links are not checked against the
 * project: a link whose element does not exist, or no longer exists, is kept and does
 * nothing until the element comes back (an undone deletion).
 */
class LinkTable : public QObject
{
  Q_OBJECT

public:
  /// Creates an empty table.
  explicit LinkTable(QObject* parent = nullptr);

  /// Returns the links, in the order they were added.
  const QList<Link>& links() const { return _links; }

  /// Returns the number of links.
  int count() const { return _links.size(); }

  /// Returns true when there is no link.
  bool isEmpty() const { return _links.isEmpty(); }

  /// Adds a link, first removing the link of the same trigger and the link of the same kind
  /// of trigger to the same target with the same action. Refused, returning false, when the
  /// trigger or the target is invalid.
  bool add(const Link& link);

  /// Removes the link at index; returns false when out of range.
  bool removeAt(int index);

  /// Removes the links of a kind of trigger to a target; returns false when there is none.
  bool remove(const LinkTarget& target, LinkTrigger::Kind kind);

  /// Changes the action of the link at index, removing the link of the same kind of trigger
  /// to the same target that already had this action; returns false when out of range.
  bool setAction(int index, LinkAction action);

  /// Removes every link.
  void clear();

  /// Returns the index of the link of a trigger, or -1.
  int indexOf(const LinkTrigger& trigger) const;

  /// Returns the index of the first link of a kind of trigger to a target, or -1.
  int indexOf(const LinkTarget& target, LinkTrigger::Kind kind) const;

  /// Returns the indexes of the links to a target, of any kind.
  QList<int> indexesOf(const LinkTarget& target) const;

  /// Returns the links as written in project files.
  QJsonArray toJson() const;

  /// Replaces the links by those of a project file. A link that cannot be read is skipped,
  /// and its reason, with its position, is appended to skipped.
  void fromJson(const QJsonArray& array, QStringList* skipped);

signals:
  /// A link was added, removed or changed.
  void changed();

private:
  /// Returns true when two links cannot be in the table together.
  static bool _contradicts(const Link& one, const Link& other);

  /// Removes the links that link contradicts, except the one at index kept.
  void _removeContradicted(const Link& link, int kept = -1);

  QList<Link> _links;
};

} // namespace mmp

#endif /* LINK_H_ */
