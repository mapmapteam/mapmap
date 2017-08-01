#!/bin/bash

MAPMAP_VERSION=$(cat VERSION.txt)

git archive --format=tar.gz -9 --prefix=mapmap-${MAPMAP_VERSION}/ --output=mapmap-${MAPMAP_VERSION}.tar.gz HEAD

