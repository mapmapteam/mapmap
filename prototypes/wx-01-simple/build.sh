#!/bin/bash
# libwxgtk2.8-dev
# libsoil-dev
g++ main.cpp -o run `wx-config --libs --cxxflags --gl-libs` -lSOIL
