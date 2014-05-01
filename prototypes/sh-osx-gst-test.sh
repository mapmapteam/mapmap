#!/bin/bash
set -o verbose
#export GST_PLUGIN_PATH=/Library/Frameworks/GStreamer.framework/Libraries
export LANG=C
# export GST_DEBUG=3
#FILEPATH=/Users/aalex/Desktop/mapmap-videos/BonneFete.mov
FILEPATH=/Users/aalex/Downloads/sintel_trailer-480p.ogv

gst-launch-0.10 uridecodebin uri=file://${FILEPATH} ! ffmpegcolorspace ! autovideosink

