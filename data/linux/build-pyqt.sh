#!/bin/bash

# apt-get install build-essential autoconf libtool cmake libglib2.0-dev libgl1-mesa-dev

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

cd $(dirname $0)

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf cx_Freeze-*
rm -rf Python-*
rm -rf PyQt-*
rm -rf PyQt5_*
rm -rf pyliblo-*
rm -rf qtbase-*
rm -rf qtsvg-*
rm -rf sip-*

}

# ---------------------------------------------------------------------------------------------------------------------
# function to build base libs

build_pyqt()
{

export CC=gcc
export CXX=g++

export PREFIX=${TARGETDIR}/carla${ARCH}
export PATH=${PREFIX}/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

# ---------------------------------------------------------------------------------------------------------------------
# with visibility

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse"
export CFLAGS="${CFLAGS} -fPIC -DPIC -DNDEBUG -I${PREFIX}/include -m${ARCH}"
export CXXFLAGS="${CFLAGS}"

export LDFLAGS="-L${PREFIX}/lib -m${ARCH} -Wl,-O1"

# TODO build libffi statically

# ---------------------------------------------------------------------------------------------------------------------
# python

if [ ! -d Python-${PYTHON_VERSION} ]; then
  aria2c https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
  tar -xf Python-${PYTHON_VERSION}.tgz
fi

if [ ! -f Python-${PYTHON_VERSION}/build-done ]; then
  cd Python-${PYTHON_VERSION}
  ./configure --prefix=${PREFIX}
  make
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# cxfreeze

if [ ! -d cx_Freeze-${CXFREEZE_VERSION} ]; then
  aria2c https://github.com/anthony-tuininga/cx_Freeze/archive/${CXFREEZE_VERSION}.tar.gz
  tar -xf cx_Freeze-${CXFREEZE_VERSION}.tar.gz
fi

if [ ! -f cx_Freeze-${CXFREEZE_VERSION}/build-done ]; then
  cd cx_Freeze-${CXFREEZE_VERSION}
  python3 setup.py build
  python3 setup.py install --prefix=${PREFIX}
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# without visibility

export CFLAGS="${CFLAGS} -fvisibility=hidden -fdata-sections -ffunction-sections"
export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"

export LDFLAGS="${LDFLAGS} -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,--as-needed -Wl,--strip-all -Wl,--no-undefined"

# ---------------------------------------------------------------------------------------------------------------------
# qt

if [ ! -d qtbase-opensource-src-${QT5_VERSION} ]; then
  aria2c http://download.qt.io/archive/qt/${QT5_MVERSION}/${QT5_VERSION}/submodules/qtbase-opensource-src-${QT5_VERSION}.tar.xz
  tar xf qtbase-opensource-src-${QT5_VERSION}.tar.xz
fi

if [ ! -f qtbase-opensource-src-${QT5_VERSION}/build-done ]; then
  cd qtbase-opensource-src-${QT5_VERSION}
  if [ ! -f configured ]; then
    ./configure -release -strip -silent \
      -sse2 \
      -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 \
      -no-avx -no-avx2 -no-avx512 \
      -prefix ${PREFIX} \
      -opensource -confirm-license \
      -optimize-size -optimized-qmake \
      -qt-freetype \
      -qt-harfbuzz \
      -qt-libjpeg \
      -qt-libpng \
      -qt-pcre \
      -qt-sqlite \
      -qt-xcb \
      -qt-zlib \
      -opengl desktop \
      -no-cups \
      -no-gtk \
      -no-icu \
      -no-openssl \
      -make libs \
      -make tools \
      -nomake examples \
      -nomake tests
    touch configured
  fi
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-${QT5_VERSION} ]; then
  aria2c http://download.qt.io/archive/qt/${QT5_MVERSION}/${QT5_VERSION}/submodules/qtsvg-opensource-src-${QT5_VERSION}.tar.xz
  tar xf qtsvg-opensource-src-${QT5_VERSION}.tar.xz
fi

if [ ! -f qtsvg-opensource-src-${QT5_VERSION}/build-done ]; then
  cd qtsvg-opensource-src-${QT5_VERSION}
  qmake
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# with visibility

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse"
export CFLAGS="${CFLAGS} -fPIC -DPIC -DNDEBUG -I${PREFIX}/include -m${ARCH}"
export CXXFLAGS="${CFLAGS}"

export LDFLAGS="-L${PREFIX}/lib -m${ARCH} -Wl,-O1"

# ---------------------------------------------------------------------------------------------------------------------
# sip

if [ ! -d sip-${SIP_VERSION} ]; then
  aria2c http://sourceforge.net/projects/pyqt/files/sip/sip-${SIP_VERSION}/sip-${SIP_VERSION}.tar.gz
  tar -xf sip-${SIP_VERSION}.tar.gz
fi

if [ ! -f sip-${SIP_VERSION}/build-done ]; then
  cd sip-${SIP_VERSION}
  python3 configure.py --sip-module PyQt5.sip
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt5_gpl-${PYQT5_VERSION} ]; then
  aria2c http://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-${PYQT5_VERSION}/PyQt5_gpl-${PYQT5_VERSION}.tar.gz
  tar -xf PyQt5_gpl-${PYQT5_VERSION}.tar.gz
fi

if [ ! -f PyQt5_gpl-${PYQT5_VERSION}/build-done ]; then
  cd PyQt5_gpl-${PYQT5_VERSION}
  python3 configure.py --confirm-license -c
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# pyliblo

if [ ! -d pyliblo-${PYLIBLO_VERSION} ]; then
  aria2c http://das.nasophon.de/download/pyliblo-${PYLIBLO_VERSION}.tar.gz
  tar -xf pyliblo-${PYLIBLO_VERSION}.tar.gz
fi

if [ ! -f pyliblo-${PYLIBLO_VERSION}/build-done ]; then
  cd pyliblo-${PYLIBLO_VERSION}
  if [ ! -f patched ]; then
    ls ../../macos/patches
    patch -p1 -i ../../macos/patches/pyliblo-python3.7.patch
    touch patched
  fi
  python3 setup.py build
  python3 setup.py install --prefix=${PREFIX}
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export TARGET="${1}"

if [ x"${TARGET}" = x"32" ]; then
  export ARCH=32
else
  export ARCH=64
fi

build_pyqt

# ---------------------------------------------------------------------------------------------------------------------
