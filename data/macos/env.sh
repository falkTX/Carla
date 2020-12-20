#!/bin/bash

##############################################################################################
# MacOS X default environment for Carla

source data/macos/common.env

if [ $(uname -r | cut -d '.' -f 1) -lt 16 ]; then
  export MACOS_VERSION_MIN="10.8"
else
  export MACOS_VERSION_MIN="10.12"
fi

export CC=clang
export CXX=clang++
export CFLAGS="-I${TARGETDIR}/carla64/include -mmacosx-version-min=${MACOS_VERSION_MIN}"
export CFLAGS="${CFLAGS} -mtune=generic -msse -msse2"
export LDFLAGS="-L${TARGETDIR}/carla64/lib -stdlib=libc++"
unset CPPFLAGS

if [ "${MACOS_UNIVERSAL}" -eq 1 ]; then
    export CFLAGS="${CFLAGS} -arch x86_64 -arch arm64"
    export LDFLAGS="${LDFLAGS} -arch x86_64 -arch arm64"
    export MACOS_UNIVERSAL="true"
else
    export CFLAGS="${CFLAGS} -m64"
    export LDFLAGS="${LDFLAGS} -m64"
fi

export CXXFLAGS="${CFLAGS} -stdlib=libc++ -Wno-unknown-pragmas -Wno-unused-private-field -Werror=auto-var-id"

export MACOS="true"

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla/lib/pkgconfig:${TARGETDIR}/carla64/lib/pkgconfig

export MOC_QT5=moc
export RCC_QT5=rcc
export UIC_QT5=uic
