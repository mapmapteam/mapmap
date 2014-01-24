CONFIG  += qt debug
TEMPLATE = app
HEADERS  = \
    DestinationGLCanvas.h \
#    Facade.h \
    MainApplication.h \
    MainWindow.h \
    Mapper.h \
    MapperGLCanvas.h \
    Mapping.h \
    MappingManager.h \
#    NameAllocator.h \
    Paint.h \
    ProjectReader.h \
    ProjectWriter.h \
    Shape.h \
    SourceGLCanvas.h \
    UidAllocator.h \
    Util.h

SOURCES  = \
#    Controller.cpp \
    DestinationGLCanvas.cpp \
#    Facade.cpp \
    MainWindow.cpp \
    MainApplication.cpp \
    Mapper.cpp \
    MapperGLCanvas.cpp \
    Mapping.cpp \
    MappingManager.cpp \
#    NameAllocator.cpp \
    Paint.cpp \
    ProjectReader.cpp \
    ProjectWriter.cpp \
    Shape.cpp \
    SourceGLCanvas.cpp \
    UidAllocator.cpp \
    Util.cpp \
    main.cpp

QT      += gui opengl xml
RESOURCES = libremapping.qrc

TRANSLATIONS = libremapping_fr.ts

include(contrib/qtpropertybrowser/src/qtpropertybrowser.pri)

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# mac
macx:LIBS += -framework OpenGL -framework GLUT
macx:QMAKE_CXXFLAGS += -D__MACOSX_CORE__

# not mac
!macx:LIBS    += -lglut -lGLU
