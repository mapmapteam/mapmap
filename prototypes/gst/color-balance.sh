#!/bin/bash
# default:
CONTRAST=1.5
BRIGHTNESS=0.25
SATURATION=1.25
HUE=0.0

# blasted
# CONTRAST=2.0
# BRIGHTNESS=1.0
# SATURATION=2.0
# HUE=0.0

gst-launch-1.0 videotestsrc ! videobalance contrast=${CONTRAST} brightness=${BRIGHTNESS} saturation=${SATURATION} hue=${HUE} ! videoconvert ! autovideosink

# Valid value for each setting:
# 
# contrast: [ 0 ~ 2 ] (default = 1)
# brightness: [ -1~ 1] (default = 0)
# saturation: [ 0 ~ 2 ] (default = 1)
# hue: [ -1 ~ 1 ] (default = 0)
