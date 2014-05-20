#!/bin/bash

export MACOS="true"
export CC="/opt/local/bin/clang"
export CXX="/opt/local/bin/clang++"
export CXXFLAGS="-DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_5 -DMAC_OS_X_VERSION_MAX_ALLOWED=MAC_OS_X_VERSION_10_5"
export PATH=/opt/kxstudio/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig
