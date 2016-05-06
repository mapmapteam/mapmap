#!/bin/bash
# Using the Blackmagic Intensity Shuttle USB 3.0
#
# Download and build: you can download source driver from
# https://www.blackmagicdesign.com/support - compile a driver from this site
# for a decklinksink blackmagic card
# Direct link:
# https://www.blackmagicdesign.com/support/download/f73ee77fdd384b5286fe86a46d20d045/Linux

GST_DEBUG=4 \
LANG=C \
gst-launch-1.0 -v decklinksrc mode=18 connection=1 ! \
    videoconvert ! \
    queue ! \
    xvimagesink
# sync=false
