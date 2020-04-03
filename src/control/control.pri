include(../src.pri)

HEADERS += $$PWD/ConcurrentQueue.h \
    $$PWD/OscInterface.h

SOURCES += $$PWD/OscInterface.cpp

# OSC support:
INCLUDEPATH += $$PWD/qosc
INCLUDEPATH += $$PWD/qosc/contrib/packosc

SOURCES += \
    $$PWD/qosc/contrib/oscpack/OscOutboundPacketStream.cpp \
    $$PWD/qosc/contrib/oscpack/OscPrintReceivedElements.cpp \
    $$PWD/qosc/contrib/oscpack/OscReceivedElements.cpp \
    $$PWD/qosc/contrib/oscpack/OscTypes.cpp \
    $$PWD/qosc/oscreceiver.cpp \
    $$PWD/qosc/oscsender.cpp

HEADERS += \
    $$PWD/qosc/contrib/oscpack/MessageMappingOscPacketListener.h \
    $$PWD/qosc/contrib/oscpack/OscException.h \
    $$PWD/qosc/contrib/oscpack/OscHostEndianness.h \
    $$PWD/qosc/contrib/oscpack/OscOutboundPacketStream.h \
    $$PWD/qosc/contrib/oscpack/OscPacketListener.h \
    $$PWD/qosc/contrib/oscpack/OscPrintReceivedElements.h \
    $$PWD/qosc/contrib/oscpack/OscReceivedElements.h \
    $$PWD/qosc/contrib/oscpack/OscTypes.h \
    $$PWD/qosc/oscreceiver.h \
    $$PWD/qosc/oscsender.h

