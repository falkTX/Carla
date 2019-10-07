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

# ------------------------------------------------------------------------------------
# function to build python

build_python()
{

# ---------------------------------------------------------------------------------------------------------------------
# clean env

unset AR
unset CC
unset CXX
unset STRIP
unset WINDRES

unset PKG_CONFIG_PATH

unset CFLAGS
unset CPPFLAGS
unset CXXFLAGS
unset LDFLAGS

export PREFIX=${TARGETDIR}/carla-w${ARCH_PREFIX}
export PATH=${PREFIX}/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

# ---------------------------------------------------------------------------------------------------------------------
# setup

if [ x"${ARCH}" != x"32" ]; then
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

# ---------------------------------------------------------------------------------------------------------------------
# download stuff

REPO_URL="http://repo.msys2.org/mingw/${CPUARCH}"

GCC_VERSION="9.2.0-2"
WINPTHREADS_VERSION="git-7.0.0.5522.977a9720-1"
PYTHON_VERSION="3.7.4-7"
QT5_VERSION="5.13.1-1"
SIP_VERSION="4.19.19-1"
PYQT5_VERSION="5.13.1-1"

PKGDIR="$(pwd)/python-pkgs-${CPUARCH}"

mkdir -p "${PKGDIR}"
pushd "${PKGDIR}"

# # gcc
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-gcc-${GCC_VERSION}-any.pkg.tar.xz"
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-gcc-libs-${GCC_VERSION}-any.pkg.tar.xz"
# 
# # winpthreads
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-libwinpthread-${WINPTHREADS_VERSION}-any.pkg.tar.xz"
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-winpthreads-${WINPTHREADS_VERSION}-any.pkg.tar.xz"
# 
# # python3
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-python3-${PYTHON_VERSION}-any.pkg.tar.xz"
# 
# # sip
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-python3-sip-${SIP_VERSION}-any.pkg.tar.xz"
# 
# # qt5
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-qt5-${QT5_VERSION}-any.pkg.tar.xz"
# # /mingw-w64-${CPUARCH}-qt5-static-5.12.4-1-any.pkg.tar.xz
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-qtwebkit-5.212.0alpha2-6-any.pkg.tar.xz"

# # pyqt5
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-pyqt5-common-${PYQT5_VERSION}-any.pkg.tar.xz"
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-python3-pyqt5-${PYQT5_VERSION}-any.pkg.tar.xz"
# 
# # misc
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-zlib-1.2.11-7-any.pkg.tar.xz"
# wget -c "${REPO_URL}/mingw-w64-${CPUARCH}-zstd-1.4.3-1-any.pkg.tar.xz"
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-expat-2.2.8-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-icu-64.2-1-any.pkg.tar.xz
# # wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-pcre-8.43-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-pcre2-10.33-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-double-conversion-3.1.5-1-any.pkg.tar.xz

# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-harfbuzz-2.6.2-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libpng-1.6.37-3-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-dbus-1.12.8-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-freetype-2.10.1-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-glib2-2.62.1-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-graphite2-1.3.13-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-bzip2-1.0.8-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-gettext-0.19.8.1-8-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libiconv-1.16-1-any.pkg.tar.xz

# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libffi-3.2.1-4-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-openssl-1.1.1.d-1-any.pkg.tar.xz

# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libjpeg-turbo-2.0.3-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-sqlite3-3.30.0-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libwebp-1.0.3-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libxml2-2.9.9-2-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libxslt-1.1.33-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-xz-5.2.4-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-jasper-2.0.16-1-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-libtiff-4.0.9-2-any.pkg.tar.xz
# wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-python3-nuitka-0.6.4-1-any.pkg.tar.xz
wget -c http://repo.msys2.org/mingw/i686/mingw-w64-i686-python3-cx_Freeze-5.1.1-3-any.pkg.tar.xz

# wget -c 

popd

# ---------------------------------------------------------------------------------------------------------------------
# extract into target dir

# rm -rf "${TARGETDIR}/msys2-${CPUARCH}"
# mkdir "${TARGETDIR}/msys2-${CPUARCH}"

pushd "${TARGETDIR}/msys2-${CPUARCH}"

# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-gcc-${GCC_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-gcc-libs-${GCC_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-libwinpthread-${WINPTHREADS_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-winpthreads-${WINPTHREADS_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-python3-${PYTHON_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-python3-sip-${SIP_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-qt5-${QT5_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-pyqt5-common-${PYQT5_VERSION}-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-python3-pyqt5-${PYQT5_VERSION}-any.pkg.tar.xz"

# tar xf "${PKGDIR}/mingw-w64-i686-expat-2.2.8-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-icu-64.2-1-any.pkg.tar.xz"
# # tar xf "${PKGDIR}/mingw-w64-i686-pcre-8.43-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-pcre2-10.33-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-double-conversion-3.1.5-1-any.pkg.tar.xz"

# tar xf "${PKGDIR}/mingw-w64-i686-harfbuzz-2.6.2-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libpng-1.6.37-3-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-dbus-1.12.8-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-freetype-2.10.1-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-glib2-2.62.1-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-graphite2-1.3.13-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-bzip2-1.0.8-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-gettext-0.19.8.1-8-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libiconv-1.16-1-any.pkg.tar.xz"

# tar xf "${PKGDIR}/mingw-w64-${CPUARCH}-qtwebkit-5.212.0alpha2-6-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libffi-3.2.1-4-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-openssl-1.1.1.d-1-any.pkg.tar.xz"

# tar xf "${PKGDIR}/mingw-w64-i686-libjpeg-turbo-2.0.3-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-sqlite3-3.30.0-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libwebp-1.0.3-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libxml2-2.9.9-2-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libxslt-1.1.33-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-xz-5.2.4-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-jasper-2.0.16-1-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-libtiff-4.0.9-2-any.pkg.tar.xz"
# tar xf "${PKGDIR}/mingw-w64-i686-python3-nuitka-0.6.4-1-any.pkg.tar.xz"
tar xf "${PKGDIR}/mingw-w64-i686-python3-cx_Freeze-5.1.1-3-any.pkg.tar.xz"

# tar xf "${PKGDIR}/"

sed -i "s|E:/mingwbuild/mingw-w64-qt5/pkg/mingw-w64-i686-qt5|${TARGETDIR}/msys2-${CPUARCH}|" ./mingw${ARCH}/lib/pkgconfig/Qt5*.pc

popd

# HOST_ARCH=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE)
# MINGW_PREFIX="${CPUARCH}-w64-mingw32"

# export AR=${MINGW_PREFIX}-ar
# export CC=${MINGW_PREFIX}-gcc
# export CXX=${MINGW_PREFIX}-g++
# export STRIP=${MINGW_PREFIX}-strip
# export WINDRES=${MINGW_PREFIX}-windres
# 
# export PATH=/opt/mingw${ARCH}/${MINGW_PREFIX}/bin:/opt/mingw${ARCH}/bin:${PATH}
# 
# if [ -z "${NOSSE}" ]; then
# export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -mstackrealign -fvisibility=hidden -fdata-sections -ffunction-sections"
# else
# export CFLAGS="-O2 -mstackrealign -fvisibility=hidden -fdata-sections -ffunction-sections"
# fi
# 
# export CFLAGS="${CFLAGS} -DNDEBUG -DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL -I${PREFIX}/include -I/opt/mingw${ARCH}/include"
# export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"
# export CPPFLAGS="-DPIC -DNDEBUG -DPTW32_STATIC_LIB -I${PREFIX}/include -I/opt/mingw${ARCH}/include"
# 
# export LDFLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
# export LDFLAGS="${LDFLAGS} -L${PREFIX}/lib -L/opt/mingw${ARCH}/lib"

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
export ARCH_PREFIX=32
build_python

# export ARCH=64
# export ARCH_PREFIX=64
# build_python

# ---------------------------------------------------------------------------------------------------------------------
