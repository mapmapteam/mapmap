// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <iostream>
#include <QApplication>
#include <QtGui>
#include "Common.h"
#include "MainWindow.h"
#include "Controller.h"
#include <QPointF>

#include <iostream>

int main(int argc, char *argv[])
{
  // TODO: avoid segfaults when OSC port is busy
  QApplication app(argc, argv);

  if (! QGLFormat::hasOpenGL())
  {
    std::cerr << "This system has no OpenGL support" << std::endl;
    return 1;
  }

  // some examples, commented out
  /* Controller *control = new Controller(&MainWindow::getInstance());
  control->createObject("Point", "point1");
  control->createObject("Point", "point2");
  QVariant var(1.55), var2;
  QList<QString> names, objs;
  QVariantList values;
  control->setObjectProperty("point1","x", QVariant(1.23));
  control->setObjectProperty("point1","y", QVariant(3.14));
  control->getObjectProperty("point1","y", var2);
  control->getObjectProperty("point1","x", var2);
  control->listObjectProperties("point1", names, values);
  control->listObjects("QObject", objs);*/
  MainWindow::getInstance().show();

  return app.exec();
}


