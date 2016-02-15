// NOTE: To run, it is recommended not to be in Compiz or Beryl, they have shown some instability.

#define USING_QT_5 (QT_VERSION >= QT_VERSION_CHECK(5,0,0))

#include <iostream>
#include <QTranslator>
#include <QDebug>
#if USING_QT_5
#include <QCommandLineParser>
#include <QCommandLineOption>
#endif
#include "MM.h"
#include "MainWindow.h"
#include "MainApplication.h"

#include "MetaObjectRegistry.h"

#include <stdlib.h>
#include <iostream>

static void set_env_vars_if_needed()
{
#ifdef __MACOSX_CORE__
  std::cout << "OS X detected. Set environment for GStreamer support." << std::endl;
  if (0 == setenv("GST_PLUGIN_PATH", "/Library/Frameworks/GStreamer.framework/Libraries", 1))
      std::cout << " * GST_PLUGIN_PATH=/Library/Frameworks/GStreamer.framework/Libraries" << std::endl;
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

void initRegistry()
{
  MetaObjectRegistry& registry = MetaObjectRegistry::instance();

  // Paints.
  registry.add<Media>();
  registry.add<Image>();
  registry.add<Color>();

  // Mappings.
  registry.add<TextureMapping>();
  registry.add<ColorMapping>();

  // Shapes.
  registry.add<Quad>();
  registry.add<Mesh>();
  registry.add<Ellipse>();
  registry.add<Triangle>();
}

// Intercept all logging message and display it in the console
void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  ConsoleWindow::getInstance()->messageLog(type, context, msg);
}

int main(int argc, char *argv[])
{
  // Install message handler
  qInstallMessageHandler(logMessageHandler);

  set_env_vars_if_needed();

  // Initialize meta-object registry.
  initRegistry();

  MainApplication app(argc, argv);

#if USING_QT_5
  QCommandLineParser parser;
  parser.setApplicationDescription("Video mapping editor");

  // --help option
  const QCommandLineOption helpOption = parser.addHelpOption();

  // --version option
  const QCommandLineOption versionOption = parser.addVersionOption();

  // --fullscreen option
  QCommandLineOption fullscreenOption(QStringList() << "F" << "fullscreen",
    "Display the output window and make it fullscreen.");
  parser.addOption(fullscreenOption);

  // --file option
  QCommandLineOption fileOption(QStringList() << "f" << "file", "Load project from <file>.", "file", "");
  parser.addOption(fileOption);

  // --reset-settings option
  QCommandLineOption resetSettingsOption(QStringList() << "R" << "reset-settings",
    "Reset MapMap settings, such as GUI properties.");
  parser.addOption(resetSettingsOption);

  // --osc-port option
  QCommandLineOption oscPortOption(QStringList() << "p" << "osc-port", "Use OSC port number <osc-port>.", "osc-port", "");
  parser.addOption(oscPortOption);

  // Positional argument: file
  parser.addPositionalArgument("file", "Load project from that file.");

  parser.process(app);
  if (parser.isSet(versionOption) || parser.isSet(helpOption))
  {
    return 0;
  }
  if (parser.isSet(resetSettingsOption))
  {
    Util::eraseSettings();
  }

#endif // USING_QT_5

  if (! QGLFormat::hasOpenGL())
  {
    qFatal("This system has no OpenGL support.");
    return 1;
  }

  // Create splash screen.
  QPixmap pixmap(":/mapmap-splash");
  QSplashScreen splash(pixmap);

  // Show splash.
  splash.show();

  splash.showMessage("  " + QObject::tr("Initiating program..."),
                     Qt::AlignLeft | Qt::AlignTop, MM::WHITE);

  bool FORCE_FRENCH_LANG = false;
  // set_language_to_french(app);

  // Let splash for at least one second.
  I::sleep(1);

  // Create window.
  MainWindow* win = MainWindow::instance();
  // Add custom font
  int id = QFontDatabase::addApplicationFont(":/base-font");
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  app.setFont(QFont(family, 11, QFont::Normal));

  // Load stylesheet.
  QFile stylesheet(":/stylesheet");
  stylesheet.open(QFile::ReadOnly);
  app.setStyleSheet(QLatin1String(stylesheet.readAll()));

  //win.setLocale(QLocale("fr"));

#if USING_QT_5
  // read positional argument:
  const QStringList args = parser.positionalArguments();
  QString projectFileValue = QString();

  // there are two ways to specify the project file name.
  // The 2nd overrides the first:

  // read the file option value: (overrides the positional argument)
  projectFileValue = parser.value("file");
  // read the first positional argument:
  if (! args.isEmpty())
  {
    projectFileValue = args.first();
  }

  // finally, load the project file.
  if (projectFileValue != "")
  {
    win->loadFile(projectFileValue);
  }

  QString oscPortNumberValue = parser.value("osc-port");
  if (oscPortNumberValue != "")
  {
    win->setOscPort(oscPortNumberValue);
  }
#endif

  // Terminate splash.
  splash.showMessage("  " + QObject::tr("Done."),
                     Qt::AlignLeft | Qt::AlignTop, MM::WHITE);
  splash.finish(win);
  splash.raise();

  // Launch program.
  win->show();

#if USING_QT_5
  if (parser.isSet(fullscreenOption))
  {
    qDebug() << "TODO: Running in fullscreen mode";
    win->startFullScreen();
  }
#endif

  // Start app.
  int result = app.exec();

  delete win;
  return result;
}

