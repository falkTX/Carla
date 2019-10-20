#!/bin/bash

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

cleanup_prefix()
{

rm -rf ${TARGETDIR}/msys2-i686 ${TARGETDIR}/msys2-x86_64
rm -rf python-pkgs-*

}

cleanup_pkgs()
{

rm -rf Carla
rm -rf Carla.exe
rm -rf Carla.lv2
rm -rf Carla.vst
rm -rf Carla_*
rm -rf pyliblo-*

}

cleanup()
{

cleanup_prefix
cleanup_pkgs
exit 0

}

# ---------------------------------------------------------------------------------------------------------------------
# function to download python stuff from msys2

download_python()
{

# ---------------------------------------------------------------------------------------------------------------------
# setup

if [ x"${ARCH}" != x"32" ]; then
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

# ---------------------------------------------------------------------------------------------------------------------
# list packages

PACKAGES=(
  "binutils-2.32-3"
  "bzip2-1.0.8-1"
  "crt-git-7.0.0.5524.2346384e-1"
  "dbus-1.12.8-1"
  "double-conversion-3.1.5-1"
  "expat-2.2.8-1"
  "freetype-2.10.1-1"
  "gcc-9.2.0-2"
  "gcc-libs-9.2.0-2"
  "gettext-0.19.8.1-8"
  "glib2-2.62.1-1"
  "gmp-6.1.2-1"
  "graphite2-1.3.13-1"
  "harfbuzz-2.6.2-1"
  "icu-64.2-1"
  "jasper-2.0.16-1"
  "libffi-3.2.1-4"
  "libiconv-1.16-1"
  "libjpeg-turbo-2.0.3-1"
  "libpng-1.6.37-3"
  "libtiff-4.0.9-2"
  "libwebp-1.0.3-1"
  "libxml2-2.9.9-2"
  "libxslt-1.1.33-1"
  "libwinpthread-git-7.0.0.5522.977a9720-1"
  "headers-git-7.0.0.5524.2346384e-1"
  "openssl-1.1.1.d-1"
  "pcre-8.43-1"
  "pcre2-10.33-1"
  "pyqt5-common-5.13.1-1"
  "python3-3.7.4-7"
  "python3-cx_Freeze-5.1.1-3"
  "python3-nuitka-0.6.4-1"
  "python3-sip-4.19.19-1"
  "python3-pyqt5-5.13.1-1"
  "qt5-5.13.1-1"
  "qtwebkit-5.212.0alpha2-6"
  "sqlite3-3.30.0-1"
  "windows-default-manifest-6.4-3"
  "winpthreads-git-7.0.0.5522.977a9720-1"
  "xz-5.2.4-1"
  "zlib-1.2.11-7"
  "zstd-1.4.3-1"
)
# qt5-static-5.12.4-1

PKG_DIR="$(pwd)/python-pkgs-${CPUARCH}"
PKG_PREFIX="mingw-w64-${CPUARCH}-"
PKG_SUFFIX="-any.pkg.tar.xz"

REPO_URL="http://repo.msys2.org/mingw/${CPUARCH}"

# ---------------------------------------------------------------------------------------------------------------------
# download stuff

mkdir -p "${PKG_DIR}"
pushd "${PKG_DIR}"

for PKG in ${PACKAGES[@]}; do
  wget -c "${REPO_URL}/${PKG_PREFIX}${PKG}${PKG_SUFFIX}"
done

popd

# ---------------------------------------------------------------------------------------------------------------------
# extract into target dir

rm -rf "${TARGETDIR}/msys2-${CPUARCH}"
mkdir "${TARGETDIR}/msys2-${CPUARCH}"

pushd "${TARGETDIR}/msys2-${CPUARCH}"

for PKG in ${PACKAGES[@]}; do
  tar xf "${PKG_DIR}/${PKG_PREFIX}${PKG}${PKG_SUFFIX}"
done

sed -i "s|E:/mingwbuild/mingw-w64-qt5/pkg/mingw-w64-${CPUARCH}-qt5|${TARGETDIR}/msys2-${CPUARCH}|" ./mingw${ARCH}/lib/pkgconfig/Qt5*.pc

popd

}

# ---------------------------------------------------------------------------------------------------------------------
# function to build python modules

build_python()
{

# ---------------------------------------------------------------------------------------------------------------------
# setup

if [ x"${ARCH}" != x"32" ]; then
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

# ---------------------------------------------------------------------------------------------------------------------
# clean env

unset AR
unset CC
unset CXX
unset STRIP
unset WINDRES

unset CFLAGS
unset CPPFLAGS
unset CXXFLAGS
unset LDFLAGS

export DEPS_PREFIX=${TARGETDIR}/carla-w${ARCH_PREFIX}
export MSYS2_PREFIX=${TARGETDIR}/msys2-${CPUARCH}/mingw${ARCH}

export PATH=${DEPS_PREFIX}/bin:${MSYS2_PREFIX}/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${DEPS_PREFIX}/lib/pkgconfig:${MSYS2_PREFIX}/lib/pkgconfig

HOST_ARCH=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE)
MINGW_PREFIX="${CPUARCH}-w64-mingw32"

export AR=${MINGW_PREFIX}-ar
export CC=${MINGW_PREFIX}-gcc
export CXX=${MINGW_PREFIX}-g++
export STRIP=${MINGW_PREFIX}-strip
export WINDRES=${MINGW_PREFIX}-windres

export CFLAGS="-O2 -DNDEBUG -mstackrealign -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="${CFLAGS} -I${DEPS_PREFIX}/include -I${MSYS2_PREFIX}/include"
export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"
export LDFLAGS="-Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
export LDFLAGS="${LDFLAGS} -L${DEPS_PREFIX}/lib -L${MSYS2_PREFIX}/lib"

# ---------------------------------------------------------------------------------------------------------------------
# pyliblo

if [ ! -d pyliblo-${PYLIBLO_VERSION} ]; then
  wget -c http://das.nasophon.de/download/pyliblo-${PYLIBLO_VERSION}.tar.gz
  tar -xf pyliblo-${PYLIBLO_VERSION}.tar.gz
fi

if [ ! -f pyliblo-${PYLIBLO_VERSION}/build-done ]; then
  cd pyliblo-${PYLIBLO_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/pyliblo-python3.7.patch
    touch patched
  fi
  mkdir -p build
  # build
  ${CC} -pthread -Wall ${CFLAGS} \
    src/liblo.c -c -o build/liblo.o \
    $(python3-config --cflags | awk 'sub("-ne ","")') \
    -D_FORTIFY_SOURCE=2 -fPIC -fno-strict-aliasing \
    -Wdate-time -Werror-implicit-function-declaration -Wfatal-errors
  # link
  ${CC} -pthread -shared ${LDFLAGS} \
    build/liblo.o -o build/liblo-cpython-37m.dll \
    -llo $(python3-config --ldflags | awk 'sub("-ne ","")') -lws2_32 -liphlpapi
  # install
  install -m 644 build/liblo-cpython-37m.dll ${MSYS2_PREFIX}/lib/python3.7/site-packages/
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
export ARCH_PREFIX=32nosse
download_python
build_python
cleanup_pkgs

export ARCH=64
export ARCH_PREFIX=64
download_python
build_python
cleanup_pkgs

# ---------------------------------------------------------------------------------------------------------------------
