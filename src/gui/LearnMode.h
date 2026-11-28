/*
 * LearnMode.h
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

#ifndef LEARN_MODE_H_
#define LEARN_MODE_H_

#include <functional>

#include <QObject>
#include <QPointer>

#include "Link.h"

namespace mmp {

class SettingGroup;

/**
 * The state of learning, shared by the widgets that show the controls one can link: whether
 * MapMap is learning, and which setting was chosen to receive the next trigger.
 *
 * The main window creates one and makes it current; the rows of the inspector and the list of
 * layers ask the current one, if any, what to show. It also tells them the links of the
 * project, so that a control shows the key it is linked to.
 */
class LearnMode : public QObject
{
  Q_OBJECT

public:
  /// Finds the element whose tree of settings has root as its topmost group.
  typedef std::function<bool(const SettingGroup* root, LinkTarget::ElementKind* element, uid* id)> ElementFinder;

  /// Creates a learn mode, not learning.
  explicit LearnMode(QObject* parent = nullptr);

  /// Destroys the learn mode; it stops being current.
  ~LearnMode() override;

  /// Returns the current learn mode, or nullptr when there is none (as in tests).
  static LearnMode* current();

  /// Makes this learn mode the current one.
  void makeCurrent();

  /// Sets the links shown by the controls.
  void setLinks(LinkTable* links);

  /// Returns the links shown by the controls, or nullptr.
  LinkTable* links() const { return _links; }

  /// Sets the function finding the element of a tree of settings.
  void setElementFinder(ElementFinder finder) { _finder = finder; }

  /// Returns the target of the setting at path in the tree whose topmost group is root;
  /// false when no element has this tree.
  bool targetOf(const SettingGroup* root, const QString& path, LinkTarget* target) const;

  /// Returns true while learning.
  bool isActive() const { return _active; }

  /// Returns true when a setting was chosen to receive the next trigger.
  bool hasTarget() const { return _target.isValid(); }

  /// Returns the setting chosen to receive the next trigger.
  LinkTarget target() const { return _target; }

  /// Returns true when target is the setting chosen.
  bool isChosen(const LinkTarget& target) const { return hasTarget() && _target == target; }

  /// Returns the trigger of a kind linked to a target, as shown to the user, or an empty text.
  QString triggerText(const LinkTarget& target, LinkTrigger::Kind kind = LinkTrigger::Key) const;

  /// Returns the lines naming the triggers linked to a target, such as "Keyboard: M", for
  /// tooltips; an empty list when there is none.
  QStringList triggerLines(const LinkTarget& target) const;

public slots:
  /// Starts learning, with no setting chosen.
  void start();

  /// Starts learning, with a setting already chosen.
  void startWith(const LinkTarget& target);

  /// Stops learning.
  void stop();

  /// Chooses the setting that receives the next trigger; starts learning when needed.
  void choose(const LinkTarget& target);

  /// Forgets the setting chosen, and keeps learning.
  void clearTarget();

signals:
  /// Learning started or stopped, or another setting was chosen.
  void changed();

  /// A link was added, removed or changed.
  void linksChanged();

private:
  bool _active;
  LinkTarget _target;
  QPointer<LinkTable> _links;
  QMetaObject::Connection _linksConnection;
  ElementFinder _finder;
};

} // namespace mmp

#endif /* LEARN_MODE_H_ */
