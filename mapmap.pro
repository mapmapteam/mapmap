CONFIG  += qt debug
TEMPLATE = app
VERSION = 0.1.0
TARGET = mapmap
QT += gui opengl xml
CONFIG += qt debug
DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB

HEADERS  = \
    DestinationGLCanvas.h \
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
    OutputGLWindow.h \
    Paint.h \
    ProjectReader.h \
    ProjectWriter.h \
    Shape.h \
    SourceGLCanvas.h \
    UidAllocator.h \
    Util.h

SOURCES  = \
    DestinationGLCanvas.cpp \
    MainApplication.cpp \
    MainWindow.cpp \
    Mapper.cpp \
    MapperGLCanvas.cpp \
    Mapping.cpp \
    MappingManager.cpp \
    MediaImpl.cpp \
    OscInterface.cpp \
    OscReceiver.cpp \
    OutputGLWindow.cpp \
    Paint.cpp \
    ProjectReader.cpp \
    ProjectWriter.cpp \
    Shape.cpp \
    SourceGLCanvas.cpp \
    UidAllocator.cpp \
    Util.cpp \
    main.cpp

QT += gui opengl xml
RESOURCES = mapmap.qrc
TRANSLATIONS = mapmap_fr.ts
include(contrib/qtpropertybrowser/src/qtpropertybrowser.pri)

# Add the docs target:
docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# Linux-specific:
unix:!mac {
  DEFINES += UNIX
  CONFIG += link_pkgconfig
  PKGCONFIG += gstreamer-0.10 gstreamer-base-0.10 glib-2.0 libxml-2.0

  # stricter build flags:
  QMAKE_CXXFLAGS += -Wno-unused-result -Wfatal-errors
  QMAKE_CXXFLAGS += -DHAVE_OSC

  QMAKE_LFLAGS_RPATH+=-Wl,-rpath=/opt/gstreamer-sdk/lib
  #INCLUDEPATH += \
   # /usr/include/gstreamer-0.10 \
   # /usr/local/include/gstreamer-0.10 \
   # /usr/include/glib-2.0 \
   # /usr/lib/x86_64-linux-gnu/glib-2.0/include \
   # /usr/include/libxml2
  LIBS += \
   # -lgstreamer-0.10 \
   # -lgstinterfaces-0.10 \
   # -lglib-2.0 \
    -lglut \
    -llo -lpthread \
  #  -lgmodule-2.0 \
  #  -lgobject-2.0 \
    -lgthread-2.0 \
    -lGL \
    -lGLU \
    -llo \
    -lpthread \
    -lX11 \
    -lGLEW
}

# Mac OS X-specific:
mac {
  TARGET = MapMap
  DEFINES += MACOSX
  INCLUDEPATH += \
    /opt/local/include/ \
    /opt/local/include/libxml2
  LIBS += \
    -framework OpenGL \
    -framework GLUT
  # -L/opt/local/lib \
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
  QMAKE_CXXFLAGS += -stdlib=libstdc++

  INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
  # LIBS+=-lavformat -lavcodec -lavutil -lswscale -lz
  LIBS += -F /Library/Frameworks/ -framework GStreamer
  # QMAKE_LFLAGS+=-read_only_relocs suppress

  # LIBS += -lgstreamer-0.10 \
  #   -lgstapp-0.10 \
  #   -lgstvideo-0.10 \
  #   -lglib-2.0 \
  #   -lgobject-2.0
  LIBS += -framework OpenGL -framework GLUT
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
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
    -lglu32 \
    -lglew32
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

