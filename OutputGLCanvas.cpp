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

MM_BEGIN_NAMESPACE

OutputGLCanvas::OutputGLCanvas(MainWindow* mainWindow, QWidget* parent, const QGLWidget* shareWidget, QGraphicsScene* scene)
: MapperGLCanvas(mainWindow, true, parent, shareWidget, scene),
  _displayCrosshair(false),
  _displayTestSignal(false),
  _svg_test_signal(":/test-signal"),
  _brush_test_signal(_svg_test_signal),
  _palTestCard(":/pal-test-card")
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
  if (_displayTestSignal)
  {
    // Draw the preferred signal test card
    QSettings settings;
    int testCard = settings.value("signalTestCard", MM::DEFAULT_TEST_CARD).toInt();
    glPushMatrix();
    painter->translate(rect.x(), rect.y());
    painter->save();
    switch (testCard) {
    case MM::Classic:
      _drawTestSignal(painter);
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
  else
  {
    MapperGLCanvas::drawForeground(painter, rect);

    // Display crosshair cursor.
    if (_displayCrosshair)
    {
      QPointF cursorPosition = mapToScene(mapFromGlobal(cursor().pos()));// - rect.topLeft();//(QCursor::pos());///*this->mapFromGlobal(*/QCursor::pos()/*)*/;
      if (rect.contains(cursorPosition))
      {
        painter->setPen(MM::CONTROL_COLOR);
        painter->drawLine(cursorPosition.x(), rect.y(), cursorPosition.x(), rect.height());
        painter->drawLine(rect.x(), cursorPosition.y(), rect.width(), cursorPosition.y());
      }
    }
  }

}

void OutputGLCanvas::_drawTestSignal(QPainter* painter)
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
  QImage image = _svg_test_signal.scaled(height, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  int imageX = (width - image.width()) / 2;
  int imageY = (height - image.height()) / 2;

  // Draw the image.
  painter->drawImage(imageX, imageY, image);
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
  QImage image = _palTestCard.scaled(height - (rectSize * 2), height - (rectSize * 2),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
  // Draw image
  int imageX = (width - image.width()) / 2;
  int imageY = (height - image.height()) / 2;
  int imageWidth = image.width();
  int imageHeight = image.height();
  painter->drawImage(imageX, imageY, image);
  // Draw text for screen resolution
  int fontSize = imageHeight / 20;
  QRect textRect((width / 2) - (fontSize * 3), imageY + (imageHeight / 13), fontSize * 6, fontSize);
  _drawResolutionText(painter, textRect, fontSize);
}

void OutputGLCanvas::_drawNTSCTestCard(QPainter* painter)
{
  const QRect& geo = geometry();
  int width = geo.width();
  int height = geo.height();
  QList<QColor> colors;
  QColor white(Qt::white);
  QColor yellow(Qt::yellow);
  QColor cyan(Qt::cyan);
  QColor green(Qt::green);
  QColor magenta(Qt::magenta);
  QColor red(Qt::red);
  QColor blue(Qt::blue);
  QColor black(Qt::black);

  colors.push_back(white);
  colors.push_back(yellow);
  colors.push_back(cyan);
  colors.push_back(green);
  colors.push_back(magenta);
  colors.push_back(red);
  colors.push_back(blue);
  colors.push_back(black);

  const int bandWidth = width / colors.size();

  painter->setPen(Qt::NoPen);

  // Draw checkerboard pattern.
  for (int x = 0; x < width / bandWidth; x++)
  {
    painter->setBrush(colors.at(x));
    painter->drawRect(x * bandWidth, 0, bandWidth, height);
  }

  // Create responsive image
//  QImage image = _palTestCard.scaled(height - (rectSize * 2), height - (rectSize * 2),
//                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
//  // Draw image
//  int imageX = (width - image.width()) / 2;
//  int imageY = (height - image.height()) / 2;
//  int imageWidth = image.width();
//  int imageHeight = image.height();
//  painter->drawImage(imageX, imageY, image);
  // Draw text for screen resolution
  int fontSize = height / 30;
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

MM_END_NAMESPACE
