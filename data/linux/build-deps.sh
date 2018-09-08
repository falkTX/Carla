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

rm -rf ${TARGETDIR}/carla32/ ${TARGETDIR}/carla64/
rm -rf file-*
rm -rf flac-*
rm -rf fltk-*
rm -rf fluidsynth-*
rm -rf liblo-*
rm -rf libogg-*
rm -rf libsndfile-*
rm -rf libvorbis-*
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
  aria2c https://pkg-config.freedesktop.org/releases/pkg-config-${PKG_CONFIG_VERSION}.tar.gz
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
  aria2c https://download.sourceforge.net/liblo/liblo-${LIBLO_VERSION}.tar.gz
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

if [ x"${ARCH}" = x"32" ] && [ x"${TARGET}" != x"32" ]; then
  return
fi

# ---------------------------------------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-${ZLIB_VERSION} ]; then
  aria2c https://github.com/madler/zlib/archive/v${ZLIB_VERSION}.tar.gz
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
  aria2c https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-${LIBOGG_VERSION}.tar.gz
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
  aria2c https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${LIBVORBIS_VERSION}.tar.gz
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
  aria2c https://ftp.osuosl.org/pub/xiph/releases/flac/flac-${FLAC_VERSION}.tar.xz
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
  aria2c https://github.com/FluidSynth/fluidsynth/archive/v${FLUIDSYNTH_VERSION}.tar.gz
  tar -xf fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
fi

if [ ! -f fluidsynth-${FLUIDSYNTH_VERSION}/build-done ]; then
  cd fluidsynth-${FLUIDSYNTH_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../../patches/fluidsynth-skip-drivers-build.patch
    touch patched
  fi
  sed -i "s/3.0.2/2.8.0/" CMakeLists.txt
  sed -i 's/_init_lib_suffix "64"/_init_lib_suffix ""/' CMakeLists.txt
  cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${PREFIX} -DBUILD_SHARED_LIBS=OFF \
    -Denable-debug=OFF -Denable-profiling=OFF -Denable-ladspa=OFF -Denable-fpe-check=OFF -Denable-portaudio=OFF \
    -Denable-trap-on-fpe=OFF -Denable-aufile=OFF -Denable-dbus=OFF -Denable-ipv6=OFF -Denable-jack=OFF \
    -Denable-midishare=OFF -Denable-oss=OFF -Denable-pulseaudio=OFF -Denable-readline=OFF -Denable-ladcca=OFF \
    -Denable-lash=OFF -Denable-alsa=OFF -Denable-coreaudio=OFF -Denable-coremidi=OFF -Denable-framework=OFF \
    -Denable-floats=ON
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" ${PREFIX}/lib/pkgconfig/fluidsynth.pc
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# mxml

if [ ! -d mxml-${MXML_VERSION} ]; then
  aria2c https://github.com/michaelrsweet/mxml/releases/download/v${MXML_VERSION}/mxml-${MXML_VERSION}.tar.gz
  mkdir mxml-${MXML_VERSION}
  cd mxml-${MXML_VERSION}
  tar -xf ../mxml-${MXML_VERSION}.tar.gz
  cd ..
fi

if [ ! -f mxml-${MXML_VERSION}/build-done ]; then
  cd mxml-${MXML_VERSION}
  ./configure --disable-shared --prefix=$PREFIX
  make libmxml.a
  cp *.a    $PREFIX/lib/
  cp *.pc   $PREFIX/lib/pkgconfig/
  cp mxml.h $PREFIX/include/
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# fftw3 (needs to be last as it modifies C[XX]FLAGS)

if [ ! -d fftw-${FFTW3_VERSION} ]; then
  aria2c http://www.fftw.org/fftw-${FFTW3_VERSION}.tar.gz
  tar -xf fftw-${FFTW3_VERSION}.tar.gz
fi

if [ ! -f fftw-${FFTW3_VERSION}/build-done ]; then
  export CFLAGS="${CFLAGS} -ffast-math"
  export CXXFLAGS="${CXXFLAGS} -ffast-math"
  cd fftw-${FFTW3_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-sse2 \
    --disable-debug --disable-alloca --disable-fortran \
    --with-our-malloc
  make
  make install
  make clean
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-sse2 --enable-sse --enable-single \
    --disable-debug --disable-alloca --disable-fortran \
    --with-our-malloc
  make
  make install
  make clean
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
export TARGET="${1}"
build_base

if [ x"${TARGET}" != x"32" ]; then
  export ARCH=64
  build_base
fi

# ---------------------------------------------------------------------------------------------------------------------
