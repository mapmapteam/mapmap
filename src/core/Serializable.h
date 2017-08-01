/*
 * Serializable.h
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

#ifndef SERIALIZABLE_H_
#define SERIALIZABLE_H_

#include <QObject>
#include <QDomDocument>
#include <QMetaProperty>
#include <QDebug>

#include "ProjectLabels.h"

namespace mmp {

class Serializable : public QObject {
  Q_OBJECT

protected:
  Serializable() {}

public:
  virtual ~Serializable() {}

  /// Returns the "cleaned" classname ie. without the namespace prefix.
  QString cleanClassName() const;

  /*
   * Used for file loading/saving. File save uses a "clean" version of the classname that
   * removes the namespace prefix (mmp::) to avoid useless repetitions in the saved file.
   */
  static QString classNameRealToClean(const QString& realClassName);
  static QString classNameCleanToReal(const QString& cleanClassName);

  virtual void read(const QDomElement& obj);
  virtual void write(QDomElement& obj);

protected:
  // Lists QProperties that should be represented as XML attributes, not as childen nodes.
  virtual QList<QString> _propertiesAttributes() const { return QList<QString>(); }

  // Lists QProperties that should NOT be parsed automatically.
  virtual QList<QString> _propertiesSpecial() const { return QList<QString>(); }

  void _writeNode(QDomElement& obj, const QString& nodeName, const QString& nodeValue);
};

}

#endif /* SERIALIZABLE_H_ */
