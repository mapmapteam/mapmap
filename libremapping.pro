CONFIG  += qt debug
TEMPLATE = app
HEADERS  = MainWindow.h Util.h MapperGLCanvas.h SourceGLCanvas.h DestinationGLCanvas.h Mapper.h Mapping.h Shape.h Paint.h Layer.h MappingManager.h
SOURCES  = main.cpp MainWindow.cpp Util.cpp Mapper.cpp MapperGLCanvas.cpp SourceGLCanvas.cpp DestinationGLCanvas.cpp Layer.cpp MappingManager.cpp
QT      += gui opengl
LIBS    += -lglut -lGLU
RESOURCES = libremapping.qrc

include(qtpropertybrowser/src/qtpropertybrowser.pri)

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

