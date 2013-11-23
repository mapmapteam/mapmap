// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <QApplication>
#include <QtGui>

#include "Common.h"
#include "DestinationGLCanvas.h"
#include "SourceGLCanvas.h"

#include <iostream>

#include <QtGui>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  if (!QGLFormat::hasOpenGL())
  {
    std::cerr << "This system has no OpenGL support" << std::endl;
    return 1;
  }

  SourceGLCanvas sourceCanvas;
  DestinationGLCanvas destinationCanvas(0, &sourceCanvas);

  Common::initializeLibremapper(320, 480);

  QSplitter splitter(Qt::Horizontal);
  splitter.addWidget(&sourceCanvas);
  splitter.addWidget(&destinationCanvas);

  sourceCanvas.setFocusPolicy(Qt::ClickFocus);
  destinationCanvas.setFocusPolicy(Qt::ClickFocus);

  splitter.setWindowTitle(QObject::tr("Libremapping"));
  splitter.resize(640, 480);
  splitter.show();

  return app.exec();
}


