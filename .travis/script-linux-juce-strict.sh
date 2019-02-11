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
make -C source/modules/audio_decoder
make -C source/modules/dgl
make -C source/modules/hylia
make -C source/modules/juce_audio_basics
make -C source/modules/juce_audio_devices
make -C source/modules/juce_audio_processors
make -C source/modules/juce_core
make -C source/modules/juce_data_structures
make -C source/modules/juce_events
make -C source/modules/juce_graphics
make -C source/modules/juce_gui_basics
make -C source/modules/sfzero
make -C source/modules/water
make -C source/theme all qt4 qt5

# Build native stuff
make TESTBUILD=true USING_JUCE=true
