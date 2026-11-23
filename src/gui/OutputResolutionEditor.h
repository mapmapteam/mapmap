/*
 * OutputResolutionEditor.h
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

#ifndef OUTPUT_RESOLUTION_EDITOR_H_
#define OUTPUT_RESOLUTION_EDITOR_H_

#include <QWidget>
#include <QSize>

class QComboBox;
class QSpinBox;

namespace mmp {

/**
 * Lets the user pick an output resolution: a usual one from a list, the
 * resolution of the output screen, or any width and height ("Custom", the
 * only choice that lets the user edit them).
 */
class OutputResolutionEditor : public QWidget
{
  Q_OBJECT

public:
  /**
   * Creates the editor. screenResolution is offered as "Output screen" when
   * valid.
   */
  explicit OutputResolutionEditor(const QSize& screenResolution, QWidget* parent = nullptr);

  /// Returns the chosen resolution.
  QSize resolution() const;

  /**
   * Shows a resolution: selects the preset of that size if there is one,
   * otherwise "Custom".
   */
  void setResolution(const QSize& resolution);

  /// Offers this resolution as "Output screen" (none if invalid).
  void setScreenResolution(const QSize& screenResolution);

  /// Returns true if "Output screen" is chosen.
  bool followsScreen() const;

  /// Chooses "Output screen", if it is offered.
  void setFollowsScreen();

private:
  // Applies the chosen entry of the list to the width and height.
  void _entryChosen(int index);

  // Selects an entry and applies it.
  void _selectEntry(int index);

  // Index of the "Output screen" entry, or -1.
  int _screenIndex() const;

  // Index of the "Custom" entry.
  int _customIndex() const;

  QComboBox* _presetBox;
  QSpinBox* _widthBox;
  QSpinBox* _heightBox;
};

}

#endif /* OUTPUT_RESOLUTION_EDITOR_H_ */
