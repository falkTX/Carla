#!/bin/bash

set -e

MAKE_FLAGS="-j 8 HAVE_FFMPEG=false HAVE_PULSEAUDIO=false EXTERNAL_PLUGINS=false"

if [ ! -f Makefile ]; then
  cd ../..
fi

TARGETDIR=$HOME/builds

export LINUX="true"
export CC=gcc
export CXX=g++

unset CPPFLAGS

##############################################################################################
# Complete 64bit build

export CFLAGS="-m64"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m64"
export PKG_CONFIG_PATH=$TARGETDIR/carla/lib/pkgconfig:$TARGETDIR/carla64/lib/pkgconfig

make $MAKE_FLAGS

##############################################################################################
# Build 32bit bridges

export CFLAGS="-m32"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m32"
export PKG_CONFIG_PATH=$TARGETDIR/carla32/lib/pkgconfig

make posix32 $MAKE_FLAGS

##############################################################################################
