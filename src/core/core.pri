
include(../src.pri)

HEADERS += $$PWD/Commands.h \
    $$PWD/CameraImpl.h \
    $$PWD/CameraSurface.h \
    $$PWD/Element.h \
    $$PWD/Mapping.h \
    $$PWD/MappingManager.h \
    $$PWD/Maths.h \
    $$PWD/MetaObjectRegistry.h \
    $$PWD/MM.h \
    $$PWD/Paint.h \
    $$PWD/ProjectLabels.h \
    $$PWD/ProjectReader.h \
    $$PWD/ProjectWriter.h \
    $$PWD/Serializable.h \
    $$PWD/UidAllocator.h \
    $$PWD/VideoImpl.h \
    $$PWD/VideoShmSrcImpl.h \
    $$PWD/VideoUriDecodeBinImpl.h \
    $$PWD/VideoV4l2SrcImpl.h \
    $$PWD/Util.h

SOURCES += $$PWD/Commands.cpp \
    $$PWD/CameraImpl.cpp \
    $$PWD/CameraSurface.cpp \
    $$PWD/Element.cpp \
    $$PWD/Mapping.cpp \
    $$PWD/MappingManager.cpp \
    $$PWD/MetaObjectRegistry.cpp \
    $$PWD/MM.cpp \
    $$PWD/Paint.cpp \
    $$PWD/ProjectLabels.cpp \
    $$PWD/ProjectReader.cpp \
    $$PWD/ProjectWriter.cpp \
    $$PWD/Serializable.cpp \
    $$PWD/UidAllocator.cpp \
    $$PWD/VideoImpl.cpp \
    $$PWD/VideoShmSrcImpl.cpp \
    $$PWD/VideoUriDecodeBinImpl.cpp \
    $$PWD/VideoV4l2SrcImpl.cpp \
    $$PWD/Util.cpp
