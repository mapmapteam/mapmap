#!/bin/bash
if [[ $# -eq 0 ]] ; then
  echo "This script is useful to find and replace a path in a MapMap project file."
  echo "Usage: $0 /old/path /new/path project_file.mmp"
  exit 0
fi

OLDPATH=$1
NEWPATH=$2
PROJECTFILE=$3

sed -i "s|${OLDPATH}|${NEWPATH}|g" ${PROJECTFILE}

