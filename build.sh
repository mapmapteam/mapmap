#!/bin/bash
# Install qt4-default qt4-qmake
# On Mac, install it from http://qt-project.org/downloads
# set -o verbose

qtversion=5.8

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
    qtbindir=~/Qt/${qtversion}/clang_64/bin/
    PATH=$PATH:${qtbindir}
    gstreamer="GStreamer.framework/Versions/1.0/lib/GStreamer"
    app="MapMap.app"

    # XXX
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-llvm
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-g++
    #qmake -config release -spec macx-g++

    # build program
    echo "Building program ..."
    qmake -config release
    make -j4
    
    # Bundle Qt frameworks in app using macdeployqt
    echo "Bundling Qt ..."
    macdeployqt $app
    
    # Bundle GStreamer framework in app
    #echo "Bundling GStreamer ..."
    #cp -R /Library/Frameworks/GStreamer.framework ${app}/Contents/Frameworks/
    #install_name_tool -id @executable_path/../Frameworks/${gstreamer} ${app}/Contents/Frameworks/${gstreamer}
    #install_name_tool -change /Library/Frameworks/${gstreamer} @executable_path/../Frameworks/${gstreamer} ${app}/Contents/MacOs/MapMap
    
    # do_fix_qt_plugins_in_app
    echo "Creating DMG ..."
    do_create_dmg
elif [[ $unamestr == "Linux" ]]; then
    qmake
    make -j4
fi

