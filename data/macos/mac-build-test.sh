#!/bin/bash

set -e

export MACOS_OLD=true
export CROSS_COMPILING=true

_FLAGS="-mmacosx-version-min=10.6 -Wno-attributes -Wno-deprecated-declarations -Werror"

export CFLAGS="${_FLAGS} -m32"
export CXXFLAGS="${_FLAGS} -m32"
export LDFLAGS="-m32"
apple-cross-setup make -j4

export CFLAGS="${_FLAGS} -m64"
export CXXFLAGS="${_FLAGS} -m64"
export LDFLAGS="-m64"
# FIXME
apple-cross-setup make posix64 -j4
