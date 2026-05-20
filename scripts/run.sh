#!/bin/bash
export LANG=C

if [[ `uname` == 'Linux' ]]; then
  ./src/mapmap/mapmap
else #macOS
  ./src/mapmap/mapmap.app/Contents/MacOS/mapmap
fi


