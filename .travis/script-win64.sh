#!/bin/bash

set -e

# Preparation
_FLAGS="-DFLUIDSYNTH_NOT_A_DLL -DPTW32_STATIC_LIB -Werror"
_PREFIX=x86_64-w64-mingw32
export AR=${_PREFIX}-ar
export CC=${_PREFIX}-gcc
export CXX=${_PREFIX}-g++
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export PATH=/opt/mingw64/${_PREFIX}/bin:/opt/mingw64/bin:${PATH}
export PKG_CONFIG_PATH=/opt/mingw64/lib/pkgconfig:${PKG_CONFIG_PATH}
export CROSS_COMPILING=true

# Start clean
make distclean >/dev/null

# Print available features
make features

# Build native stuff
make BUILDING_FOR_WINDOWS=true
