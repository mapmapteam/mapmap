/*
 * Element.h
 *
 * (c) 2016 Sofian Audry -- info(@)sofianaudry(.)com
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

#ifndef ELEMENT_H_
#define ELEMENT_H_

#include <QtGlobal>
#include <QObject>
#include <QIcon>

#include "Serializable.h"
#include "UidAllocator.h"

#include <QEvent>

namespace mmp {

/**
 * An object of the project that the user names and adjusts: a source or a layer.
 *
 * Each element declares its settings in its constructor, in the tree returned by settings(),
 * and relays the notifications of that tree as the Qt signals settingChanged() and
 * settingsStructureChanged().
 */
class Element : public Serializable, public SettingListener
{
  Q_OBJECT

public:
  typedef QSharedPointer<Element> ptr;

  Element(uid id, UidAllocator* allocator);
  virtual ~Element();

  uid getId() const { return _id; }

  virtual void setName(const QString& name);
  virtual QString getName() const { return _name; }

  virtual float getOpacity() const { return _opacity; }
  virtual void setOpacity(float opacity);

  virtual bool isLocked() const    { return _isLocked; }
  virtual void setLocked(bool locked);
  virtual void toggleLocked()  { setLocked(!isLocked()); }

  virtual void build() {}

  virtual QIcon getIcon() const { return QIcon(); }

  void read(const QJsonObject& obj) override;
  void write(QJsonObject& obj) override;

  /// Relays a change of value of the tree as settingChanged().
  void settingValueChanged(const QString& path, const QVariant& value) override;

  /// Relays a change of the shape of the tree as settingsStructureChanged().
  void settingStructureChanged() override;

signals:
  /// Emitted when the value of the setting at path, relative to settings(), changed.
  void settingChanged(uid id, QString path, QVariant value);

  /// Emitted when a setting was added or removed, a list resized or a setting made (un)available.
  void settingsStructureChanged(uid id);

protected:
  /// Notifies the current value of the setting, or of the list, with this id: for setters
  /// that changed the value outside the tree.
  void _emitPropertyChanged(const QString& propertyName);

  /// The id is given to the constructor and written by Element::write().
  QStringList _reservedKeys() const override { return Serializable::_reservedKeys() << ProjectLabels::ID; }

private:
  uid _id;
  QString _name;
  bool _isLocked;
  float _opacity;
  UidAllocator* _allocator;
};

}

#endif /* MOBJECT_H_ */
