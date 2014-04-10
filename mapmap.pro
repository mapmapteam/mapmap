CONFIG  += qt debug
TEMPLATE = app
HEADERS  = \
    DestinationGLCanvas.h \
    MainApplication.h \
    MainWindow.h \
    Mapper.h \
    MapperGLCanvas.h \
    Mapping.h \
    MappingManager.h \
    Maths.h \
    Paint.h \
    OscInterface.h \
    OscReceiver.h \
    OutputGLWindow.h \
    ProjectReader.h \
    ProjectWriter.h \
    Shape.h \
    SourceGLCanvas.h \
    UidAllocator.h \
    Util.h

SOURCES  = \
    DestinationGLCanvas.cpp \
    MainWindow.cpp \
    MainApplication.cpp \
    Mapper.cpp \
    MapperGLCanvas.cpp \
    Mapping.cpp \
    MappingManager.cpp \
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
docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# Linux-specific:
unix:!mac {
  DEFINES += UNIX
  # stricter build flags:
  QMAKE_CXXFLAGS += -Wno-unused-result -Wfatal-errors
  QMAKE_CXXFLAGS += -DHAVE_OSC
  INCLUDEPATH += /usr/include/gstreamer-0.10 \
    /usr/local/include/gstreamer-0.10 \
    /usr/include/glib-2.0 \
    /usr/lib/x86_64-linux-gnu/glib-2.0/include \
    /usr/include/libxml2
  LIBS += \
    -lglut \
    -lGLU \
    -llo \
    -lpthread \
    -lX11 \
    -lGLEW
}

# Mac OS X-specific:
mac {
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

  #  -lGLEW
}

