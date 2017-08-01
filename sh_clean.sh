#!/bin/bash
cd $(dirname $0)
cd src/mapmap
make clean 
rm -rf MapMap.app mapmap.pro.user
