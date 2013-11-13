#!/bin/bash
# freeglut2-dev
# libwxgtk2.8-dev
g++ main.cpp -o run `wx-config --libs --cxxflags --gl-libs` -lglut -lGL -I/usr/include
