#!/bin/bash
# Using the Blackmagic Intensity Pro

gst-launch -v decklinksrc mode=18 connection=0 ! ffmpegcolorspace ! xvimagesink sync=false

