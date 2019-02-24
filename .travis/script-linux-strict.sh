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
make 3rd
make -C source/modules/water posix32

# Build native stuff
make TESTBUILD=true

# Build 32bit bridges
make TESTBUILD=true posix32
