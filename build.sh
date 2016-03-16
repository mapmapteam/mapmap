#!/bin/bash
# Install qt4-default qt4-qmake
# On Mac, install it from http://qt-project.org/downloads
# set -o verbose

qtversion=5.4

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
    install_name_tool -change $qtdir/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
    install_name_tool -change $qtdir/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
    install_name_tool -change $qtdir/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
    install_name_tool -change $qtdir/lib/QtXml.framework/Versions/5/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/5/QtXml $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
    install_name_tool -change $qtdir/lib/QtOpenGL.framework/Versions/5/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/5/QtOpenGL $appdir/Contents/PlugIns/platforms/libqcocoa.dylib
}

unamestr=$(uname)

if [[ $unamestr == "Darwin" ]]; then
    #MAKE_CFLAGS_X86_64+="-Xarch_x86_64 -mmacosx-version-min=10.7"
    #QMAKE_CFLAGS_PPC_64+="-Xarch_ppc64 -mmacosx-version-min=10.7"
    #export MAKE_CFLAGS_X86_64
    #export QMAKE_CFLAGS_PPC_64
    export QMAKESPEC=macx-g++
    #export QMAKESPEC=macx-xcode
    PATH=$PATH:~/Qt${qtversion}/${qtversion}/clang_64/bin
    qmake5=~/Qt/${qtversion}/clang_64/bin/qmake
    # $qmake5 -spec macx-llvm

    # XXX
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-llvm
    #$qmake5 -config release -spec macx-llvm
    #$qmake5 -config debug -spec macx-g++
    $qmake5 -config release -spec macx-g++

    make
    macdeployqt MapMap.app
    #cp -R /Library/Frameworks/GStreamer.framework ./MapMap.app/Contents/Frameworks/
    # do_fix_qt_plugins_in_app
    do_create_dmg
elif [[ $unamestr == "Linux" ]]; then
    qmake
    make
fi

