#!/bin/bash

##############################################################################################
# MacOS X default environment for Carla

export MACOS="true"
export CC=clang
export CXX=clang++

export CFLAGS=-m64
export CXXFLAGS=-m64
export LDLAGS=-m64
unset CPPFLAGS

export PATH=$HOME/Builds/carla/bin:$HOME/Builds/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$HOME/Builds/carla/lib/pkgconfig:$HOME/Builds/carla64/lib/pkgconfig

export DEFAULT_QT=5
export PYUIC5=/opt/carla/bin/pyuic5
