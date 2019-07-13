#!/bin/sh
cd $(dirname $0)
cd ..
cd src/mapmap
set -o verbose
# Convert markdown file to html
markdown NEWS > docs/informations/CHANGELOG.html
