#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd $(dirname $0)/../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source data/linux/common.env

MAKE_ARGS="${MAKE_ARGS} HAVE_FFMPEG=false HAVE_PULSEAUDIO=false EXTERNAL_PLUGINS=false"

export LINUX="true"
export CC=gcc
export CXX=g++

unset CPPFLAGS

# ---------------------------------------------------------------------------------------------------------------------
# Complete 64bit build

export CFLAGS="-I${TARGETDIR}/carla64/include -m64"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-L${TARGETDIR}/carla64/lib -m64"
export PKG_CONFIG_PATH=${TARGETDIR}/carla64/lib/pkgconfig

make ${MAKE_ARGS}

# ---------------------------------------------------------------------------------------------------------------------
# Build 32bit bridges

export CFLAGS="-I${TARGETDIR}/carla32/include -m32"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-L${TARGETDIR}/carla32/lib -m32"
export PKG_CONFIG_PATH=${TARGETDIR}/carla32/lib/pkgconfig

make posix32 ${MAKE_ARGS}

# ---------------------------------------------------------------------------------------------------------------------
