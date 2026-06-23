
include(../src.pri)

HEADERS += $$PWD/Commands.h \
    $$PWD/CameraImpl.h \
    $$PWD/CameraSurface.h \
    $$PWD/Element.h \
    $$PWD/Layer.h \
    $$PWD/MappingManager.h \
    $$PWD/Maths.h \
    $$PWD/MetaObjectRegistry.h \
    $$PWD/MM.h \
    $$PWD/Source.h \
    $$PWD/ProjectLabels.h \
    $$PWD/ProjectReader.h \
    $$PWD/ProjectWriter.h \
    $$PWD/Serializable.h \
    $$PWD/UidAllocator.h \
    $$PWD/VideoImpl.h \
    $$PWD/VideoPlayerImpl.h \
    $$PWD/Util.h

SOURCES += $$PWD/Commands.cpp \
    $$PWD/CameraImpl.cpp \
    $$PWD/CameraSurface.cpp \
    $$PWD/Element.cpp \
    $$PWD/Layer.cpp \
    $$PWD/MappingManager.cpp \
    $$PWD/MetaObjectRegistry.cpp \
    $$PWD/MM.cpp \
    $$PWD/Source.cpp \
    $$PWD/ProjectLabels.cpp \
    $$PWD/ProjectReader.cpp \
    $$PWD/ProjectWriter.cpp \
    $$PWD/Serializable.cpp \
    $$PWD/UidAllocator.cpp \
    $$PWD/VideoImpl.cpp \
    $$PWD/VideoPlayerImpl.cpp \
    $$PWD/Util.cpp

# Syphon (macOS-only) inter-application video sharing.
macx {
    HEADERS += $$PWD/Syphon.h
    OBJECTIVE_SOURCES += $$PWD/SyphonImpl.mm
}
