CONFIG  += qt debug
TEMPLATE = app
VERSION = 0.3.1
TARGET = mapmap
QT += gui opengl xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB

HEADERS  = \
    Commands.h \
    DestinationGLCanvas.h \
    MM.h \
    MainApplication.h \
    MainWindow.h \
    Mapper.h \
    MapperGLCanvas.h \
    Mapping.h \
    MappingManager.h \
    Maths.h \
    MediaImpl.h \
    OscInterface.h \
    OscReceiver.h \
    OutputGLCanvas.h \
    OutputGLWindow.h \
    Paint.h \
    PaintGui.h \
    PreferencesDialog.h \
    ProjectReader.h \
    ProjectWriter.h \
    Shape.h \
    SourceGLCanvas.h \
    UidAllocator.h \
    Util.h

SOURCES  = \
    Commands.cpp \
    DestinationGLCanvas.cpp \
    MM.cpp \
    MainApplication.cpp \
    MainWindow.cpp \
    Mapper.cpp \
    MapperGLCanvas.cpp \
    Mapping.cpp \
    MappingManager.cpp \
    MediaImpl.cpp \
    OscInterface.cpp \
    OscReceiver.cpp \
    OutputGLCanvas.cpp \
    OutputGLWindow.cpp \
    Paint.cpp \
    PaintGui.cpp \
    PreferencesDialog.cpp \
    ProjectReader.cpp \
    ProjectWriter.cpp \
    Shape.cpp \
    SourceGLCanvas.cpp \
    UidAllocator.cpp \
    Util.cpp \
    main.cpp

RESOURCES = mapmap.qrc
TRANSLATIONS = resources/texts/mapmap_*.ts
include(contrib/qtpropertybrowser/src/qtpropertybrowser.pri)
include(contrib/qtpropertybrowser-extension/qtpropertybrowser-extension.pri)

# Add the docs target:
docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# Linux-specific:
unix:!mac {
  DEFINES += UNIX
  CONFIG += link_pkgconfig
  INCLUDE_PATH += 
  PKGCONFIG += \
    gstreamer-1.0 gstreamer-base-1.0 gstreamer-app-1.0 \
    liblo \
    gl x11 
  QMAKE_CXXFLAGS += -Wno-unused-result -Wfatal-errors
  QMAKE_CXXFLAGS += -DHAVE_OSC
  mapmapfile.files = mapmap
  mapmapfile.path = /usr/bin
  INSTALLS += mapmapfile
  desktopfile.files = resources/texts/mapmap.desktop
  desktopfile.path = /usr/share/applications
  INSTALLS += desktopfile
  iconfile.files = resources/images/logo/mapmap.svg
  iconfile.path = /usr/share/icons/hicolor/scalable/apps
  INSTALLS += iconfile
  mimetypesfile.files = resources/texts/mapmap.xml 
  mimetypesfile.path = /usr/share/mime/packages
  INSTALLS += mimetypesfile

# REQUIRES ROOT PRIVILEDGES: (does not comply to the standards of Debian)
# -------------------------
# updatemimetypes.path = /usr/share/mime/packages
# updatemimetypes.commands = update-mime-database /usr/share/mime
# INSTALLS += updatemimetypes
# updatemimeappdefault.path = /usr/share/applications
# updatemimeappdefault.commands='grep mapmap.desktop /usr/share/applications/defaults.list >/dev/null|| sudo echo "application/mapmap=mapmap.desktop;" >> /usr/share/applications/defaults.list'
# INSTALLS += updatemimeappdefault
# -------------------------
  
  # Add the docs target:
  docs.depends = $(HEADERS) $(SOURCES)
  docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
  QMAKE_EXTRA_TARGETS += docs
}

# Mac OS X-specific:
mac {
  TARGET = MapMap
  DEFINES += MACOSX
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
  QMAKE_CXXFLAGS += -stdlib=libstdc++
  INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Versions/1.0/Headers
  LIBS += -F /Library/Frameworks/ -framework GStreamer
  LIBS += -framework OpenGL -framework GLUT
  # With Xcode Tools > 1.5, to reduce the size of your binary even more:
  # LIBS += -dead_strip
  # This tells qmake not to put the executable inside a bundle.
  # just for reference. Do not uncomment.
  # CONFIG-=app_bundle
}

# Windows-specific:
win32 {
  DEFINES += WIN32
  INCLUDEPATH += \
    C:/gstreamer/include \
    C:/gstreamer/include/libxml2 \
    C:/gstreamer/include/glib-2.0 \
    C:/gstreamer/lib/glib-2.0/include \
    C:/gstreamer/include/gstreamer-0.10
  LIBS += -L"C:/gstreamer/lib" \
    -L"C:/gstreamer/bin" \
    -lgstreamer-0.10 \
    -lglib-2.0 \
    -lgmodule-2.0 \
    -lgobject-2.0 \
    -lgthread-2.0 \
    -lgstinterfaces-0.10 \
    -lopengl32 \
    -lglu32
  # Add console to the CONFIG to see debug messages printed in 
  # the console on Windows
  CONFIG += console
}

# Adds the tarball target
tarball.target = mapmap-$${VERSION}.tar.gz
tarball.commands = git archive --format=tar.gz -9 --prefix=mapmap-$${VERSION}/ --output=mapmap-$${VERSION}.tar.gz HEAD
tarball.depends = .git
QMAKE_EXTRA_TARGETS += tarball

# Show various messages
message("MapMap version: $${VERSION}")
# message("Qt version: $$[QT_VERSION]")
# message("LIBS: $${LIBS}")
# message("PKGCONFIG: $${PKGCONFIG}")
# message("The project contains the following files: $${SOURCES} $${HEADERS}}")
# message("To create a tarball, run `make tarball`")
# message("To build the documentation, run `make docs`")

