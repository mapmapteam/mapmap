CONFIG += c++11

# Always use major.minor.micro version number format
VERSION = 0.5.1
TARGET = mapmap

DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB

include(src/core/core.pri)
include(src/shape/shape.pri)
include(src/gui/gui.pri)
include(src/control/control.pri)
include(src/app/app.pri)

TRANSLATIONS = \
    translations/mapmap_en.ts \
    translations/mapmap_fr.ts
RESOURCES = \
    translations/translation.qrc \
    docs/documentation.qrc \
    resources/interface.qrc

# Manage lrelease (for translations)
isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
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

 Linux-specific:
unix:!macx {
  mapmapfile.files = mapmap
  mapmapfile.path = /usr/bin
  INSTALLS += mapmapfile
  desktopfile.files = resources/texts/mapmap.desktop
  desktopfile.path = /usr/share/applications
  INSTALLS += desktopfile
  iconfile.files = resources/images/logo/mapmap.svg
  iconfile.path = /usr/share/icons/hicolor/scalable/apps
  INSTALLS += iconfile
  mimetypesfile.files = resources/texts/mapmap.xml
  mimetypesfile.path = /usr/share/mime/packages
  INSTALLS += mimetypesfile

  # REQUIRES ROOT PRIVILEDGES: (does not comply to the standards of Debian)
  # -------------------------
  # updatemimetypes.path = /usr/share/mime/packages
  # updatemimetypes.commands = update-mime-database /usr/share/mime
  # INSTALLS += updatemimetypes
  # updatemimeappdefault.path = /usr/share/applications
  # updatemimeappdefault.commands='grep mapmap.desktop /usr/share/applications/defaults.list >/dev/null|| sudo echo "application/mapmap=mapmap.desktop;" >> /usr/share/applications/defaults.list'
  # INSTALLS += updatemimeappdefault
  # -------------------------

  # Add the docs target:
  docs.depends = $(HEADERS) $(SOURCES)
  docs.commands = (cat Doxyfile; echo "INPUT = $?") | doxygen -
  QMAKE_EXTRA_TARGETS += docs
}

# macOS-specific:
macx {
  TARGET = MapMap
  DEFINES += MACOSX
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
  QMAKE_CXXFLAGS += -stdlib=libc++
  INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Versions/1.0/Headers
  LIBS += -F /Library/Frameworks/ -framework GStreamer
  LIBS += -framework OpenGL -framework GLUT
  # With Xcode Tools > 1.5, to reduce the size of your binary even more:
  # LIBS += -dead_strip
  # This tells qmake not to put the executable inside a bundle.
  # just for reference. Do not uncomment.
  # CONFIG-=app_bundle

  # For OSC support: (if pkg-config was installed)
  # CONFIG += link_pkgconfig
  # PKGCONFIG += lo

  # FIXME: No OSC for now:
  # LIBS += -L/usr/local/lib -llo
  # INCLUDEPATH += /usr/local/include
  # QMAKE_CXXFLAGS += -DHAVE_OSC
}

# Windows-specific:
win32 {
  DEFINES += WIN32
  TARGET = Mapmap
  GST_HOME = $$quote($$(GSTREAMER_1_0_ROOT_X86))
  isEmpty(GST_HOME) {
    message(\"GSTREAMER_1_0_ROOT_X86\" not detected ...)
  }
  else {
    message(\"GSTREAMER_1_0_ROOT_X86\" detected in \"$${GST_HOME}\")
  }
  #  DESTDIR = ../../Mapmap # Just for packaging

#  INCLUDEPATH += $${GST_HOME}/lib/gstreamer-1.0/include \
#    $${GST_HOME}/include/glib-2.0 \
#    $${GST_HOME}/lib/glib-2.0/include \
#    $${GST_HOME}/include/gstreamer-1.0

#  LIBS += $${GST_HOME}/lib/gstapp-1.0.lib \
#    $${GST_HOME}/lib/gstbase-1.0.lib \
#    $${GST_HOME}/lib/gstpbutils-1.0.lib \
#    $${GST_HOME}/lib/gstreamer-1.0.lib \
#    $${GST_HOME}/lib/gobject-2.0.lib \
#    $${GST_HOME}/lib/glib-2.0.lib \
#    -lopengl32

  CONFIG += release

  RC_FILE = resources/windows_resource.rc
  QMAKE_CXXFLAGS += -D_USE_MATH_DEFINES
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
