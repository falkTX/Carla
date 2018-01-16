#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check input

ARCH="${1}"

if [ x"${ARCH}" != x"32" ] && [ x"${ARCH}" != x"64" ]; then
  echo "usage: $0 32|64"
  exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd $(dirname $0)/../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source data/windows/common.env

MAKE_ARGS="${MAKE_ARGS} HAVE_QT4=false HAVE_QT5=false HAVE_PYQT5=true HAVE_FFMPEG=false"
MAKE_ARGS="${MAKE_ARGS} BUILDING_FOR_WINDOWS=true EXTERNAL_PLUGINS=false"

if [ x"${ARCH}" != x"32" ]; then
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

MINGW_PREFIX="${CPUARCH}-w64-mingw32"

export PREFIX=${TARGETDIR}/carla-w${ARCH}
export PATH=/opt/mingw${ARCH}/bin:${PREFIX}/bin/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

export AR=${MINGW_PREFIX}-ar
export CC=${MINGW_PREFIX}-gcc
export CXX=${MINGW_PREFIX}-g++
export STRIP=${MINGW_PREFIX}-strip
export WINDRES=${MINGW_PREFIX}-windres

export CFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL"
export CFLAGS="${CFLAGS} -I${PREFIX}/include -I/opt/mingw${ARCH}/include -I/opt/mingw${ARCH}/${MINGW_PREFIX}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${PREFIX}/lib -L/opt/mingw${ARCH}/lib -L/opt/mingw${ARCH}/${MINGW_PREFIX}/lib"

export WIN32=true

if [ x"${ARCH}" != x"32" ]; then
  export WIN64=true
fi

export WINEARCH=win${ARCH}
export WINEPREFIX=~/.winepy3_x${ARCH}
export PYTHON_EXE="wine C:\\\\Python34\\\\python.exe"

export CXFREEZE="$PYTHON_EXE C:\\\\Python34\\\\Scripts\\\\cxfreeze"
export PYUIC="$PYTHON_EXE -m PyQt5.uic.pyuic"
export PYRCC="wine C:\\\\Python34\\\\Lib\\\\site-packages\\\\PyQt5\\\\pyrcc5.exe"

make ${MAKE_ARGS}

if [ x"${ARCH}" != x"32" ]; then
  make ${MAKE_ARGS} HAVE_LIBLO=false LDFLAGS="-L${TARGETDIR}/carla-w32/lib -L/opt/mingw32/lib -L/opt/mingw32/i686-w64-mingw32/lib" win32
fi

# Testing:
echo "export WINEPREFIX=~/.winepy3_x${ARCH}"
echo "$PYTHON_EXE ./source/carla"
