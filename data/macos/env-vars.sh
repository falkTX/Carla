#!/bin/bash

export MACOS="true"
export CC="gcc-4.2"
export CXX="g++-4.2"
export CXXFLAGS="-DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_5 -DMAC_OS_X_VERSION_MAX_ALLOWED=MAC_OS_X_VERSION_10_5"
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig

export PYRCC="pyrcc4-3.3 -py3"
export PYUIC="pyuic4-3.3 -w"
