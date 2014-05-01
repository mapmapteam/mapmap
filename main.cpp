// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <iostream>
#include <QTranslator>
#include <QtGui>
#include "Common.h"
#include "MainWindow.h"
#include "MainApplication.h"
#include <stdlib.h>
#include <iostream>

static void set_env_vars_if_needed()
{
#ifdef __MACOSX_CORE__
  std::cout << "OS X detected. Set environment for GStreamer-SDK support." << std::endl;
  if (0 == setenv("GST_PLUGIN_PATH", "/Library/Frameworks/GStreamer.framework/Libraries", 1))
      std::cout << " * GST_PLUGIN_PATH=Library/Frameworks/GStreamer.framework/Libraries" << std::endl;
  if (0 == setenv("GST_DEBUG", "2", 1))
      std::cout << " * GST_DEBUG=2" << std::endl;
  //setenv("LANG", "C", 1);
#endif // __MACOSX_CORE__
}

int main(int argc, char *argv[])
{
  set_env_vars_if_needed();

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

