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

# Windows-specific:
win32 {
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
    -lopengl32

  QMAKE_CXXFLAGS += -D_USE_MATH_DEFINES
}


