// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#include <iostream>
#include <QTranslator>
#include <QtGui>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include "MM.h"
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
  set_env_vars_if_needed();

  MainApplication app(argc, argv);

  QCommandLineParser parser;
  parser.setApplicationDescription("Video mapping editor");
  const QCommandLineOption helpOption = parser.addHelpOption();
  const QCommandLineOption versionOption = parser.addVersionOption();
  // A boolean option for running it via GUI (--gui)
  QCommandLineOption fullscreenOption(QStringList() << "f" << "fullscreen",
    "Display the output window and make it fullscreen.");
  parser.addOption(fullscreenOption);

  parser.process(app);
  if (parser.isSet(versionOption) || parser.isSet(helpOption))
  {
    return 0;
  }

  if (!QGLFormat::hasOpenGL())
    qFatal("This system has no OpenGL support.");


  // Create splash screen.
  QPixmap pixmap("splash.png");
  QSplashScreen splash(pixmap);

  // Show splash.
  splash.show();

  splash.showMessage("  " + QObject::tr("Initiating program..."),
                     Qt::AlignLeft | Qt::AlignTop, MM::WHITE);

  bool FORCE_FRENCH_LANG = false;
  // set_language_to_french(app);
  if (FORCE_FRENCH_LANG)
  {
    std::cerr << "This system has no OpenGL support" << std::endl;
    return 1;
  }

  // Let splash for at least one second.
  I::sleep(1);

  // Create window.
  MainWindow win;

  QFontDatabase db;
  Q_ASSERT( QFontDatabase::addApplicationFont(":/base-font") != -1);
  app.setFont(QFont(":/base-font", 10, QFont::Bold));

  // Load stylesheet.
  QFile stylesheet("mapmap.qss");
  stylesheet.open(QFile::ReadOnly);
  app.setStyleSheet(QLatin1String(stylesheet.readAll()));

  //win.setLocale(QLocale("fr"));

  // Terminate splash.
  splash.showMessage("  " + QObject::tr("Done."),
                     Qt::AlignLeft | Qt::AlignTop, MM::WHITE);
  splash.finish(&win);
  splash.raise();

  if (parser.isSet(fullscreenOption))
  {
    qDebug() << "TODO: Running in fullscreen mode";
    win.enableFullscreen();
  }

  // Launch program.
  win.show();

  // Start app.
  return app.exec();
}

