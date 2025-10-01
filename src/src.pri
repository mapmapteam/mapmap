QT += gui
QT += opengl
QT += xml
QT += core
QT += network
QT += multimedia

greaterThan(QT_MAJOR_VERSION, 4) {
  QT -= gui # using widgets instead gui in Qt5
  QT += widgets webenginewidgets
}

greaterThan(QT_MAJOR_VERSION, 5) {
  QT += multimediawidgets openglwidgets
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
    gl x11
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-result -Wno-unused-parameter \
                            -Wno-unused-variable -Wno-switch -Wno-comment \
                            -Wno-unused-but-set-variable
}

# macOS-specific:
macx {
  TARGET = MapMap
  DEFINES += MACOSX
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
  QMAKE_CXXFLAGS += -stdlib=libc++
  LIBS += -framework OpenGL -framework GLUT
  # With Xcode Tools > 1.5, to reduce the size of your binary even more:
  # LIBS += -dead_strip
  # This tells qmake not to put the executable inside a bundle.
  # just for reference. Do not uncomment.
  # CONFIG-=app_bundle
  ICON = resources/app_icons/mapmap.icns
}


# Windows-specific:
win32 {
  DEFINES += WIN32
  TARGET = ../../../MapMap/MapMap # Just for release
  
  LIBS += -lole32 -loleaut32 -lwinmm -lopengl32

  CONFIG -= debug
  CONFIG += release

  RC_FILE = resources/windows_resource.rc
  QMAKE_CXXFLAGS += -D_USE_MATH_DEFINES
}
