#!/bin/bash
# Using the Blackmagic Intensity Pro

gst-launch -v decklinksrc mode=3 connection=4 ! \
    ffmpegcolorspace ! \
    xvimagesink sync=false

