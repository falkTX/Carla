#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check input

ARCH="${1}"
ARCH_PREFIX="${1}"

if [ x"${ARCH}" != x"32" ] && [ x"${ARCH}" != x"32nosse" ] && [ x"${ARCH}" != x"64" ]; then
  echo "usage: $0 32|64"
  exit 1
fi

if [ x"${ARCH}" = x"32nosse" ]; then
  ARCH="32"
  MAKE_ARGS="NOOPT=true"
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

MAKE_ARGS="${MAKE_ARGS} HAVE_QT4=false HAVE_QT5=false HAVE_PYQT5=true HAVE_FFMPEG=false HAVE_PROJECTM=false"
MAKE_ARGS="${MAKE_ARGS} BUILDING_FOR_WINDOWS=true"

export WIN32=true

if [ x"${ARCH}" != x"32" ]; then
  export WIN64=true
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

# ---------------------------------------------------------------------------------------------------------------------

export_vars() {

local _ARCH="${1}"
local _ARCH_PREFIX="${2}"
local _MINGW_PREFIX="${3}-w64-mingw32"

export PREFIX=${TARGETDIR}/carla-w${_ARCH_PREFIX}
export PATH=/opt/mingw${_ARCH}/bin:${PREFIX}/bin/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

export AR=${_MINGW_PREFIX}-ar
export CC=${_MINGW_PREFIX}-gcc
export CXX=${_MINGW_PREFIX}-g++
export STRIP=${_MINGW_PREFIX}-strip
export WINDRES=${_MINGW_PREFIX}-windres

export CFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL"
export CFLAGS="${CFLAGS} -I${PREFIX}/include -I/opt/mingw${_ARCH}/include -I/opt/mingw${_ARCH}/${_MINGW_PREFIX}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${PREFIX}/lib -L/opt/mingw${_ARCH}/lib -L/opt/mingw${_ARCH}/${_MINGW_PREFIX}/lib"

}

# ---------------------------------------------------------------------------------------------------------------------

export_vars "${ARCH}" "${ARCH_PREFIX}" "${CPUARCH}"

export WINEARCH=win${ARCH}
export WINEDEBUG=-all
export WINEPREFIX=~/.winepy3_x${ARCH}
export PYTHON_EXE="wine C:\\\\Python34\\\\python.exe"

export CXFREEZE="$PYTHON_EXE C:\\\\Python34\\\\Scripts\\\\cxfreeze"
export PYUIC="$PYTHON_EXE -m PyQt5.uic.pyuic"
export PYRCC="wine C:\\\\Python34\\\\Lib\\\\site-packages\\\\PyQt5\\\\pyrcc5.exe"

make ${MAKE_ARGS}

if [ x"${ARCH}" != x"32" ]; then
  export_vars "32" "32" "i686"
  make ${MAKE_ARGS} win32
fi

# Testing:
echo "export WINEPREFIX=~/.winepy3_x${ARCH}"
echo "$PYTHON_EXE ./source/carla"
