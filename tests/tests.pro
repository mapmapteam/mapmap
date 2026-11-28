QT += testlib
QT += core gui widgets opengl openglwidgets multimedia multimediawidgets network

CONFIG += c++11 console testcase
CONFIG -= app_bundle

TEMPLATE = app
TARGET = mapmap_tests

DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB QT_MESSAGELOGCONTEXT
unix:!macx: DEFINES += UNIX
win32: LIBS += -lopengl32
macx: LIBS += -framework CoreFoundation
macx: DEFINES += GL_SILENCE_DEPRECATION

CORE    = $$PWD/../src/core
SHAPE   = $$PWD/../src/shape
CONTROL = $$PWD/../src/control
GUI     = $$PWD/../src/gui

INCLUDEPATH += $$CORE $$SHAPE $$CONTROL $$GUI $$GUI/inspector

# Production sources under test plus the minimal set of dependencies they
# need to link. We deliberately avoid pulling in the whole core/shape .pri
# files because some core sources (Commands, ...) depend on the GUI, which is
# out of scope for these unit tests. The model (sources, layers,
# MappingManager) and the project reader/writer have no GUI dependency.
HEADERS += \
    $$CORE/MM.h \
    $$CORE/Maths.h \
    $$CORE/Util.h \
    $$CORE/FrameStats.h \
    $$CORE/Serializable.h \
    $$CORE/Setting.h \
    $$CORE/SettingJson.h \
    $$CORE/ProjectLabels.h \
    $$CORE/UidAllocator.h \
    $$CORE/Element.h \
    $$CORE/MetaObjectRegistry.h \
    $$CORE/Source.h \
    $$SHAPE/Shape.h \
    $$SHAPE/Polygon.h \
    $$SHAPE/Triangle.h \
    $$SHAPE/Quad.h \
    $$SHAPE/Mesh.h \
    $$SHAPE/Ellipse.h \
    $$SHAPE/Shapes.h \
    $$SHAPE/ShapeHandles.h \
    $$GUI/HandleModeKeys.h \
    $$CONTROL/OscAction.h \
    $$CONTROL/RemoteSettingWrite.h \
    $$CORE/VideoImpl.h \
    $$CORE/VideoPlayerImpl.h \
    $$CORE/YuvTexture.h \
    $$CORE/CameraImpl.h \
    $$CORE/Layer.h \
    $$CORE/Link.h \
    $$CORE/MappingManager.h \
    $$CORE/ProjectReader.h \
    $$CORE/ProjectWriter.h \
    TestMaths.h \
    TestUtil.h \
    TestFrameStats.h \
    TestUidAllocator.h \
    TestShape.h \
    TestHandleModeKeys.h \
    TestOsc.h \
    TestFileVersion.h \
    TestVideo.h \
    TestCameraSource.h \
    TestMappingManager.h \
    TestProjectRoundTrip.h \
    TestSettings.h \
    TestSettingFormat.h \
    TestInspector.h \
    TestLinks.h

SOURCES += \
    $$CORE/MM.cpp \
    $$CORE/Util.cpp \
    $$CORE/FrameStats.cpp \
    $$CORE/Serializable.cpp \
    $$CORE/Setting.cpp \
    $$CORE/SettingJson.cpp \
    $$CORE/ProjectLabels.cpp \
    $$CORE/UidAllocator.cpp \
    $$CORE/Element.cpp \
    $$CORE/MetaObjectRegistry.cpp \
    $$CORE/Source.cpp \
    $$SHAPE/Shape.cpp \
    $$SHAPE/Polygon.cpp \
    $$SHAPE/Mesh.cpp \
    $$SHAPE/Ellipse.cpp \
    $$SHAPE/ShapeHandles.cpp \
    $$GUI/HandleModeKeys.cpp \
    $$CONTROL/OscAction.cpp \
    $$CONTROL/RemoteSettingWrite.cpp \
    $$CORE/VideoImpl.cpp \
    $$CORE/VideoPlayerImpl.cpp \
    $$CORE/YuvTexture.cpp \
    $$CORE/CameraImpl.cpp \
    $$CORE/Layer.cpp \
    $$CORE/Link.cpp \
    $$CORE/MappingManager.cpp \
    $$CORE/ProjectReader.cpp \
    $$CORE/ProjectWriter.cpp \
    $$CORE/MetaObjectRegistryBuiltins.cpp \
    main.cpp \
    TestMaths.cpp \
    TestUtil.cpp \
    TestFrameStats.cpp \
    TestUidAllocator.cpp \
    TestShape.cpp \
    TestHandleModeKeys.cpp \
    TestOsc.cpp \
    TestFileVersion.cpp \
    TestVideo.cpp \
    TestCameraSource.cpp \
    TestMappingManager.cpp \
    TestProjectRoundTrip.cpp \
    TestSettings.cpp \
    TestSettingFormat.cpp \
    TestInspector.cpp \
    TestLinks.cpp

# The inspector.
HEADERS += $$GUI/inspector/SettingFormat.h $$GUI/inspector/SettingControl.h \
    $$GUI/inspector/ColorPicker.h $$GUI/inspector/Inspector.h
SOURCES += $$GUI/inspector/SettingFormat.cpp $$GUI/inspector/SettingControl.cpp \
    $$GUI/inspector/ColorPicker.cpp $$GUI/inspector/Inspector.cpp

# Projects read by TestProjectRoundTrip.
DEFINES += MAPMAP_TEST_FIXTURES_DIR=\\\"$$PWD/fixtures\\\"
