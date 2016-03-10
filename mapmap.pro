CONFIG  += qt debug c++11
TEMPLATE = app
VERSION = 0.3.2
TARGET = mapmap
QT += gui opengl xml core
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets core
DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB

HEADERS  = \
    Commands.h \
    ConcurrentQueue.h \
    ConsoleWindow.h \
    Element.h \
    Ellipse.h \
    MM.h \
    MainApplication.h \
    MainWindow.h \
    MappingGui.h \
    MappingItemDelegate.h \
    MappingListModel.h \
    MapperGLCanvas.h \
    MapperGLCanvasToolbar.h \
    Mapping.h \
    MappingManager.h \
    Maths.h \
    VideoImpl.h \
    Mesh.h \
    MetaObjectRegistry.h \
    OscInterface.h \
    OscReceiver.h \
    OutputGLCanvas.h \
    OutputGLWindow.h \
    Paint.h \
    PaintGui.h \
    Polygon.h \
    PreferencesDialog.h \
    ProjectLabels.h \
    ProjectReader.h \
    ProjectWriter.h \
    Quad.h \
    Serializable.h \
    Shape.h \
    Shapes.h \
    ShapeControlPainter.h \
    ShapeGraphicsItem.h \
    Triangle.h \
    UidAllocator.h \
    Util.h

SOURCES  = \
    Commands.cpp \
    ConsoleWindow.cpp \
    Element.cpp \
    Ellipse.cpp \
    MM.cpp \
    MainApplication.cpp \
    MainWindow.cpp \
    MappingGui.cpp \
    MappingItemDelegate.cpp \
    MappingListModel.cpp \
    MapperGLCanvas.cpp \
    MapperGLCanvasToolbar.cpp \
    Mapping.cpp \
    MappingManager.cpp \
    VideoImpl.cpp \
    Mesh.cpp \
    MetaObjectRegistry.cpp \
    OscInterface.cpp \
    OscReceiver.cpp \
    OutputGLCanvas.cpp \
    OutputGLWindow.cpp \
    Paint.cpp \
    PaintGui.cpp \
    Polygon.cpp \
    PreferencesDialog.cpp \
    ProjectLabels.cpp \
    ProjectReader.cpp \
    ProjectWriter.cpp \
    Serializable.cpp \
    Shape.cpp \
    ShapeControlPainter.cpp \
    ShapeGraphicsItem.cpp \
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
    gstreamer-1.0 gstreamer-base-1.0 gstreamer-app-1.0 gstreamer-pbutils-1.0 \
    liblo \
    gl x11 
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-result -Wno-unused-parameter \
                            -Wno-unused-variable -Wno-switch -Wno-comment \
                            -Wno-unused-but-set-variable
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

  # For OSC support: (if pkg-config was installed)
  # CONFIG += link_pkgconfig
  # PKGCONFIG += lo

  LIBS += -L/usr/local/lib -llo
  INCLUDEPATH += /usr/local/include
  QMAKE_CXXFLAGS += -DHAVE_OSC
}

# Windows-specific:
win32 {
  DEFINES += WIN32
  INCLUDEPATH += \
#    C:/gstreamer/include \
#    C:/gstreamer/include/libxml2 \
#    C:/gstreamer/include/glib-2.0 \
#    C:/gstreamer/lib/glib-2.0/include \
#    C:/gstreamer/include/gstreamer-0.10
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

