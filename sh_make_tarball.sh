#!/bin/bash

MAPMAP_VERSION=0.1.0

git archive --format=tar.gz -9 --prefix=mapmap-${MAPMAP_VERSION}/ --output=mapmap-${MAPMAP_VERSION}.tar.gz HEAD

