#!/bin/bash

##############################################################################################
# MacOS X default environment for Carla

source data/macos/common.env

export CC=clang
export CXX=clang++

export MACOS="true"

if [ $(clang -v  2>&1 | sed -n 's/.*version \([0-9]\).*/\1/p') -lt 9 ]; then
  export MACOS_OLD="true"
fi

export CC=clang
export CXX=clang++
export CFLAGS="-I${TARGETDIR}/carla64/include -m64"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${TARGETDIR}/carla64/lib -m64"
unset CPPFLAGS

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla/lib/pkgconfig:${TARGETDIR}/carla64/lib/pkgconfig

#export DEFAULT_QT=5
#export PYRCC5=$TARGETDIR/carla/bin/pyrcc5
#export PYUIC5=$TARGETDIR/carla/bin/pyuic5
