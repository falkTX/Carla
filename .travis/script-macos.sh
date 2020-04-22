#!/bin/bash

set -e

# Preparation
_FLAGS="-Wno-attributes -Wno-deprecated-declarations -Werror -DBUILDING_FOR_CI"
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export MACOS_OLD=true
export CROSS_COMPILING=true
. /usr/bin/apple-cross-setup.env

# Start clean
make distclean >/dev/null

# Print available features
make features

# Build native stuff
make
