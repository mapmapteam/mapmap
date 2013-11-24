CONFIG  += qt debug
TEMPLATE = app
HEADERS  = Common.h SourceGLCanvas.h DestinationGLCanvas.h MapperGLCanvas.h
SOURCES  = main.cpp Common.cpp SourceGLCanvas.cpp DestinationGLCanvas.cpp MapperGLCanvas.cpp
QT      += gui opengl
LIBS    += -lSOIL -lglut

