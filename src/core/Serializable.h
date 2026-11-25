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

#include <memory>

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QDebug>

#include "ProjectLabels.h"
#include "Setting.h"

namespace mmp {

class Serializable : public QObject {
  Q_OBJECT

protected:
  /// Creates an object without a tree of settings: settings() builds it when first called.
  Serializable();

public:
  virtual ~Serializable() {}

  /**
   * Returns the root of this object's tree of settings, owned by this object.
   *
   * The tree is built on the first call: _declareSettings() runs then. An object that
   * nobody asks for its settings, as the quads a mesh builds to be drawn, never pays for
   * them. Called from a constructor, it builds the tree with the declarations of the
   * classes constructed so far: elements declare their settings in their constructors,
   * after this first call.
   */
  SettingGroup* settings() const;

  /// Returns true once settings() has built the tree of settings.
  bool hasSettingsTree() const { return _settings != nullptr; }

  /// Returns the "cleaned" classname ie. without the namespace prefix.
  QString cleanClassName() const;

  /*
   * Used for file loading/saving. File save uses a "clean" version of the classname that
   * removes the namespace prefix (mmp::) to avoid useless repetitions in the saved file.
   */
  static QString classNameRealToClean(const QString& realClassName);
  static QString classNameCleanToReal(const QString& cleanClassName);

  /**
   * Reads the settings found in obj, among the stored and writable settings that are direct
   * children of settings(), in their declaration order. A text is read by
   * Setting::setFromText(), a JSON number or boolean by Setting::setFromArguments(); any other
   * value, and a text that is not a value of the setting, is skipped with a warning.
   *
   * The keys that are neither a setting, nor the class name, nor in _reservedKeys() are kept
   * pending, with their JSON value as it is: write() writes them back, and a setting added
   * later to settings() takes the value of the pending key with its id. Each call replaces
   * the pending keys of the previous one. Subclasses read their reserved keys themselves.
   */
  virtual void read(const QJsonObject& obj);

  /**
   * Writes the pending keys, then the class name, then each stored setting that is a direct
   * child of settings(), as Setting::toText(). A stored Point or Range setting has no text
   * form: it is not written, with a warning. Subclasses write their reserved keys themselves.
   */
  virtual void write(QJsonObject& obj);

protected:
  /// JSON keys read or written by a subclass itself: they are never kept as pending.
  virtual QStringList _reservedKeys() const;

  /**
   * Declares the settings of this class in root; called once, by the first call of
   * settings(). An override calls the one of its base class first, so that settings are
   * declared base class first. Does nothing by default.
   */
  virtual void _declareSettings(SettingGroup* root);

private:
  /// Gives a setting the JSON value of its key; skips, with a warning, a value it cannot read.
  void _readSetting(Setting* setting, const QJsonValue& value) const;

  /// Gives a setting just added to the root the value of the pending key with its id, if any.
  void _takePendingKey(Setting* setting);

  /// Root of the tree of settings, built by the first call of settings().
  mutable std::unique_ptr<SettingGroup> _settings;

  /// Keys of the last object read that no setting and no subclass claimed, with their values.
  QJsonObject _pendingKeys;
};

}

#endif /* SERIALIZABLE_H_ */
