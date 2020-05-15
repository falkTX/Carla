#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check input

ARCH="${1}"
ARCH_PREFIX="${1}"

if [ x"${ARCH}" != x"32" ] && [ x"${ARCH}" != x"32nosse" ] && [ x"${ARCH}" != x"64" ]; then
  echo "usage: $0 32|32nonosse|64"
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

if [ x"${ARCH}" = x"32nosse" ]; then
  ARCH="32"
  MAKE_ARGS="${MAKE_ARGS} NOOPT=true"
fi

MAKE_ARGS="${MAKE_ARGS} BUILDING_FOR_WINDOWS=true CROSS_COMPILING=true"

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

export DEPS_PREFIX=${TARGETDIR}/carla-w${ARCH_PREFIX}
export MSYS2_PREFIX=${TARGETDIR}/msys2-${CPUARCH}/mingw${ARCH}

export PATH=${DEPS_PREFIX}/bin:/opt/wine-staging/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${DEPS_PREFIX}/lib/pkgconfig:${MSYS2_PREFIX}/lib/pkgconfig

export AR=${_MINGW_PREFIX}-ar
export CC=${_MINGW_PREFIX}-gcc
export CXX=${_MINGW_PREFIX}-g++
export STRIP=${_MINGW_PREFIX}-strip
export WINDRES=${_MINGW_PREFIX}-windres

export CFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL"
export CFLAGS="${CFLAGS} -I${DEPS_PREFIX}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${DEPS_PREFIX}/lib"

export MOC_QT5="wine ${MSYS2_PREFIX}/bin/moc.exe"
export RCC_QT5="wine ${MSYS2_PREFIX}/bin/rcc.exe"
export UIC_QT5="wine ${MSYS2_PREFIX}/bin/uic.exe"

}

# ---------------------------------------------------------------------------------------------------------------------

export_vars "${ARCH}" "${ARCH_PREFIX}" "${CPUARCH}"

export WINEARCH=win${ARCH}
export WINEDEBUG=-all
export WINEPREFIX=~/.winepy3_x${ARCH}
export PYTHON_EXE="wine ${MSYS2_PREFIX}/bin/python.exe"
export PYRCC5="${PYTHON_EXE} -m PyQt5.pyrcc_main"
export PYUIC5="${PYTHON_EXE} -m PyQt5.uic.pyuic"

make ${MAKE_ARGS}

if [ x"${ARCH}" != x"32" ]; then
  export_vars "32" "32" "i686"
  make ${MAKE_ARGS} win32
fi

# Testing:
echo "export WINEPREFIX=~/.winepy3_x${ARCH}"
echo "/opt/wine-staging/bin/${PYTHON_EXE} ./source/frontend/carla"

# ---------------------------------------------------------------------------------------------------------------------
