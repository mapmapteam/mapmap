
include(../src.pri)

HEADERS += $$PWD/MainApplication.h

SOURCES += $$PWD/MainApplication.cpp \
    $$PWD/main.cpp

MOC_DIR = $$PWD/mocs
OBJECTS_DIR = $$PWD/objs
RCC_DIR = $$PWD/qrc
