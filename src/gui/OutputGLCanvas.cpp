/*
 * OutputGLCanvas.cpp
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

#include "OutputGLCanvas.h"
#include "MainWindow.h"

namespace mmp {

OutputGLCanvas::OutputGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget* shareWidget, QGraphicsScene* scene)
: MapperGLCanvas(mainWindow, true, parent, shareWidget, scene),
  _displayCrosshair(false),
  _displayTestSignal(false),
  _windowIsHovered(false)
{
  // Disable scrollbars.
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setSceneRectToViewportGeometry();
}

void OutputGLCanvas::setSceneRectToViewportGeometry()
{
  setSceneRect(viewport()->geometry());
}

void OutputGLCanvas::drawForeground(QPainter *painter , const QRectF &rect)
{
  QSettings settings;
  bool controlOnMouseOver = settings.value("showControlOnMouseOver", MM::SHOW_OUTPUT_ON_MOUSE_HOVER).toBool();

  if (_displayTestSignal)
  {
    // Draw the preferred signal test card
    int testCard = settings.value("signalTestCard", MM::DEFAULT_TEST_CARD).toInt();
    glPushMatrix();
    painter->translate(rect.x(), rect.y());
    painter->setRenderHint(QPainter::Antialiasing);
    painter->save();
    switch (testCard) {
    case MM::Classic:
      _drawClassicTestSignal(painter);
      break;
    case MM::PAL:
      _drawPALTestCard(painter);
      break;
    case MM::NTSC:
      _drawNTSCTestCard(painter);
      break;
    default: // Do nothing;
      break;
    }
    painter->restore();
    glPopMatrix();
  }
  else if (!controlOnMouseOver || (MainWindow::window()->displayControls() && _windowIsHovered))
  {
    MapperGLCanvas::drawForeground(painter, rect);

    // Display crosshair cursor.
    if (_displayCrosshair)
    {
#ifdef Q_OS_OSX
      QPoint globalCursorPos = QCursor::pos();
      int mouseScreen = QApplication::desktop()->screenNumber(globalCursorPos);
      QRect mouseScreenGeometry = QApplication::desktop()->screen(mouseScreen)->geometry();
      QPoint localCursorPos = globalCursorPos - mouseScreenGeometry.topLeft();
      QPointF cursorPosition = mapToScene(localCursorPos);
//      qDebug() << "Cursor pos " << globalCursorPos << " " << cursorPosition << " " << localCursorPos << mouseScreen << endl;
      if (rect.contains(cursorPosition) && getMainWindow()->getPreferredScreen() == mouseScreen)
//      qDebug() << "Cursor pos " << mapToScene(mapFromGlobal(QCursor::pos(QApplication::screens()[1])));
#else
      QPointF cursorPosition = mapToScene(mapFromGlobal(cursor().pos()));// - rect.topLeft();//(QCursor::pos());///*this->mapFromGlobal(*/QCursor::pos()/*)*/;
      if (rect.contains(cursorPosition))
#endif
      {
//        painter->setPen(MM::CONTROL_COLOR);
//        painter->drawLine(cursorPosition.x(), rect.y(), cursorPosition.x(), rect.height());
//        painter->drawLine(rect.x(), cursorPosition.y(), rect.width(), cursorPosition.y());
        // Draw crosshair line
        painter->setPen(MM::CROSSHAIR_STROKE);
        painter->setBrush(MM::CONTROL_COLOR);
        painter->drawRect(cursorPosition.x() - 2, rect.y(), 4, rect.height());
        painter->drawRect(rect.x(), cursorPosition.y() - 2, rect.width(), 4);
        // Draw crosshair pointer
        painter->setPen(MM::CONTROL_COLOR);
        painter->setBrush(MM::CROSSHAIR_STROKE);
        painter->drawRect(cursorPosition.x() - 10, cursorPosition.y() - 10, 20, 20);
      }
    }
  }

}

void OutputGLCanvas::enterEvent(QEvent *event)
{
  _windowIsHovered = true;
  QGraphicsView::enterEvent(event);
}

void OutputGLCanvas::leaveEvent(QEvent *event)
{
  _windowIsHovered = false;
  QGraphicsView::leaveEvent(event);
}

void OutputGLCanvas::_drawClassicTestSignal(QPainter* painter)
{
  const QRect& geo = geometry();
  int width = geo.width();
  int height = geo.height();
  int rectSize = 10;
  QColor darkerGray(191, 191, 191);
  QColor darkGray(Qt::darkGray);

  painter->setPen(Qt::NoPen);

  // Draw checkerboard pattern.
  for (int x = geo.x(); x < width; x += rectSize)
  {
    for (int y = geo.y(); y < height; y += rectSize)
    {
      if (((x + y) % 20) == 0)
      {
        painter->setBrush(darkerGray);
      } else {
        painter->setBrush(darkGray);
      }
      painter->drawRect(x, y, rectSize, rectSize);
    }
  }

  // Create responsive image
  _classicTestCard = QImage(":/test-signal").scaled(height, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  int imageX = (width - _classicTestCard.width()) / 2;
  int imageY = (height - _classicTestCard.height()) / 2;

  // Draw the image.
  painter->drawImage(imageX, imageY, _classicTestCard);
}

void OutputGLCanvas::_drawPALTestCard(QPainter* painter)
{
  const QRect& geo = geometry();
  int width = geo.width();
  int height = geo.height();
  int rectSize = 85;
  int marginX = (rectSize - width % rectSize) / 2;
  int marginY = (rectSize - height % rectSize) / 2;
  QColor black(Qt::black);
  QColor white(Qt::white);
  QColor darkGray(Qt::darkGray);

  // Draw checkerboard pattern.
  for (int x = geo.x(); x < width; x += rectSize)
  {
    for (int y = geo.y(); y < height; y += rectSize)
    {
      painter->setPen(Qt::NoPen);
      if (((x + y) % (rectSize * 2)) == 0)
      {
        painter->setBrush(QBrush(white));
      } else {
        painter->setBrush(QBrush(black));
      }

      if ((x > 0 && x <= (width - rectSize)) && (y > 0 && y <= (height - rectSize))) {
        painter->setBrush(QBrush(darkGray));
        painter->setPen(QPen(QBrush(white), 3));
      }

      painter->drawRect(x - marginX, y - marginY, rectSize, rectSize);
    }
  }

  // Create responsive image
  _palTestCard = QImage(":/pal-test-card").scaled(height - (rectSize * 2), height - (rectSize * 2),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
  // Draw image
  int imageX = (width - _palTestCard.width()) / 2;
  int imageY = (height - _palTestCard.height()) / 2;
  int imageHeight = _palTestCard.height();
  painter->drawImage(imageX, imageY, _palTestCard);

  // Draw text for screen resolution
  int fontSize = imageHeight / 18;
  QRect textRect((width / 2) - (fontSize * 3), imageY + (imageHeight / 19), fontSize * 6, fontSize);
  _drawResolutionText(painter, textRect, fontSize);
}

void OutputGLCanvas::_drawNTSCTestCard(QPainter* painter)
{
  const QRect& geo = geometry();
  int width = geo.width();
  int height = geo.height();

  // Create image
  _ntscTestCard = QImage(":/ntsc-test-card").scaled(width, height);

  // Draw backgroung image
  painter->drawImage(geo.x(), geo.y(), _ntscTestCard);
  // Draw logo
  QImage mapmapLogo = QImage(":/mapmap-logo-with-border").scaled(width, height / 15, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  painter->drawImage((width - mapmapLogo.width()) / 2, height / 4, mapmapLogo);


  // Draw text for screen resolution
  int fontSize = height / 21;
  QRect textRect((width / 2) - (fontSize * 3), height / 3, fontSize * 6, fontSize);
  _drawResolutionText(painter, textRect, fontSize);
}

void OutputGLCanvas::_drawResolutionText(QPainter *painter, const QRect &rect, int fontSize)
{
  QSettings settings;

  if (settings.value("showResolution").toBool())
  {
    painter->fillRect(rect, Qt::black);
    QFont font = painter->font();
    font.setPixelSize(fontSize);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter,
                      QString::number(geometry().width()) +
                      " x " + QString::number(geometry().height()));
  }
}

void OutputGLCanvas::resizeGL(int width, int height)
{
  setSceneRectToViewportGeometry();
}

void OutputGLCanvas::wheelEvent(QWheelEvent *event)
{
  event->ignore();
}

void OutputGLCanvas::mouseMoveEvent(QMouseEvent *event)
{
  // Click-and-drag translate view.
  if (event->buttons() & Qt::MiddleButton)
  {
    event->ignore();
  }
  else
  {
    MapperGLCanvas::mouseMoveEvent(event);
  }
}

}
