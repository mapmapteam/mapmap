#!/bin/bash
# libwxgtk2.8-dev
g++ main.cpp Common.cpp DestinationGLCanvas.cpp SourceGLCanvas.cpp MapperGLCanvas.cpp -o run `wx-config --libs --cxxflags --gl-libs` -lglut -lGL -lSOIL -I/usr/include
