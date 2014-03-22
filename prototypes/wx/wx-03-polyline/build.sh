#!/bin/bash
set -o verbose

# libwxgtk2.8-dev
g++ main.cpp -o run `wx-config --libs --cxxflags --gl-libs`  -lGL -lGLU -lglut

# g++ main.cpp -o run -I/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread -lwx_gtk2u_gl-2.8 -L/usr/lib/x86_64-linux-gnu -pthread -Wl,-Bsymbolic-functions -Wl,-z,relro  -L/usr/lib/x86_64-linux-gnu   -lwx_gtk2u_richtext-2.8 -lwx_gtk2u_aui-2.8 -lwx_gtk2u_xrc-2.8 -lwx_gtk2u_qa-2.8 -lwx_gtk2u_html-2.8 -lwx_gtk2u_adv-2.8 -lwx_gtk2u_core-2.8 -lwx_baseu_xml-2.8 -lwx_baseu_net-2.8 -lwx_baseu-2.8 
