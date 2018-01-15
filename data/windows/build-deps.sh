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

rm -rf $TARGETDIR/carla-w32/ $TARGETDIR/carla-w64/

}

cleanup_pkgs()
{

rm -rf flac-*
rm -rf fluidsynth-*
rm -rf glib-*
rm -rf libgig-*
rm -rf liblo-*
rm -rf libogg-*
rm -rf libsndfile-*
rm -rf libvorbis-*
rm -rf linuxsampler-*
rm -rf pkg-config-*
rm -rf zlib-*

}

cleanup()
{

cleanup_prefix
cleanup_pkgs
exit 0

}

# ------------------------------------------------------------------------------------
# function to build base libs

build_base()
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

export PREFIX=${TARGETDIR}/carla-w${ARCH}
export PATH=${PREFIX}/bin/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

# ---------------------------------------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-${PKG_CONFIG_VERSION} ]; then
  wget -c https://pkg-config.freedesktop.org/releases/pkg-config-${PKG_CONFIG_VERSION}.tar.gz
  tar -xf pkg-config-${PKG_CONFIG_VERSION}.tar.gz
fi

if [ ! -f pkg-config-${PKG_CONFIG_VERSION}/build-done ]; then
  cd pkg-config-${PKG_CONFIG_VERSION}
  env AR="ar" CC="gcc" STRIP="strip" CFLAGS="" LDFLAGS="" PATH="/usr/sbin:/usr/bin:/sbin:/bin" \
    ./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# setup

if [ x"${ARCH}" != x"32" ]; then
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

HOST_ARCH=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE)
MINGW_PREFIX="${CPUARCH}-w64-mingw32"

export AR=${MINGW_PREFIX}-ar
export CC=${MINGW_PREFIX}-gcc
export CXX=${MINGW_PREFIX}-g++
export STRIP=${MINGW_PREFIX}-strip
export WINDRES=${MINGW_PREFIX}-windres

export PATH=/opt/mingw${ARCH}/${MINGW_PREFIX}/bin:/opt/mingw${ARCH}/bin:${PATH}

# export MOC=${MINGW_PREFIX}-moc
# export RCC=${MINGW_PREFIX}-rcc
# export PKGCONFIG=${MINGW_PREFIX}-pkg-config

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="${CFLAGS} -DNDEBUG -DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL -I${PREFIX}/include -I/opt/mingw${ARCH}/include"
export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"
export CPPFLAGS="-DPIC -DNDEBUG -DPTW32_STATIC_LIB -I${PREFIX}/include -I/opt/mingw${ARCH}/include"

export LDFLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
export LDFLAGS="${LDFLAGS} -L${PREFIX}/lib -L/opt/mingw${ARCH}/lib"

# FIXME: the following flags are not supported by my current mingw build
#

# ---------------------------------------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-${LIBLO_VERSION} ]; then
  wget -c https://download.sourceforge.net/liblo/liblo-${LIBLO_VERSION}.tar.gz
  tar -xf liblo-${LIBLO_VERSION}.tar.gz
fi

if [ ! -f liblo-${LIBLO_VERSION}/build-done ]; then
  cd liblo-${LIBLO_VERSION}
  sed -i "s/@extralibs@/@extralibs@ -lm/" liblo.pc.in
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --enable-threads \
    --disable-examples --disable-tools
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-${ZLIB_VERSION} ]; then
  wget -c https://github.com/madler/zlib/archive/v${ZLIB_VERSION}.tar.gz -O zlib-${ZLIB_VERSION}.tar.gz
  tar -xf zlib-${ZLIB_VERSION}.tar.gz
fi

if [ ! -f zlib-${ZLIB_VERSION}/build-done ]; then
  cd zlib-${ZLIB_VERSION}
  ./configure --static --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libogg

if [ ! -d libogg-${LIBOGG_VERSION} ]; then
  wget -c https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-${LIBOGG_VERSION}.tar.gz
  tar -xf libogg-${LIBOGG_VERSION}.tar.gz
fi

if [ ! -f libogg-${LIBOGG_VERSION}/build-done ]; then
  cd libogg-${LIBOGG_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-${LIBVORBIS_VERSION} ]; then
  wget -c https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${LIBVORBIS_VERSION}.tar.gz
  tar -xf libvorbis-${LIBVORBIS_VERSION}.tar.gz
fi

if [ ! -f libvorbis-${LIBVORBIS_VERSION}/build-done ]; then
  cd libvorbis-${LIBVORBIS_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# flac

if [ ! -d flac-${FLAC_VERSION} ]; then
  wget -c https://svn.xiph.org/releases/flac/flac-${FLAC_VERSION}.tar.xz
  tar -xf flac-${FLAC_VERSION}.tar.xz
fi

if [ ! -f flac-${FLAC_VERSION}/build-done ]; then
  cd flac-${FLAC_VERSION}
  chmod +x configure install-sh
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --disable-cpplibs
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-${LIBSNDFILE_VERSION} ]; then
  wget -c http://www.mega-nerd.com/libsndfile/files/libsndfile-${LIBSNDFILE_VERSION}.tar.gz
  tar -xf libsndfile-${LIBSNDFILE_VERSION}.tar.gz
fi

if [ ! -f libsndfile-${LIBSNDFILE_VERSION}/build-done ]; then
  cd libsndfile-${LIBSNDFILE_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --disable-full-suite --disable-alsa --disable-sqlite
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-${LIBGIG_VERSION} ]; then
  wget -c http://download.linuxsampler.org/packages/libgig-${LIBGIG_VERSION}.tar.bz2
  tar -xf libgig-${LIBGIG_VERSION}.tar.bz2
fi

if [ ! -f libgig-${LIBGIG_VERSION}/build-done ]; then
  cd libgig-${LIBGIG_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/libgig_fix-build.patch
    touch patched
  fi
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-${LINUXSAMPLER_VERSION} ]; then
  wget -c http://download.linuxsampler.org/packages/linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
  tar -xf linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
fi

if [ ! -f linuxsampler-${LINUXSAMPLER_VERSION}/build-done ]; then
  cd linuxsampler-${LINUXSAMPLER_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/linuxsampler_allow-no-drivers-build.patch
    patch -p1 -i ../patches/linuxsampler_disable-ladspa-fx.patch
    sed -i -e "s|HAVE_LV2|HAVE_AU|" src/hostplugins/Makefile.am
    touch patched
  fi
  rm -f configure
  make -f Makefile.svn configure
  ./configure \
    --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --enable-signed-triang-algo=diharmonic --enable-unsigned-triang-algo=diharmonic --enable-subfragment-size=8 \
    --disable-alsa-driver --disable-arts-driver --disable-jack-driver \
    --disable-asio-driver --disable-midishare-driver --disable-mmemidi-driver \
    --disable-coreaudio-driver --disable-coremidi-driver \
    --disable-instruments-db --disable-sf2-engine
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-llinuxsampler|-llinuxsampler -L${PREFIX}/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -lrpcrt4|" ${PREFIX}/lib/pkgconfig/linuxsampler.pc
  touch build-done
  cd ..
fi

# ------------------------------------------------------------------------------------
# glib

if [ ! -d glib-${GLIB_VERSION} ]; then
  wget -c http://caesar.ftp.acc.umu.se/pub/GNOME/sources/glib/${GLIB_MVERSION}/glib-${GLIB_VERSION}.tar.gz
  tar -xf glib-${GLIB_VERSION}.tar.gz
fi

if [ ! -f glib-${GLIB_VERSION}/build-done ]; then
  cd glib-${GLIB_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/glib_skip-gettext.patch
    sed -i "s|po docs|po|" Makefile.in
    touch patched
  fi
  chmod +x configure install-sh
  autoconf
  ./configure --enable-static --disable-shared --disable-docs --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --with-threads=win32
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# fluidsynth

if [ ! -d fluidsynth-${FLUIDSYNTH_VERSION} ]; then
  wget -c https://download.sourceforge.net/fluidsynth/fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
  tar -xf fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
fi

if [ ! -f fluidsynth-${FLUIDSYNTH_VERSION}/build-done ]; then
  cd fluidsynth-${FLUIDSYNTH_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/fluidsynth_fix-64-bit-cast.patch
    touch patched
  fi
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --target=${MINGW_PREFIX} --host=${MINGW_PREFIX} --build=${HOST_ARCH} \
    --enable-libsndfile-support \
    --disable-dbus-support --disable-aufile-support \
    --disable-pulse-support --disable-alsa-support --disable-portaudio-support --disable-oss-support --disable-jack-support \
    --disable-coreaudio --disable-coremidi --disable-dart --disable-lash --disable-ladcca \
    --without-readline
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -ldsound -lwinmm -lole32 -lws2_32|" ${PREFIX}/lib/pkgconfig/fluidsynth.pc
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
build_base
cleanup_pkgs

export ARCH=64
build_base
cleanup_pkgs

# ---------------------------------------------------------------------------------------------------------------------
