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

  // Load translation.
  QTranslator translator;
  translator.load("mapmap_fr");
  app.installTranslator(&translator);

  // Create main window.
  MainWindow win;

  QFontDatabase db;
  Q_ASSERT( QFontDatabase::addApplicationFont(":/base-font") != -1);
  app.setFont(QFont(":/base-font", 10, QFont::Bold));

  // Load stylesheet.
  QFile stylesheet("mapmap.qss");
  stylesheet.open(QFile::ReadOnly);
  app.setStyleSheet(QLatin1String(stylesheet.readAll()));

  //win.setLocale(QLocale("fr"));

  // Launch program.
  win.show();

  return app.exec();
}

