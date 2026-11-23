/*
 * OutputResolutionEditor.cpp
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

#include "OutputResolutionEditor.h"

#include "MappingManager.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

namespace mmp {

namespace {

// What an entry of the list is, stored in its Qt::UserRole + 1 data.
enum EntryKind { PresetEntry, ScreenEntry, CustomEntry };
const int KIND_ROLE = Qt::UserRole + 1;

}

OutputResolutionEditor::OutputResolutionEditor(const QSize& screenResolution, QWidget* parent)
  : QWidget(parent)
{
  // Common presets, then the others, then "Custom". Each preset holds its
  // size in its data.
  _presetBox = new QComboBox;
  bool afterCommon = false;
  for (const MappingManager::OutputResolutionPreset& preset : MappingManager::outputResolutionPresets())
  {
    if (!preset.common && !afterCommon)
    {
      _presetBox->insertSeparator(_presetBox->count());
      afterCommon = true;
    }
    _presetBox->addItem(tr("%1 × %2 (%3)").arg(preset.size.width()).arg(preset.size.height()).arg(preset.name),
                        preset.size);
    _presetBox->setItemData(_presetBox->count() - 1, PresetEntry, KIND_ROLE);
  }
  _presetBox->insertSeparator(_presetBox->count());
  _presetBox->addItem(tr("Custom"));
  _presetBox->setItemData(_presetBox->count() - 1, CustomEntry, KIND_ROLE);

  _widthBox = new QSpinBox;
  _heightBox = new QSpinBox;
  for (QSpinBox* box : { _widthBox, _heightBox })
  {
    box->setRange(1, MM::OUTPUT_RESOLUTION_MAX);
    box->setSuffix(tr(" px"));
  }

  QHBoxLayout* layout = new QHBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_presetBox, 1);
  layout->addWidget(_widthBox);
  layout->addWidget(new QLabel(tr("×")));
  layout->addWidget(_heightBox);
  setLayout(layout);

  connect(_presetBox, &QComboBox::activated, this, &OutputResolutionEditor::_entryChosen);

  setScreenResolution(screenResolution);
  setResolution(QSize(MM::DEFAULT_OUTPUT_WIDTH, MM::DEFAULT_OUTPUT_HEIGHT));
}

QSize OutputResolutionEditor::resolution() const
{
  return QSize(_widthBox->value(), _heightBox->value());
}

void OutputResolutionEditor::setResolution(const QSize& resolution)
{
  if (!MappingManager::isValidOutputResolution(resolution))
    return;

  // A preset of that size if there is one (rather than "Output screen", which
  // can change), otherwise "Custom" with that size.
  for (int i = 0; i < _presetBox->count(); i++)
  {
    if (_presetBox->itemData(i, KIND_ROLE).toInt() == PresetEntry &&
        _presetBox->itemData(i).toSize() == resolution)
    {
      _selectEntry(i);
      return;
    }
  }
  _widthBox->setValue(resolution.width());
  _heightBox->setValue(resolution.height());
  _selectEntry(_customIndex());
}

void OutputResolutionEditor::setScreenResolution(const QSize& screenResolution)
{
  const bool wasFollowingScreen = followsScreen();
  const QSize current = resolution();

  const int index = _screenIndex();
  if (index >= 0)
  {
    _presetBox->removeItem(index + 1); // Its separator.
    _presetBox->removeItem(index);
  }

  if (MappingManager::isValidOutputResolution(screenResolution))
  {
    _presetBox->insertItem(0, tr("Output screen (%1 × %2)").arg(screenResolution.width()).arg(screenResolution.height()),
                           screenResolution);
    _presetBox->setItemData(0, ScreenEntry, KIND_ROLE);
    _presetBox->insertSeparator(1);
  }

  if (wasFollowingScreen && _screenIndex() >= 0)
    setFollowsScreen();
  else
    setResolution(current);
}

bool OutputResolutionEditor::followsScreen() const
{
  return _presetBox->currentIndex() >= 0 && _presetBox->currentIndex() == _screenIndex();
}

void OutputResolutionEditor::setFollowsScreen()
{
  if (_screenIndex() >= 0)
    _selectEntry(_screenIndex());
}

void OutputResolutionEditor::_entryChosen(int index)
{
  _selectEntry(index);
  // Ready to type the custom size.
  if (index == _customIndex())
    _widthBox->setFocus();
}

void OutputResolutionEditor::_selectEntry(int index)
{
  _presetBox->setCurrentIndex(index);

  // Width and height are only editable for "Custom", which keeps the size
  // shown so far as a starting point.
  const bool custom = (index == _customIndex());
  if (!custom)
  {
    const QSize size = _presetBox->itemData(index).toSize();
    _widthBox->setValue(size.width());
    _heightBox->setValue(size.height());
  }
  _widthBox->setEnabled(custom);
  _heightBox->setEnabled(custom);
}

int OutputResolutionEditor::_screenIndex() const
{
  return _presetBox->findData(ScreenEntry, KIND_ROLE);
}

int OutputResolutionEditor::_customIndex() const
{
  return _presetBox->findData(CustomEntry, KIND_ROLE);
}

}
