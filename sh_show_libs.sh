#!/bin/bash
unamestr=$(uname)

if [[ $unamestr == "Darwin" ]]; then
    otool -L mapmap.app/Contents/MacOS/mapmap
elif [[ $unamestr == "Linux" ]]; then
    ldd mapmap
fi
