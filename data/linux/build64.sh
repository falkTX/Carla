#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd ../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

MAKE_FLAGS="${MAKE_FLAGS} HAVE_FFMPEG=false HAVE_PULSEAUDIO=false EXTERNAL_PLUGINS=false"

export LINUX="true"
export CC=gcc
export CXX=g++

unset CPPFLAGS

# ---------------------------------------------------------------------------------------------------------------------
# Complete 64bit build

export CFLAGS="-m64"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-m64"
export PKG_CONFIG_PATH=${TARGETDIR}/carla64/lib/pkgconfig

make ${MAKE_FLAGS}

# ---------------------------------------------------------------------------------------------------------------------
# Build 32bit bridges

export CFLAGS="-m32"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-m32"
export PKG_CONFIG_PATH=${TARGETDIR}/carla32/lib/pkgconfig

make posix32 ${MAKE_FLAGS}

# ---------------------------------------------------------------------------------------------------------------------
