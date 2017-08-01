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
Q_DECLARE_METATYPE(uid)

namespace mmp {

class Element : public Serializable
{
  Q_OBJECT

  Q_PROPERTY(uid     id      READ getId)
  Q_PROPERTY(QString name    READ getName    WRITE setName)
  Q_PROPERTY(bool    locked  READ isLocked   WRITE setLocked)
  Q_PROPERTY(float   opacity READ getOpacity WRITE setOpacity NOTIFY propertyChanged)
  Q_PROPERTY(QIcon   icon    READ getIcon)

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

  virtual void read(const QDomElement& obj);
  virtual void write(QDomElement& obj);

signals:
  void propertyChanged(uid id, QString propertyName, QVariant value);

protected:
  virtual QList<QString> _propertiesAttributes() const
  { return Serializable::_propertiesAttributes() << "name" << "locked";  }

  void _emitPropertyChanged(const QString& propertyName);

private:
  uid _id;
  QString _name;
  bool _isLocked;
  float _opacity;
  UidAllocator* _allocator;
};

}

#endif /* MOBJECT_H_ */
