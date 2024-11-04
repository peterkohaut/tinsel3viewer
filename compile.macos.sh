#!/bin/bash

set -xe

# CXX=g++
CXX="clang++ -fstandalone-debug" #-D_GLIBCXX_DEBUG

SRC="viewer.cpp tinsel.cpp imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp imgui/imgui*.cpp "
INCLUDES="-Iimgui -Iimgui/backends -Iimgui_club/imgui_memory_editor $(pkg-config sdl2 --cflags) "
LIBS="$(pkg-config sdl2 --libs) $(pkg-config glew --libs) -framework OpenGL"
ARGS="--std=c++17 -g -o viewer "


$CXX $ARGS $SRC $INCLUDES $LIBS
