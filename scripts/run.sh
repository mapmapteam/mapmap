#!/bin/bash
#export GST_PLUGIN_PATH=/Library/Frameworks/GStreamer.framework/Libraries
#export GST_DEBUG=2
export LANG=C

if [[ `uname` == 'Linux' ]]; then
  ./src/mapmap/mapmap
else #macOS
  ./src/mapmap/mapmap.app/Contents/MacOS/mapmap
fi


