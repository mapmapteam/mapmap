QT += testlib
QT += core gui widgets opengl openglwidgets multimedia multimediawidgets network

CONFIG += c++11 console testcase
CONFIG -= app_bundle

TEMPLATE = app
TARGET = mapmap_tests

DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB QT_MESSAGELOGCONTEXT
unix:!macx: DEFINES += UNIX
win32: LIBS += -lopengl32

CORE  = $$PWD/../src/core
SHAPE = $$PWD/../src/shape

INCLUDEPATH += $$CORE $$SHAPE

# Production sources under test plus the minimal set of dependencies they
# need to link. We deliberately avoid pulling in the whole core/shape .pri
# files because some core sources (Commands, Paint, ...) depend on the GUI
# and multimedia back-ends, which are out of scope for these unit tests.
HEADERS += \
    $$CORE/MM.h \
    $$CORE/Maths.h \
    $$CORE/Util.h \
    $$CORE/Serializable.h \
    $$CORE/ProjectLabels.h \
    $$CORE/UidAllocator.h \
    $$SHAPE/Shape.h \
    $$SHAPE/Polygon.h \
    $$SHAPE/Triangle.h \
    $$SHAPE/Quad.h \
    $$SHAPE/Mesh.h \
    $$SHAPE/Ellipse.h \
    $$SHAPE/Shapes.h \
    TestMaths.h \
    TestUtil.h \
    TestUidAllocator.h \
    TestShape.h

SOURCES += \
    $$CORE/MM.cpp \
    $$CORE/Util.cpp \
    $$CORE/Serializable.cpp \
    $$CORE/ProjectLabels.cpp \
    $$CORE/UidAllocator.cpp \
    $$SHAPE/Shape.cpp \
    $$SHAPE/Polygon.cpp \
    $$SHAPE/Mesh.cpp \
    $$SHAPE/Ellipse.cpp \
    main.cpp \
    TestMaths.cpp \
    TestUtil.cpp \
    TestUidAllocator.cpp \
    TestShape.cpp
