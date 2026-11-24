/*
 * OutputGLCanvas.h
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2014 Dame Diongue -- baydamd(@)gmail(.)com
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

#ifndef OUTPUTGLCANVAS_H_
#define OUTPUTGLCANVAS_H_

#include "MapperGLCanvas.h"

#include <QTimer>

#ifdef HAVE_SYPHON
#include "SyphonOutput.h"
#endif

namespace mmp {

class OutputGLCanvas: public MapperGLCanvas
{
  Q_OBJECT

public:
  OutputGLCanvas(MainWindow* mainWindow, QWidget* parent = 0, QOpenGLWidget* shareWidget = 0, QGraphicsScene* scene = 0);
  virtual ~OutputGLCanvas() {}

  /**
   * Sets the output frame: the part of the scene that this canvas shows,
   * scaled to fill the window while keeping its proportions (see
   * MappingManager::getOutputResolution()).
   */
  void setOutputFrame(const QRectF& frame);

  /// Returns the output frame.
  QRectF getOutputFrame() const { return _outputFrame; }

  /// Returns the scale from output coordinates to window pixels.
  qreal getZoomFactor() const override { return transform().m11(); }

  // Draws foreground (displays crosshair if needed).
  void drawForeground(QPainter *painter , const QRectF &rect) override;

public:
  void setDisplayCrosshair(bool displayCrosshair) {
    _displayCrosshair = displayCrosshair;
  }

  void setDisplayTestSignal(bool displayTestSignal) {
    _displayTestSignal = displayTestSignal;
  }

  // Syphon output (macOS): publish the rendered output as a Syphon server.
  // No-ops on platforms without Syphon support.
  void setSyphonOutputEnabled(bool on);
  bool isSyphonOutputEnabled() const;
  void setSyphonServerName(const QString& name);

private:
  void _drawClassicTestSignal(QPainter* painter);
  void _drawPALTestCard(QPainter *painter);
  void _drawNTSCTestCard(QPainter *painter);

  void _drawResolutionText(QPainter *painter, const QRect &rect, int fontSize);

  // Scales the view so that the output frame fills the window.
  void _fitOutputFrame();

  // Paints black over the parts of rect outside the output frame.
  void _maskOutsideOutputFrame(QPainter* painter, const QRectF& rect);

  QRectF _outputFrame;

  // Crosshair sizes, in window pixels.
  static constexpr qreal CROSSHAIR_LINE_WIDTH   = 4;
  static constexpr qreal CROSSHAIR_POINTER_SIZE = 20;

  // Shows what is for editing (mouse cursor, crosshair, outlines and vertices of
  // the shapes), until the mouse is idle.
  void _showEditing();

  // Hides what is for editing, so that only the mapping is left. Does nothing
  // if the mouse is elsewhere, or if one of its buttons is held down.
  void _hideEditing();

  // How long the mouse must be idle before editing is hidden (in milliseconds).
  static const int EDITING_HIDE_DELAY = 3000;

  bool _displayCrosshair;
  bool _displayTestSignal;
  QImage _classicTestCard;
  QBrush _brush_test_signal;
  QImage _palTestCard;
  QImage _ntscTestCard;
  bool _windowIsHovered;

  // False when what is for editing is hidden, because the mouse is idle.
  bool _editingIsVisible;

  // Last known position of the mouse (in screen coordinates).
  QPoint _lastPointerPosition;

  // Hides what is for editing once the mouse is idle.
  QTimer _idleTimer;

#ifdef HAVE_SYPHON
  SyphonOutput _syphonOutput;
#endif

protected:
  void resizeEvent(QResizeEvent* event) override;

  /// The scene rectangle stays the output frame (see setOutputFrame()).
  void _updateSceneRect(qreal scale) override { Q_UNUSED(scale); }

  // Show what is for editing again when the mouse is used, or a vertex is moved
  // with the keyboard.
  bool viewportEvent(QEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

  void wheelEvent(QWheelEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent * event) override;
  void leaveEvent(QEvent *event) override;
};

}

#endif /* OutputGLCanvas_H_ */
