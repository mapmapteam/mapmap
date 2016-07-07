/*
 * MapperGLCanvasToolbar.cpp
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

#include "MapperGLCanvasToolbar.h"

MM_BEGIN_NAMESPACE

MapperGLCanvasToolbar::MapperGLCanvasToolbar(MapperGLCanvas* canvas, QWidget* parent)
  : QWidget(parent),
    _canvas(canvas)
{

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  createZoomToolsLayout();

  // Disable zoom tool buttons
  enableZoomToolBar(false);

}

MapperGLCanvasToolbar::~MapperGLCanvasToolbar() {
}

void MapperGLCanvasToolbar::createZoomToolsLayout()
{
  // Create zoom tool bar
  setObjectName("zoom-toolbox");

  // Create vertical layout for widgets
  QHBoxLayout* buttonsLayout = new QHBoxLayout;
  buttonsLayout->setContentsMargins(0, 0, 5, 0);
  // Create buttons
  // Zoom In button
  _zoomInButton = new QPushButton;
  _zoomInButton->setIcon(QIcon(":/zoom-in"));
  _zoomInButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _zoomInButton->setToolTip(tr("Enlarge the shape"));
  _zoomInButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _zoomInButton->setObjectName("zoom-in");
  connect(_zoomInButton, SIGNAL(clicked()), _canvas, SLOT(increaseZoomLevel()));
  // Zoom Out button
  _zoomOutButton = new QPushButton;
  _zoomOutButton->setIcon(QIcon(":/zoom-out"));
  _zoomOutButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _zoomOutButton->setToolTip(tr("Shrink the shape"));
  _zoomOutButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _zoomOutButton->setObjectName("zoom-out");
  connect(_zoomOutButton, SIGNAL(clicked()), _canvas, SLOT(decreaseZoomLevel()));
  // Reset to normal size button.
  _resetZoomButton = new QPushButton;
  _resetZoomButton->setIcon(QIcon(":/reset-zoom"));
  _resetZoomButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _resetZoomButton->setToolTip(tr("Reset the shape to the normal size"));
  _resetZoomButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _resetZoomButton->setObjectName("reset-zoom");
  connect(_resetZoomButton, SIGNAL(clicked()), _canvas, SLOT(resetZoomLevel()));
  // Fit to view button
  _fitToViewButton = new QPushButton;
  _fitToViewButton->setIcon(QIcon(":/zoom-fit"));
  _fitToViewButton->setIconSize(QSize(MM::ZOOM_TOOLBAR_ICON_SIZE, MM::ZOOM_TOOLBAR_ICON_SIZE));
  _fitToViewButton->setToolTip(tr("Fit the shape to content view"));
  _fitToViewButton->setFixedSize(MM::ZOOM_TOOLBAR_BUTTON_SIZE, MM::ZOOM_TOOLBAR_BUTTON_SIZE);
  _fitToViewButton->setObjectName("zoom-fit");
  connect(_fitToViewButton, SIGNAL(clicked()), _canvas, SLOT(fitShapeInView()));

  // Create separator
  QFrame *separator = new QFrame(this);
  separator->setFixedSize(5, 30);
  separator->setFrameShape(QFrame::VLine);

  // Create the dropdowm menu
  _dropdownMenu = new QComboBox;
  _dropdownMenu->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  // make some settings
  _dropdownMenu->setObjectName("dropdown-menu");
  // Create if empty or update list
  updateDropdownMenu();
  // And listen
  connect(_dropdownMenu, SIGNAL(activated(QString)), _canvas, SLOT(setZoomFromMenu(QString)));

  // Add widgets into layout
  buttonsLayout->addWidget(_zoomInButton);
  buttonsLayout->addWidget(_zoomOutButton);
  buttonsLayout->addWidget(_resetZoomButton);
  buttonsLayout->addWidget(_fitToViewButton);
  buttonsLayout->addWidget(separator);
  buttonsLayout->addWidget(_dropdownMenu);

  // Insert layout in widget
  setLayout(buttonsLayout);

  connect(_canvas, SIGNAL(zoomFactorChanged(qreal)), this, SLOT(updateDropdownMenu(qreal)));
}

void MapperGLCanvasToolbar::showZoomToolBar(bool visible)
{
  if (visible)
    show();
  else
    hide();
}

void MapperGLCanvasToolbar::enableZoomToolBar(bool enabled)
{
  foreach (QPushButton *button, findChildren<QPushButton *>()) {
    button->setEnabled(enabled);
  }
  _dropdownMenu->setEnabled(enabled);
  // Notify changes
  _areEnable = enabled;
}

void MapperGLCanvasToolbar::updateDropdownMenu(qreal factor)
{
  // Get current zoom factor percentage
  QString zoomFactor = QString::number(int(factor * 100)).append(QChar('%'));
  //Create list
  QStringList zoomFactorList;
  zoomFactorList << "400%" << "300%" << "200%" << "150%" << "125%" <<
                    "100%" << "75%" << "50%" << "25%" << "12.5%";
  // Avoid duplicate
  if (!zoomFactorList.contains(zoomFactor))
    zoomFactorList.append(zoomFactor);
  // Clear if is not empty
  _dropdownMenu->clear();
  // Add list item
  _dropdownMenu->addItems(zoomFactorList);
  // Select 100% by default
  _dropdownMenu->setCurrentText(zoomFactor);
}

MM_END_NAMESPACE
