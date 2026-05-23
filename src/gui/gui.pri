
include(../src.pri)

include($$PWD/contrib/qtpropertybrowser/src/qtpropertybrowser.pri)
include($$PWD/contrib/qtpropertybrowser-extension/qtpropertybrowser-extension.pri)

HEADERS += $$PWD/AboutDialog.h \
    $$PWD/ConsoleWindow.h \
    $$PWD/GuiForward.h \
    $$PWD/MainWindow.h \
    $$PWD/MapperGLCanvas.h \
    $$PWD/MapperGLCanvasToolbar.h \
    $$PWD/LayerGui.h \
    $$PWD/LayerItemDelegate.h \
    $$PWD/LayerListModel.h \
    $$PWD/OutputGLCanvas.h \
    $$PWD/OutputGLWindow.h \
    $$PWD/SourceGui.h \
    $$PWD/PreferenceDialog.h \
    $$PWD/ShapeControlPainter.h \
    $$PWD/ShapeGraphicsItem.h \
    $$PWD/ShortcutWindow.h

SOURCES += $$PWD/AboutDialog.cpp \
    $$PWD/ConsoleWindow.cpp \
    $$PWD/MainWindow.cpp \
    $$PWD/MapperGLCanvas.cpp \
    $$PWD/MapperGLCanvasToolbar.cpp \
    $$PWD/LayerGui.cpp \
    $$PWD/LayerItemDelegate.cpp \
    $$PWD/LayerListModel.cpp \
    $$PWD/OutputGLCanvas.cpp \
    $$PWD/OutputGLWindow.cpp \
    $$PWD/SourceGui.cpp \
    $$PWD/PreferenceDialog.cpp \
    $$PWD/ShapeControlPainter.cpp \
    $$PWD/ShapeGraphicsItem.cpp \
    $$PWD/ShortcutWindow.cpp
