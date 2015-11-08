#!/bin/bash

##############################################################################################
# MacOS X default environment for Carla

TARGETDIR=$HOME/builds

export MACOS="true"
export CC=clang
export CXX=clang++

export CFLAGS="-O3 -m64"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m64"
unset CPPFLAGS

export PATH=$TARGETDIR/carla/bin:$TARGETDIR/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$TARGETDIR/carla/lib/pkgconfig:$TARGETDIR/carla64/lib/pkgconfig

export DEFAULT_QT=5
export PYUIC5=$TARGETDIR/carla/bin/pyuic5
