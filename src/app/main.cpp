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

MM_USE_NAMESPACE

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
  registry.add<Video>();
  registry.add<Image>();
  registry.add<Color>();

  // Mappings.
  registry.add<TextureMapping>();
  registry.add<ColorMapping>();

  // Shapes.
  registry.add<Quad>();
  registry.add<Mesh>();
  registry.add<MM_PREPEND_NAMESPACE(Ellipse)>();
  registry.add<Triangle>();
}

// Intercept all logging message and display it in the console
void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  ConsoleWindow::console()->printMessage(type, context, msg);
}

int main(int argc, char *argv[])
{
  set_env_vars_if_needed();

  // Initialize meta-object registry.
  initRegistry();

  MainApplication app(argc, argv);
  
  // Install message handler
  // after QGuiApplication has been instanciated
  qInstallMessageHandler(logMessageHandler);

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
  QCommandLineOption fileOption(QStringList() << "f" << "file",
    "Load project from <file>.", "file", "");
  parser.addOption(fileOption);

  // --reset-settings option
  QCommandLineOption resetSettingsOption(QStringList() << "R" << "reset-settings",
    "Reset MapMap settings, such as GUI properties.");
  parser.addOption(resetSettingsOption);

  // --osc-port option
  QCommandLineOption oscPortOption(QStringList() << "p" << "osc-port",
    "Use OSC port number <osc-port>.", "osc-port", "");
  parser.addOption(oscPortOption);

  // --lang option
  QCommandLineOption localeOption(QStringList() << "l" << "lang",
    "Use language <lang>.", "lang", "");
  parser.addOption(localeOption);

  // --frame-rate option
  QCommandLineOption frameRateOption(QStringList() << "r" << "frame-rate",
    "Use a framerate of <frame-rate> per second.", "frame-rate", QString::number(MM::DEFAULT_FRAMES_PER_SECOND));
  parser.addOption(frameRateOption);

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

  // IMPORTANT: Translator must be set *before* the MainWindow is created for it to work.
  QSettings settings;
  // Get language from command line or user settings
  QString lang = parser.value("lang").isEmpty()
                 ? settings.value("language").toString()
                 : parser.value("lang");

  QTranslator qtTranslator;
  QTranslator appTranslator;
  if (MM::SUPPORTED_LANGUAGES.contains(lang))
  {
    //QLocale::setDefault(QLocale(lang));
#ifdef Q_OS_WIN32
    qtTranslator.load(QString("qt_%1").arg(lang),
                      QApplication::applicationDirPath().append("/translations"));
#else
    qtTranslator.load(QString("qtbase_%1").arg(lang),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
    app.installTranslator(&qtTranslator);

    appTranslator.load(QString(":/translations_mapmap_%1").arg(lang));
    app.installTranslator(&appTranslator);
  }
  else {
    qWarning() << "Unrecognized/unsupported language: " << lang;
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

  // Let splash for at least one second.
  I::sleep(1);

  // Create window.
  MainWindow* win = MainWindow::window();
  // Add custom font
  int id = QFontDatabase::addApplicationFont(":/base-font");
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  app.setFont(QFont(family, 11, QFont::Normal));

  // Load stylesheet.
  QFile stylesheet(":/stylesheet");
  stylesheet.open(QFile::ReadOnly);
  app.setStyleSheet(QLatin1String(stylesheet.readAll()));

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

  QString oscPortValue = parser.value("osc-port");
  if (oscPortValue != "")
    win->setOscPort(oscPortValue);

  bool optionOk;
  qreal fps = parser.value("frame-rate").toDouble(&optionOk);
  if (optionOk)
    win->setFramesPerSecond(fps);
  else
    qFatal("Invalid option <frame-rate>.");

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
    win->startFullScreen();
  }
#endif

  // Start app.
  int result = app.exec();

  delete win;
  return result;
}
