#!/bin/bash
unamestr=$(uname)
cd $(dirname $0)
cd src/mapmap

if [[ $unamestr == "Darwin" ]]; then
    otool -L mapmap.app/Contents/MacOS/mapmap
elif [[ $unamestr == "Linux" ]]; then
    ldd mapmap
fi
