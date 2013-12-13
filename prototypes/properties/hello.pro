CONFIG  += qt debug
TEMPLATE = app
HEADERS  = 
SOURCES  = main.cpp
QT      += gui
RESOURCES = 

docs.depends = $(HEADERS) $(SOURCES)
docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
QMAKE_EXTRA_TARGETS += docs

