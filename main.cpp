// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <iostream>
#include <QTranslator>
#include <QtGui>
#include "Common.h"
#include "MainWindow.h"
#include "MainApplication.h"

// This class is just used to provide sleep functionalities in the main() method.
class I : public QThread
{
public:
  static void sleep(unsigned long secs) {
    QThread::sleep(secs);
  }
  static void msleep(unsigned long msecs) {
    QThread::msleep(msecs);
  }
  static void usleep(unsigned long usecs) {
    QThread::usleep(usecs);
  }
};

int main(int argc, char *argv[])
{
  // TODO: avoid segfaults when OSC port is busy
  MainApplication app(argc, argv);

  if (!QGLFormat::hasOpenGL())
    qFatal(QObject::tr("This system has no OpenGL support.").toUtf8().constData());

  // Create splash screen.
  QPixmap pixmap("splash.png");
  QSplashScreen splash(pixmap);

  // Show splash.
  splash.show();

  splash.showMessage("  " + QObject::tr("Initiating your program now..."),
                     Qt::AlignLeft | Qt::AlignTop, QColor("#f6f5f5"));

  // Set translator.
  QTranslator translator;
  translator.load("mapmap_fr");
  app.installTranslator(&translator);

  // Let splash for at least one second.
  I::sleep(1);

  // Create window.
  MainWindow win;
  //win.setLocale(QLocale("fr"));

  // Load file from commandline (optional).
  if (QCoreApplication::arguments().size() > 1)
    win.loadFile(QCoreApplication::arguments().at(1));

  // Show window.
  win.show();

  // Terminate splash.
  splash.finish(&win);
  splash.raise();

  // Start app.
  return app.exec();
}

