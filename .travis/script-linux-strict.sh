#!/bin/bash

set -e

# Preparation
export CC=gcc-8
export CXX=g++-8
unset CFLAGS
unset CXXFLAGS
unset LDFLAGS

# Start clean
make distclean >/dev/null

# Print available features
make features

# Build things that we skip strict tests for
make -C source/modules/audio_decoder
make -C source/modules/dgl
make -C source/modules/hylia
make -C source/modules/rtaudio
make -C source/modules/rtmidi
make -C source/modules/sfzero
make -C source/modules/water
make -C source/theme all qt4 qt5

# FIXME
make -C source/libjack

# Build native stuff
make TESTBUILD=true
