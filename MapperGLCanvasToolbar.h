/*
 * MapperGLCanvasToolbar.h
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

#ifndef MAPPERGLCANVASTOOLBAR_H_
#define MAPPERGLCANVASTOOLBAR_H_

#include "MM.h"
#include "MapperGLCanvas.h"

MM_BEGIN_NAMESPACE

class MapperGLCanvasToolbar : public QWidget {
  Q_OBJECT
public:
  MapperGLCanvasToolbar(MapperGLCanvas* canvas, QWidget* parent=0);
  virtual ~MapperGLCanvasToolbar();

  // Create zoom tool buttons
  void createZoomToolsLayout();

  // Show/Hide zoom tool buttons
  void showZoomToolBar(bool visible);
  void enableZoomToolBar(bool enabled);

public slots:
  // Update and feedback zoom level
  void updateDropdownMenu();

protected:
  MapperGLCanvas* _canvas;

  // Buttons for toolbox layout
  QPushButton* _zoomInButton;
  QPushButton* _zoomOutButton;
  QPushButton* _resetZoomButton;
  QPushButton* _fitToViewButton;
  // Dropdown menu
  QComboBox* _dropdownMenu;

};

MM_END_NAMESPACE

#endif /* MAPPERGLCANVASTOOLBAR_H_ */
