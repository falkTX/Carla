#!/bin/bash

set -e

MAKE_FLAGS="-j 8 HAVE_FFMPEG=false HAVE_PROJECTM=false HAVE_PULSEAUDIO=false EXTERNAL_PLUGINS=false"

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

export CFLAGS="-O2 -m64"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m64"

# export PATH=$TARGETDIR/carla/bin:$TARGETDIR/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$TARGETDIR/carla/lib/pkgconfig:$TARGETDIR/carla64/lib/pkgconfig

make $MAKE_FLAGS

##############################################################################################
# Build 32bit bridges

export CFLAGS="-O2 -m32"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m32"

# export PATH=$TARGETDIR/carla32/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$TARGETDIR/carla32/lib/pkgconfig

make posix32 $MAKE_FLAGS

##############################################################################################
