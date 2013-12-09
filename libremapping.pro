CONFIG  += qt debug
TEMPLATE = app
HEADERS  = MainWindow.h Util.h MapperGLCanvas.h SourceGLCanvas.h DestinationGLCanvas.h Mapper.h Mapping.h Shape.h Paint.h MappingManager.h ProjectWriter.h NameAllocator.h
SOURCES  = main.cpp MainWindow.cpp Util.cpp Mapper.cpp MapperGLCanvas.cpp SourceGLCanvas.cpp DestinationGLCanvas.cpp MappingManager.cpp ProjectWriter.cpp NameAllocator.cpp
QT      += gui opengl xml
RESOURCES = libremapping.qrc

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

# mac
macx:LIBS += -framework OpenGL -framework GLUT
macx:QMAKE_CXXFLAGS += -D__MACOSX_CORE__

# not mac
!macx:LIBS    += -lglut -lGLU
