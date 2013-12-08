CONFIG  += qt debug
TEMPLATE = app
HEADERS  = MainWindow.h Util.h MapperGLCanvas.h SourceGLCanvas.h DestinationGLCanvas.h Mapper.h Mapping.h Shape.h Paint.h Layer.h MappingManager.h
SOURCES  = main.cpp MainWindow.cpp Util.cpp Mapper.cpp MapperGLCanvas.cpp SourceGLCanvas.cpp DestinationGLCanvas.cpp Layer.cpp MappingManager.cpp
QT      += gui opengl
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
