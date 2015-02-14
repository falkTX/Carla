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

export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla/lib/pkgconfig:/opt/carla64/lib/pkgconfig

export DEFAULT_QT=5
export PYUIC5=/opt/carla/bin/pyuic5
