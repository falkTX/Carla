#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# set variables

PKG_CONFIG_VERSION=0.28
LIBLO_VERSION=0.29
ZLIB_VERSION=1.2.11
FILE_VERSION=5.32
LIBOGG_VERSION=1.3.3
LIBVORBIS_VERSION=1.3.5
FLAC_VERSION=1.3.2
LIBSNDFILE_VERSION=1.0.28
LIBGIG_VERSION=4.0.0
LINUXSAMPLER_VERSION=2.0.0
FLUIDSYNTH_VERSION=1.1.7

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ -f Makefile ]; then
  cd data/linux
fi

# ---------------------------------------------------------------------------------------------------------------------
# set target dir

TARGETDIR=$HOME/builds

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf $TARGETDIR/carla32/ $TARGETDIR/carla64/
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

export PREFIX=$TARGETDIR/carla$ARCH
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="$CFLAGS -fPIC -DPIC -DNDEBUG -I$PREFIX/include"
export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

export LDFLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
export LDFLAGS="$LDFLAGS -L$PREFIX/lib"

# ---------------------------------------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-${PKG_CONFIG_VERSION} ]; then
curl -O https://pkg-config.freedesktop.org/releases/pkg-config-${PKG_CONFIG_VERSION}.tar.gz
tar -xf pkg-config-${PKG_CONFIG_VERSION}.tar.gz
fi

if [ ! -f pkg-config-${PKG_CONFIG_VERSION}_$ARCH/build-done ]; then
cp -r pkg-config-${PKG_CONFIG_VERSION} pkg-config-${PKG_CONFIG_VERSION}_$ARCH
cd pkg-config-${PKG_CONFIG_VERSION}_$ARCH
./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-${LIBLO_VERSION} ]; then
curl -L http://download.sourceforge.net/liblo/liblo-${LIBLO_VERSION}.tar.gz -o liblo-${LIBLO_VERSION}.tar.gz
tar -xf liblo-${LIBLO_VERSION}.tar.gz
fi

if [ ! -f liblo-${LIBLO_VERSION}_$ARCH/build-done ]; then
cp -r liblo-${LIBLO_VERSION} liblo-${LIBLO_VERSION}_$ARCH
cd liblo-${LIBLO_VERSION}_$ARCH
./configure --enable-static --disable-shared --prefix=$PREFIX \
--enable-threads \
--disable-examples --disable-tools --disable-tests
make
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
curl -L https://github.com/madler/zlib/archive/v${ZLIB_VERSION}.tar.gz -o zlib-${ZLIB_VERSION}.tar.gz
tar -xf zlib-${ZLIB_VERSION}.tar.gz
fi

if [ ! -f zlib-${ZLIB_VERSION}/build-done ]; then
cd zlib-${ZLIB_VERSION}
./configure --static --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# file/magic

if [ ! -d file-${FILE_VERSION} ]; then
curl -O ftp://ftp.astron.com/pub/file/file-${FILE_VERSION}.tar.gz
tar -xf file-${FILE_VERSION}.tar.gz
fi

if [ ! -f file-${FILE_VERSION}/build-done ]; then
cd file-${FILE_VERSION}
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

return

# ---------------------------------------------------------------------------------------------------------------------
# libogg

if [ ! -d libogg-${LIBOGG_VERSION} ]; then
curl -O https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-${LIBOGG_VERSION}.tar.gz
tar -xf libogg-${LIBOGG_VERSION}.tar.gz
fi

if [ ! -f libogg-${LIBOGG_VERSION}/build-done ]; then
cd libogg-${LIBOGG_VERSION}
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-${LIBVORBIS_VERSION} ]; then
wget https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${LIBVORBIS_VERSION}.tar.gz
tar -xf libvorbis-${LIBVORBIS_VERSION}.tar.gz
fi

if [ ! -f libvorbis-${LIBVORBIS_VERSION}/build-done ]; then
cd libvorbis-${LIBVORBIS_VERSION}
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# flac

if [ ! -d flac-${FLAC_VERSION} ]; then
wget https://svn.xiph.org/releases/flac/flac-${FLAC_VERSION}.tar.xz
tar -xf flac-${FLAC_VERSION}.tar.xz
fi

if [ ! -f flac-${FLAC_VERSION}/build-done ]; then
cd flac-${FLAC_VERSION}
chmod +x configure install-sh
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-${SNDFILE_VERSION} ]; then
curl -O http://www.mega-nerd.com/libsndfile/files/libsndfile-${SNDFILE_VERSION}.tar.gz
tar -xf libsndfile-${SNDFILE_VERSION}.tar.gz
fi

if [ ! -f libsndfile-${SNDFILE_VERSION}/build-done ]; then
cd libsndfile-${SNDFILE_VERSION}
# sed -i -e "s/#include <Carbon.h>//" programs/sndfile-play.c
./configure --enable-static --disable-shared --disable-sqlite --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-${LIBGIG_VERSION} ]; then
curl -O http://download.linuxsampler.org/packages/libgig-${LIBGIG_VERSION}.tar.bz2
tar -xf libgig-${LIBGIG_VERSION}.tar.bz2
fi

if [ ! -f libgig-${LIBGIG_VERSION}/build-done ]; then
cd libgig-${LIBGIG_VERSION}
if [ ! -f patched ]; then
patch -p1 -i ../patches/libgig_fix-build.patch
touch patched
fi
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-${LINUXSAMPLER_VERSION} ]; then
curl -O http://download.linuxsampler.org/packages/linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
tar -xf linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
fi

if [ ! -f linuxsampler-${LINUXSAMPLER_VERSION}/build-done ]; then
cd linuxsampler-${LINUXSAMPLER_VERSION}
if [ ! -f patched ]; then
patch -p1 -i ../patches/linuxsampler_allow-no-drivers-build.patch
patch -p1 -i ../patches/linuxsampler_disable-ladspa-fx.patch
# sed -i -e "s/HAVE_AU/HAVE_VST/" src/hostplugins/Makefile.am
touch patched
fi
rm configure
make -f Makefile.svn configure
./configure \
--enable-static --disable-shared --prefix=$PREFIX \
--disable-alsa-driver --disable-arts-driver --disable-jack-driver \
--disable-asio-driver --disable-midishare-driver --disable-mmemidi-driver \
--disable-coreaudio-driver --disable-coremidi-driver \
--disable-instruments-db --disable-sf2-engine
# sed -i -e "s/bison (GNU Bison) //" config.h
make -j 8
make install
sed -i -e "s|-llinuxsampler|-llinuxsampler -L$PREFIX/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" $PREFIX/lib/pkgconfig/linuxsampler.pc
touch build-done
cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# glib

if [ ! -d $PREFIX/include/glib-2.0 ]; then
cp -r /usr/include/glib-2.0 $PREFIX/include/
fi

# ---------------------------------------------------------------------------------------------------------------------
# fluidsynth

if [ ! -d fluidsynth-${FLUIDSYNTH_VERSION} ]; then
curl -L http://sourceforge.net/projects/fluidsynth/files/fluidsynth-${FLUIDSYNTH_VERSION}/fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz/download -o fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
tar -xf fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
fi

if [ ! -f fluidsynth-${FLUIDSYNTH_VERSION}/build-done ]; then
cd fluidsynth-${FLUIDSYNTH_VERSION}
./configure --enable-static --disable-shared --prefix=$PREFIX \
 --disable-dbus-support --disable-aufile-support \
 --disable-pulse-support --disable-alsa-support --disable-portaudio-support --disable-oss-support --disable-jack-support \
 --disable-coreaudio --disable-coremidi --disable-dart --disable-lash --disable-ladcca \
 --without-readline \
 --enable-libsndfile-support
make
make install
sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" $PREFIX/lib/pkgconfig/fluidsynth.pc
touch build-done
cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
build_base

export ARCH=64
build_base

# ---------------------------------------------------------------------------------------------------------------------
