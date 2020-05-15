#!/bin/bash

set -e

# Preparation
_FLAGS="-DFLUIDSYNTH_NOT_A_DLL -DPTW32_STATIC_LIB -Werror -DBUILDING_FOR_CI"
_PREFIX=i686-w64-mingw32
export AR=${_PREFIX}-ar
export CC=${_PREFIX}-gcc
export CXX=${_PREFIX}-g++
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export PATH=/opt/mingw32/${_PREFIX}/bin:/opt/mingw32/bin:${PATH}
export PKG_CONFIG_PATH=/opt/mingw32/lib/pkgconfig:${PKG_CONFIG_PATH}
export CROSS_COMPILING=true

MAKE_ARGS="BUILDING_FOR_WINDOWS=true CROSS_COMPILING=true USING_JUCE=false USING_JUCE_AUDIO_DEVICES=false"
MAKE_ARGS="${MAKE_ARGS} HAVE_FLUIDSYNTH=false HAVE_LIBLO=false HAVE_QT5=false HAVE_SNDFILE=false NEEDS_WINE=false"

# Start clean
make distclean >/dev/null

# Print available features
make ${MAKE_ARGS} features

# Build native stuff
make ${MAKE_ARGS}
