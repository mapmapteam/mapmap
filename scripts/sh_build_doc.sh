#!/bin/bash
# doxygen
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ..

doxygen docs/Doxyfile
