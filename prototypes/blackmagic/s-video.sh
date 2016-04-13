#!/bin/bash
# Using the Blackmagic Intensity Shuttle USB 3.0
#
# Download and build: you can download source driver from
# https://www.blackmagicdesign.com/support - compile a driver from this site
# for a decklinksink blackmagic card
# Direct link:
# https://www.blackmagicdesign.com/support/download/f73ee77fdd384b5286fe86a46d20d045/Linux

GST_DEBUG=2 \
LANG=C \
gst-launch -v decklinksrc mode=3 connection=5 ! \
    ffmpegcolorspace ! \
    xvimagesink sync=false

