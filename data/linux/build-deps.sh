#!/bin/bash

# apt-get install build-essential libglib2.0-dev uuid-dev

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ -f Makefile ]; then
  cd data/linux
else
  cd $(dirname $0)
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf ${TARGETDIR}/carla32/ ${TARGETDIR}/carla64/
rm -rf file-*
rm -rf flac-*
rm -rf fltk-*
rm -rf fluidsynth-*
rm -rf libgig-*
rm -rf liblo-*
rm -rf libogg-*
rm -rf libsndfile-*
rm -rf libvorbis-*
rm -rf linuxsampler-*
rm -rf pkg-config-*
rm -rf zlib-*

}

# ---------------------------------------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=gcc
export CXX=g++

export PREFIX=${TARGETDIR}/carla${ARCH}
export PATH=${PREFIX}/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="${CFLAGS} -fPIC -DPIC -DNDEBUG -I${PREFIX}/include -m${ARCH}"
export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"

export LDFLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
export LDFLAGS="${LDFLAGS} -L${PREFIX}/lib -m${ARCH}"

# ---------------------------------------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-${PKG_CONFIG_VERSION} ]; then
  wget --no-check-certificate https://pkg-config.freedesktop.org/releases/pkg-config-${PKG_CONFIG_VERSION}.tar.gz
  tar -xf pkg-config-${PKG_CONFIG_VERSION}.tar.gz
fi

if [ ! -f pkg-config-${PKG_CONFIG_VERSION}_$ARCH/build-done ]; then
  cp -r pkg-config-${PKG_CONFIG_VERSION} pkg-config-${PKG_CONFIG_VERSION}_$ARCH
  cd pkg-config-${PKG_CONFIG_VERSION}_$ARCH
  ./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-${LIBLO_VERSION} ]; then
  wget --no-check-certificate https://download.sourceforge.net/liblo/liblo-${LIBLO_VERSION}.tar.gz
  tar -xf liblo-${LIBLO_VERSION}.tar.gz
fi

if [ ! -f liblo-${LIBLO_VERSION}_$ARCH/build-done ]; then
  cp -r liblo-${LIBLO_VERSION} liblo-${LIBLO_VERSION}_$ARCH
  cd liblo-${LIBLO_VERSION}_$ARCH
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-threads \
    --disable-examples --disable-tools
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------

if [ "$ARCH" == "32" ]; then
return
fi

# ---------------------------------------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-${ZLIB_VERSION} ]; then
  wget --no-check-certificate https://github.com/madler/zlib/archive/v${ZLIB_VERSION}.tar.gz -O zlib-${ZLIB_VERSION}.tar.gz
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
# file/magic

if [ ! -d file-${FILE_VERSION} ]; then
  wget ftp://ftp.astron.com/pub/file/file-${FILE_VERSION}.tar.gz
  tar -xf file-${FILE_VERSION}.tar.gz
fi

if [ ! -f file-${FILE_VERSION}/build-done ]; then
  cd file-${FILE_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libogg

if [ ! -d libogg-${LIBOGG_VERSION} ]; then
  wget --no-check-certificate https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-${LIBOGG_VERSION}.tar.gz
  tar -xf libogg-${LIBOGG_VERSION}.tar.gz
fi

if [ ! -f libogg-${LIBOGG_VERSION}/build-done ]; then
  cd libogg-${LIBOGG_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-${LIBVORBIS_VERSION} ]; then
  wget --no-check-certificate https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${LIBVORBIS_VERSION}.tar.gz
  tar -xf libvorbis-${LIBVORBIS_VERSION}.tar.gz
fi

if [ ! -f libvorbis-${LIBVORBIS_VERSION}/build-done ]; then
  cd libvorbis-${LIBVORBIS_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# flac

if [ ! -d flac-${FLAC_VERSION} ]; then
  wget --no-check-certificate https://svn.xiph.org/releases/flac/flac-${FLAC_VERSION}.tar.xz
  tar -xf flac-${FLAC_VERSION}.tar.xz
fi

if [ ! -f flac-${FLAC_VERSION}/build-done ]; then
  cd flac-${FLAC_VERSION}
  chmod +x configure install-sh
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --disable-cpplibs
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-${LIBSNDFILE_VERSION} ]; then
  wget http://www.mega-nerd.com/libsndfile/files/libsndfile-${LIBSNDFILE_VERSION}.tar.gz
  tar -xf libsndfile-${LIBSNDFILE_VERSION}.tar.gz
fi

if [ ! -f libsndfile-${LIBSNDFILE_VERSION}/build-done ]; then
  cd libsndfile-${LIBSNDFILE_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --disable-full-suite --disable-alsa --disable-sqlite
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-${LIBGIG_VERSION} ]; then
  wget http://download.linuxsampler.org/packages/libgig-${LIBGIG_VERSION}.tar.bz2
  tar -xf libgig-${LIBGIG_VERSION}.tar.bz2
fi

if [ ! -f libgig-${LIBGIG_VERSION}/build-done ]; then
  cd libgig-${LIBGIG_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/libgig_fix-build.patch
    touch patched
  fi
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-${LINUXSAMPLER_VERSION} ]; then
  wget http://download.linuxsampler.org/packages/linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
  tar -xf linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
fi

if [ ! -f linuxsampler-${LINUXSAMPLER_VERSION}/build-done ]; then
  cd linuxsampler-${LINUXSAMPLER_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/linuxsampler_allow-no-drivers-build.patch
    patch -p1 -i ../patches/linuxsampler_disable-ladspa-fx.patch
    touch patched
  fi
  rm -f configure
  make -f Makefile.svn configure
  ./configure \
    --enable-static --disable-shared --prefix=${PREFIX} \
    --disable-alsa-driver --disable-arts-driver --disable-jack-driver \
    --disable-asio-driver --disable-midishare-driver --disable-mmemidi-driver \
    --disable-coreaudio-driver --disable-coremidi-driver \
    --disable-instruments-db --disable-sf2-engine
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-llinuxsampler|-llinuxsampler -L${PREFIX}/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" ${PREFIX}/lib/pkgconfig/linuxsampler.pc
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# glib

if [ ! -d ${PREFIX}/include/glib-2.0 ]; then
  cp -r /usr/include/glib-2.0 ${PREFIX}/include/
fi

if [ ! -f ${PREFIX}/lib/pkgconfig/glib-2.0.pc ]; then
  if [ -f /usr/lib/x86_64-linux-gnu/pkgconfig/glib-2.0.pc ]; then
    cp /usr/lib/x86_64-linux-gnu/pkgconfig/glib-2.0.pc ${PREFIX}/lib/pkgconfig/
    cp /usr/lib/x86_64-linux-gnu/pkgconfig/gthread-2.0.pc ${PREFIX}/lib/pkgconfig/
    cp /usr/lib/x86_64-linux-gnu/pkgconfig/libpcre.pc ${PREFIX}/lib/pkgconfig/
  else
    cp /usr/lib/pkgconfig/glib-2.0.pc ${PREFIX}/lib/pkgconfig/
    cp /usr/lib/pkgconfig/gthread-2.0.pc ${PREFIX}/lib/pkgconfig/
  fi
fi

# ---------------------------------------------------------------------------------------------------------------------
# fluidsynth

if [ ! -d fluidsynth-${FLUIDSYNTH_VERSION} ]; then
  wget --no-check-certificate https://download.sourceforge.net/fluidsynth/fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
  tar -xf fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
fi

if [ ! -f fluidsynth-${FLUIDSYNTH_VERSION}/build-done ]; then
  cd fluidsynth-${FLUIDSYNTH_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-libsndfile-support \
    --disable-dbus-support --disable-aufile-support \
    --disable-pulse-support --disable-alsa-support --disable-portaudio-support --disable-oss-support --disable-jack-support \
    --disable-coreaudio --disable-coremidi --disable-dart --disable-lash --disable-ladcca \
    --without-readline
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" ${PREFIX}/lib/pkgconfig/fluidsynth.pc
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
build_base

if [ x"${1}" != x"32" ]; then
  export ARCH=64
  build_base
fi

# ---------------------------------------------------------------------------------------------------------------------
