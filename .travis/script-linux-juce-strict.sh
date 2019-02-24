#!/bin/bash

set -e

# Preparation
export CC=gcc-8
export CXX=g++-8
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:${PKG_CONFIG_PATH}
unset CFLAGS
unset CXXFLAGS
unset LDFLAGS

# Start clean
make distclean >/dev/null

# Print available features
make features

# Build things that we skip strict tests for
make 3rd USING_JUCE=true

# Build native stuff
make TESTBUILD=true USING_JUCE=true
