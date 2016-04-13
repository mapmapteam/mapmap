#!/bin/bash
CFLAGS=`pkg-config --cflags cairo gstreamer-plugins-base-1.0 gstreamer-video-1.0 gstreamer-1.0`
LDFLAGS=`pkg-config --libs cairo gstreamer-plugins-base-1.0 gstreamer-video-1.0 gstreamer-1.0`

gcc -o cairo-proto -Wall cairo-proto.c ${LDFLAGS} ${CFLAGS}
