#!/bin/bash

export MACOS="true"
export CC="gcc-4.2"
export CXX="g++-4.2"
export CXXFLAGS="-DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_5 -DMAC_OS_X_VERSION_MAX_ALLOWED=MAC_OS_X_VERSION_10_5"
export PYUIC="/opt/local/Library/Frameworks/Python.framework/Versions/3.3/bin/pyuic5"
export PYRCC="/opt/local/Library/Frameworks/Python.framework/Versions/3.3/bin/pyrcc5"
export PKG_CONFIG_PATH="/Users/falktx/Source/Qt-5.2.1/lib/pkgconfig"
