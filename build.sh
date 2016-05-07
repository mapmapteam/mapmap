#!/bin/bash
# Install qt4-default qt4-qmake
# On Mac, install it from http://qt-project.org/downloads
# set -o verbose

QT_VERSION=5.5

do_create_dmg() {
    if [ -f DMGVERSION.txt ]
    then
        echo "Using DMGVERSION.txt"
        cat DMGVERSION.txt
    else
        echo 1 > DMGVERSION.txt
    fi
    VERSION=$(cat VERSION.txt)
    DMGVERSION=$(cat DMGVERSION.txt)
    DMGDIR=MapMap-${VERSION}-${DMGVERSION}
    echo "Creating directory ${DMGDIR}"
    rm -rf $DMGDIR
    mkdir -p $DMGDIR
    cp -R MapMap.app ${DMGDIR}
    cp README ${DMGDIR}/README.txt
    cp NEWS ${DMGDIR}/NEWS.txt
    hdiutil create \
        -volname ${DMGDIR} \
        -srcfolder ${DMGDIR} \
        -ov -format UDZO \
        ${DMGDIR}.dmg
}

do_fix_qt_plugins_in_app() {
    appdir=./MapMap.app
    qtdir=~/Qt/${qtversion}/clang_64
    # install libqcocoa library
    mkdir -p $appdir/Contents/PlugIns/platforms
    cp $qtdir/plugins/platforms/libqcocoa.dylib $appdir/Contents/PlugIns/platforms
    # fix its identity and references to others
    install_name_tool -id @executable_path/../PlugIns/platforms/libqcocoa.dylib $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
    for qtframework in QtWidgets, QtGui, QtCore, QtXml, QtOpenGL; do
      framework = "$qtframework.framework/Versions/5/$qtframework"
      echo "Processing $framework"
      install_name_tool -change $qtdir/lib/$framework @executable_path/../Frameworks/$framework
    done
}

unamestr=$(uname)

if [[ $unamestr == "Darwin" ]]; then
    #MAKE_CFLAGS_X86_64+="-Xarch_x86_64 -mmacosx-version-min=10.7"
    #QMAKE_CFLAGS_PPC_64+="-Xarch_ppc64 -mmacosx-version-min=10.7"
    #export MAKE_CFLAGS_X86_64
    #export QMAKE_CFLAGS_PPC_64
    #export QMAKESPEC=macx-g++
    #export QMAKESPEC=macx-xcode
    QT_BIN_DIR=~/Qt/${QT_VERSION}/clang_64/bin/
    PATH=$PATH:${QT_BIN_DIR}
    APP="MapMap.app"
    gstreamer="GStreamer.framework/Versions/1.0/lib/GStreamer"

    # XXX
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-llvm
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-g++
    #qmake -config release -spec macx-g++

    # build program
    echo "Building program ..."
    ${QT_BIN_DIR}/qmake -config release
    make -j4
    
    # Bundle Qt frameworks in app using macdeployqt
    echo "Bundling Qt ..."
    ${QT_BIN_DIR}/macdeployqt $APP
    
    # Bundle GStreamer framework in app
    #echo "Bundling GStreamer ..."
    cp -r /Library/Frameworks/GStreamer.framework ${APP}/Contents/Frameworks/
    osxrelocator ${APP}/Contents/Frameworks/GStreamer.framework/Versions/Current/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
    osxrelocator ${APP}/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
    osxrelocator ${APP}/Contents/Frameworks/GStreamer.framework/Versions/Current/bin /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
    osxrelocator ${APP}/Contents/MacOS /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r

    #install_name_tool -id @executable_path/../Frameworks/${gstreamer} ${APP}/Contents/Frameworks/${gstreamer}
    #install_name_tool -change /Library/Frameworks/${gstreamer} @executable_path/../Frameworks/${gstreamer} ${APP}/Contents/MacOs/MapMap
    
    # do_fix_qt_plugins_in_app
    #echo "Creating DMG ..."
    #do_create_dmg
elif [[ $unamestr == "Linux" ]]; then
    qmake
    make -j4
fi

