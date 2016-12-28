#!/bin/sh
set -o verbose
# Convert markdown file to html
markdown NEWS > docs/informations/CHANGELOG.html
