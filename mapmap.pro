CONFIG += qt
CONFIG += debug
CONFIG += c++11

TEMPLATE = app

# Always use major.minor.micro version number format
VERSION = 0.6.3
TARGET = mapmap

DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB QT_MESSAGELOGCONTEXT

include(src/core/core.pri)
include(src/shape/shape.pri)
include(src/gui/gui.pri)
include(src/control/control.pri)
include(src/app/app.pri)

TRANSLATIONS += \
    translations/mapmap_en.ts \
    translations/mapmap_es.ts \
    translations/mapmap_fr.ts \
    translations/mapmap_zh_CN.ts \
    translations/mapmap_zh_TW.ts

RESOURCES = \
    main.qrc \
    translations/translation.qrc \
    docs/documentation.qrc \
    resources/interface.qrc \
    main.qrc # Main resource file

# Manage lrelease (for translations)
isEmpty(QMAKE_LRELEASE) {
    win32: {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    }
    !win32:
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm
PRE_TARGETDEPS += compiler_updateqm_make_all
system($$QMAKE_LRELEASE mapmap.pro) # Run lrelease

## Add the docs target:
#docs.depends = $(HEADERS) $(SOURCES)
#docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
#QMAKE_EXTRA_TARGETS += docs

# Linux-specific:
unix:!macx {
    QMAKE_CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0
    QMAKE_CXXFLAGS += -Wno-expansion-to-defined
}

CONFIG -= qtquickcompiler

unix:macx {
    CONFIG += sdk_no_version_check
}

# Adds the tarball target
tarball.target = mapmap-$${VERSION}.tar.gz
tarball.commands = git archive --format=tar.gz -9 --prefix=mapmap-$${VERSION}/ --output=mapmap-$${VERSION}.tar.gz HEAD
tarball.depends = .git
QMAKE_EXTRA_TARGETS += tarball

# Show various messages
message("MapMap version: $${VERSION}")
# message("Qt version: $$[QT_VERSION]")
# message("LIBS: $${LIBS}")
# message("PKGCONFIG: $${PKGCONFIG}")
# message("The project contains the following files: $${SOURCES} $${HEADERS}}")
# message("To create a tarball, run `make tarball`")
# message("To build the documentation, run `make docs`")
