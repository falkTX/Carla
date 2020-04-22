#!/bin/bash

set -e

# Preparation
_FLAGS="-Werror"
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export LDFLAGS="-L/opt/kxstudio/lib"
export PATH=/opt/kxstudio/bin:${PATH}
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:${PKG_CONFIG_PATH}

# Start clean
make distclean >/dev/null

# Print available features
make USING_JUCE=false features

# Build native stuff
make USING_JUCE=false all posix32 posix64

# Build wine bridges
make USING_JUCE=false wine32 wine64 plugin-wine

# Build windows binaries for bridges
env PATH=/opt/mingw32/bin:${PATH} make win32 USING_JUCE=false CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++
env PATH=/opt/mingw64/bin:${PATH} make win64 USING_JUCE=false CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
