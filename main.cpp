// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <iostream>
#include <QTranslator>
#include <QtGui>
#include "Common.h"
#include "MainWindow.h"
#include "MainApplication.h"

int main(int argc, char *argv[])
{
  // TODO: avoid segfaults when OSC port is busy
  MainApplication app(argc, argv);

  if (! QGLFormat::hasOpenGL())
  {
    std::cerr << "This system has no OpenGL support" << std::endl;
    return 1;
  }

  QTranslator translator;
  translator.load("mapmap_fr");
  app.installTranslator(&translator);

  MainWindow win;

  //win.setLocale(QLocale("fr"));

  win.show();

  return app.exec();
}

