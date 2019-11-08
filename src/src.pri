QT += gui opengl xml core network
greaterThan(QT_MAJOR_VERSION, 4) {
  QT -= gui # using widgets instead gui in Qt5
  QT += widgets multimedia
}

#Includes common configuration for all subdirectory .pro files.
INCLUDEPATH += $$PWD/core \
    $$PWD/shape \
    $$PWD/gui \
    $$PWD/control \
    $$PWD/app

#Linux-specific:
unix:!macx {
  DEFINES += UNIX
  CONFIG += link_pkgconfig
  INCLUDE_PATH +=
  PKGCONFIG += \
    gstreamer-1.0 gstreamer-base-1.0 gstreamer-app-1.0 gstreamer-pbutils-1.0 \
    liblo \
    gl x11
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-result -Wno-unused-parameter \
                            -Wno-unused-variable -Wno-switch -Wno-comment \
                            -Wno-unused-but-set-variable
  QMAKE_CXXFLAGS += -DHAVE_OSC
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
  TARGET = ../../../MapMap/MapMap # Just for release
  GST_HOME = $$quote($$(GSTREAMER_1_0_ROOT_X86))
  isEmpty(GST_HOME) {
    message(\"GSTREAMER_1_0_ROOT_X86\" not detected ...)
  }
  else {
    message(\"GSTREAMER_1_0_ROOT_X86\" detected in \"$${GST_HOME}\")
  }

  INCLUDEPATH += $${GST_HOME}/lib/gstreamer-1.0/include \
    $${GST_HOME}/include/glib-2.0 \
    $${GST_HOME}/lib/glib-2.0/include \
    $${GST_HOME}/include/gstreamer-1.0

  LIBS += $${GST_HOME}/lib/gstapp-1.0.lib \
    $${GST_HOME}/lib/gstbase-1.0.lib \
    $${GST_HOME}/lib/gstpbutils-1.0.lib \
    $${GST_HOME}/lib/gstreamer-1.0.lib \
    $${GST_HOME}/lib/gobject-2.0.lib \
    $${GST_HOME}/lib/glib-2.0.lib \
    $${GST_HOME}/lib/gstaudio-1.0.lib \
    $${GST_HOME}/lib/gstvideo-1.0.lib \
    -lopengl32

  CONFIG -= debug
  CONFIG += release

  RC_FILE = resources/windows_resource.rc
  QMAKE_CXXFLAGS += -D_USE_MATH_DEFINES
}
