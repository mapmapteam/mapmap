CONFIG  += qt debug
TEMPLATE = app
HEADERS  = Common.h Util.h SourceGLCanvas.h DestinationGLCanvas.h MapperGLCanvas.h Mapper.h Shape.h Paint.h
SOURCES  = main.cpp Common.cpp Util.cpp Mapper.cpp SourceGLCanvas.cpp DestinationGLCanvas.cpp MapperGLCanvas.cpp 
QT      += gui opengl
LIBS    += -lglut -lGLU

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

