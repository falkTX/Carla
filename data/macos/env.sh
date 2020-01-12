#!/bin/bash

##############################################################################################
# MacOS X default environment for Carla

source data/macos/common.env

if [ $(clang -v  2>&1 | grep version | cut -d' ' -f4 | cut -d'.' -f1) -lt 11 ]; then
  export MACOS_VERSION_MIN="10.8"
else
  export MACOS_VERSION_MIN="10.12"
fi

export CC=clang
export CXX=clang++
export CFLAGS="-I${TARGETDIR}/carla64/include -m64 -mmacosx-version-min=${MACOS_VERSION_MIN}"
export CXXFLAGS="${CFLAGS} -stdlib=libc++"
export LDFLAGS="-L${TARGETDIR}/carla64/lib -m64 -stdlib=libc++"
unset CPPFLAGS

export MACOS="true"

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla/lib/pkgconfig:${TARGETDIR}/carla64/lib/pkgconfig

export MOC_QT5=moc
export RCC_QT5=rcc
export UIC_QT5=uic
