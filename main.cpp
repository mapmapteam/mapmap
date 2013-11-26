// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <QApplication>
#include <QtGui>

#include "Common.h"
#include "MainWindow.h"

#include <iostream>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  if (!QGLFormat::hasOpenGL())
  {
    std::cerr << "This system has no OpenGL support" << std::endl;
    return 1;
  }

  MainWindow window;
  window.show();

  return app.exec();
}


