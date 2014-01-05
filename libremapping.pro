CONFIG  += qt debug
TEMPLATE = app
HEADERS  = \
    DestinationGLCanvas.h \
    Facade.h \
    MainWindow.h \
    Mapper.h \
    MapperGLCanvas.h \
    Mapping.h \
    MappingManager.h \
    NameAllocator.h \
    Paint.h \
    ProjectReader.h \
    ProjectWriter.h \
    Shape.h \
    SourceGLCanvas.h \
    Util.h

SOURCES  = \
    Controller.cpp \
    DestinationGLCanvas.cpp \
    Facade.cpp \
    MainWindow.cpp \
    Mapper.cpp \
    MapperGLCanvas.cpp \
    MappingManager.cpp \
    NameAllocator.cpp \
    ProjectReader.cpp \
    ProjectWriter.cpp \
    SourceGLCanvas.cpp \
    Util.cpp \
    main.cpp

QT      += gui opengl xml
RESOURCES = libremapping.qrc

include(contrib/qtpropertybrowser/src/qtpropertybrowser.pri)

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# mac
macx:LIBS += -framework OpenGL -framework GLUT
macx:QMAKE_CXXFLAGS += -D__MACOSX_CORE__

# not mac
!macx:LIBS    += -lglut -lGLU
