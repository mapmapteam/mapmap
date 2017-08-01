#!/bin/bash
# Install qt4-default qt4-qmake
# On Mac, install it from http://qt-project.org/downloads

unamestr=$(uname)
cd $(dirname $0)
cd src/mapmap

if [[ $unamestr == "Darwin" ]]; then
    MAKE_CFLAGS_X86_64+="-Xarch_x86_64 -mmacosx-version-min=10.7"
    QMAKE_CFLAGS_PPC_64+="-Xarch_ppc64 -mmacosx-version-min=10.7"
    export MAKE_CFLAGS_X86_64
    export QMAKE_CFLAGS_PPC_64
    export QMAKESPEC=macx-g++
    #export QMAKESPEC=macx-xcode
    PATH=$PATH:~/Qt5.2.1/5.2.1/clang_64/bin
    qmake5=~/Qt5.2.1/5.2.1/clang_64/bin/qmake
    $qmake5 -spec macx-llvm
    lrelease mapmap_fr.ts
    macdeployqt=~/Qt5.2.1/5.2.1/clang_64/bin/macdeployqt
    $macdeployqt mapmap.app -dmg
elif [[ $unamestr == "Linux" ]]; then
    qmake-qt4
    make
    lrelease translations/mapmap_fr.ts
    lrelease translations/mapmap_en.ts
fi

