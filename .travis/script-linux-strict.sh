#!/bin/bash

set -e

# Preparation
export CC=gcc-9
export CXX=g++-9
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:${PKG_CONFIG_PATH}
unset CFLAGS
unset CXXFLAGS
unset LDFLAGS

# Start clean
make distclean >/dev/null

# Print available features
make USING_JUCE=false features

# Build things that we skip strict tests for
make USING_JUCE=false 3rd frontend
make USING_JUCE=false -C source/modules/water posix32

# Build native stuff
make USING_JUCE=false TESTBUILD=true

# Build 32bit bridges
make USING_JUCE=false TESTBUILD=true posix32
