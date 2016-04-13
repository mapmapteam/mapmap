#!/bin/bash
CFLAGS=`pkg-config --cflags cairo gstreamer-plugins-base-1.0 gstreamer-video-1.0`
LDFLAGS=`pkg-config --libs cairo gstreamer-plugins-base-1.0 gstreamer-video-1.0`

gcc -Wall ${LIBS} ${LDFLAGS} -o cairo-proto cairo-proto.c
