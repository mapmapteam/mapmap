
include(../src.pri)

HEADERS += $$PWD/AboutDialog.h \
    $$PWD/ConsoleWindow.h \
    $$PWD/GuiForward.h \
    $$PWD/HandleModeKeys.h \
    $$PWD/MainWindow.h \
    $$PWD/MapperGLCanvas.h \
    $$PWD/MapperGLCanvasToolbar.h \
    $$PWD/LayerGui.h \
    $$PWD/LayerItemDelegate.h \
    $$PWD/LayerListModel.h \
    $$PWD/OutputGLCanvas.h \
    $$PWD/OutputGLWindow.h \
    $$PWD/OutputResolutionEditor.h \
    $$PWD/SourceGui.h \
    $$PWD/PreferenceDialog.h \
    $$PWD/ShapeControlPainter.h \
    $$PWD/ShapeGraphicsItem.h \
    $$PWD/ShortcutWindow.h \
    $$PWD/WelcomeDialog.h \
    $$PWD/inspector/ColorPicker.h \
    $$PWD/inspector/Inspector.h \
    $$PWD/inspector/SettingControl.h \
    $$PWD/inspector/SettingFormat.h

SOURCES += $$PWD/AboutDialog.cpp \
    $$PWD/ConsoleWindow.cpp \
    $$PWD/HandleModeKeys.cpp \
    $$PWD/MainWindow.cpp \
    $$PWD/MapperGLCanvas.cpp \
    $$PWD/MapperGLCanvasToolbar.cpp \
    $$PWD/LayerGui.cpp \
    $$PWD/LayerItemDelegate.cpp \
    $$PWD/LayerListModel.cpp \
    $$PWD/OutputGLCanvas.cpp \
    $$PWD/OutputGLWindow.cpp \
    $$PWD/OutputResolutionEditor.cpp \
    $$PWD/SourceGui.cpp \
    $$PWD/PreferenceDialog.cpp \
    $$PWD/ShapeControlPainter.cpp \
    $$PWD/ShapeGraphicsItem.cpp \
    $$PWD/ShortcutWindow.cpp \
    $$PWD/WelcomeDialog.cpp \
    $$PWD/inspector/ColorPicker.cpp \
    $$PWD/inspector/Inspector.cpp \
    $$PWD/inspector/SettingControl.cpp \
    $$PWD/inspector/SettingFormat.cpp

# Syphon source picker, and hiding the mouse cursor (macOS-only).
macx {
    HEADERS += $$PWD/SyphonServerDialog.h $$PWD/MacCursor.h
    SOURCES += $$PWD/SyphonServerDialog.cpp
    OBJECTIVE_SOURCES += $$PWD/MacCursor.mm
}
