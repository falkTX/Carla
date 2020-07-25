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
make USING_JUCE=true features

# Build things that we skip strict tests for
make USING_JUCE=true 3rd frontend
make USING_JUCE=true -C source/modules/juce_audio_basics posix32
make USING_JUCE=true -C source/modules/juce_audio_devices posix32
make USING_JUCE=true -C source/modules/juce_audio_processors posix32
make USING_JUCE=true -C source/modules/juce_core posix32
make USING_JUCE=true -C source/modules/juce_data_structures posix32
make USING_JUCE=true -C source/modules/juce_events posix32
make USING_JUCE=true -C source/modules/juce_graphics posix32
make USING_JUCE=true -C source/modules/juce_gui_basics posix32
make USING_JUCE=true -C source/modules/juce_gui_extra posix32
make USING_JUCE=true -C source/modules/water posix32

# Build native stuff
make USING_JUCE=true TESTBUILD=true

# Build 32bit bridges
make USING_JUCE=true TESTBUILD=true posix32
