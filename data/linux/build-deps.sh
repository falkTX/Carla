#!/bin/bash

# NOTE: You need the following packages installed via MacPorts:
# automake, autoconf, bison, flex, libtool
# p5-libxml-perl, p5-xml-libxml, p7zip, pkgconfig

# ------------------------------------------------------------------------------------
# stop on error

set -e

# ------------------------------------------------------------------------------------
# cd to correct path

if [ -f Makefile ]; then
  cd data/linux
fi

# ------------------------------------------------------------------------------------
# set target dir

TARGETDIR=$HOME/builds

# ------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf $TARGETDIR/carla/ $TARGETDIR/carla32/ $TARGETDIR/carla64/
# rm -rf cx_Freeze-*
# rm -rf Python-*
# rm -rf PyQt-*
# rm -rf file-*
# rm -rf flac-*
# rm -rf fltk-*
# rm -rf fluidsynth-*
# rm -rf fftw-*
# rm -rf gettext-*
# rm -rf glib-*
# rm -rf libffi-*
# rm -rf libgig-*
rm -rf liblo-*
# rm -rf libogg-*
# rm -rf libsndfile-*
# rm -rf libvorbis-*
# rm -rf linuxsampler-*
# rm -rf mxml-*
rm -rf pkg-config-*
# rm -rf pyliblo-*
# rm -rf qtbase-*
# rm -rf qtmacextras-*
# rm -rf qtsvg-*
# rm -rf sip-*
# rm -rf zlib-*
# rm -rf PaxHeaders.20420

}

# ------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=gcc
export CXX=g++

export PREFIX=$TARGETDIR/carla$ARCH
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

export CFLAGS="-O2 -mtune=generic -msse -msse2 -mfpmath=sse -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="$CFLAGS -m$ARCH -fPIC -DPIC -DNDEBUG -I$PREFIX/include"
export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

export LDFLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all"
export LDFLAGS="$LDFLAGS -m$ARCH -L$PREFIX/lib"

# ------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-0.28 ]; then
curl -O https://pkg-config.freedesktop.org/releases/pkg-config-0.28.tar.gz
tar -xf pkg-config-0.28.tar.gz
fi

if [ ! -f pkg-config-0.28_$ARCH/build-done ]; then
cp -r pkg-config-0.28 pkg-config-0.28_$ARCH
cd pkg-config-0.28_$ARCH
./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-0.28 ]; then
curl -L http://download.sourceforge.net/liblo/liblo-0.28.tar.gz -o liblo-0.28.tar.gz
tar -xf liblo-0.28.tar.gz
fi

if [ ! -f liblo-0.28_$ARCH/build-done ]; then
cp -r liblo-0.28 liblo-0.28_$ARCH
cd liblo-0.28_$ARCH
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------

if [ "$ARCH" == "32" ]; then
return
fi

# ------------------------------------------------------------------------------------
# file/magic

if [ ! -d file-5.25 ]; then
curl -O ftp://ftp.astron.com/pub/file/file-5.25.tar.gz
tar -xf file-5.25.tar.gz
fi

if [ ! -f file-5.25/build-done ]; then
cd file-5.25
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-1.2.10 ]; then
curl -L https://github.com/madler/zlib/archive/v1.2.10.tar.gz -o zlib-1.2.10.tar.gz
tar -xf zlib-1.2.10.tar.gz
fi

if [ ! -f zlib-1.2.10/build-done ]; then
cd zlib-1.2.10
./configure --static --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# mxml

if [ ! -d mxml-2.11 ]; then
wget https://github.com/michaelrsweet/mxml/releases/download/v2.11/mxml-2.11.tar.gz
mkdir mxml-2.11
cd mxml-2.11
tar -xf ../mxml-2.11.tar.gz
cd ..
fi

if [ ! -f mxml-2.11/build-done ]; then
cd mxml-2.11
./configure --disable-shared --prefix=$PREFIX
make libmxml.a
cp *.a    $PREFIX/lib/
cp *.pc   $PREFIX/lib/pkgconfig/
cp mxml.h $PREFIX/include/
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libogg

if [ ! -d libogg-1.3.2 ]; then
curl -O https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-1.3.2.tar.gz
tar -xf libogg-1.3.2.tar.gz
fi

if [ ! -f libogg-1.3.2/build-done ]; then
cd libogg-1.3.2
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-1.3.5 ]; then
wget http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz
tar -xf libvorbis-1.3.5.tar.gz
fi

if [ ! -f libvorbis-1.3.5/build-done ]; then
cd libvorbis-1.3.5
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# flac

if [ ! -d flac-1.3.1 ]; then
wget https://svn.xiph.org/releases/flac/flac-1.3.1.tar.xz
tar -xf flac-1.3.1.tar.xz
fi

if [ ! -f flac-1.3.1/build-done ]; then
cd flac-1.3.1
chmod +x configure install-sh
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-1.0.26 ]; then
curl -O http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.26.tar.gz
tar -xf libsndfile-1.0.26.tar.gz
fi

if [ ! -f libsndfile-1.0.26/build-done ]; then
cd libsndfile-1.0.26
sed -i -e "s/#include <Carbon.h>//" programs/sndfile-play.c
./configure --enable-static --disable-shared --disable-sqlite --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-4.0.0 ]; then
curl -O http://download.linuxsampler.org/packages/libgig-4.0.0.tar.bz2
tar -xf libgig-4.0.0.tar.bz2
fi

if [ ! -f libgig-4.0.0/build-done ]; then
cd libgig-4.0.0
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

# ------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-2.0.0 ]; then
curl -O http://download.linuxsampler.org/packages/linuxsampler-2.0.0.tar.bz2
tar -xf linuxsampler-2.0.0.tar.bz2
fi

if [ ! -f linuxsampler-2.0.0/build-done ]; then
cd linuxsampler-2.0.0
if [ ! -f patched ]; then
patch -p1 -i ../patches/linuxsampler_allow-no-drivers-build.patch
patch -p1 -i ../patches/linuxsampler_disable-ladspa-fx.patch
sed -i -e "s/HAVE_AU/HAVE_VST/" src/hostplugins/Makefile.am
touch patched
fi
rm configure
make -f Makefile.svn configure
./configure \
--enable-static --disable-shared --prefix=$PREFIX \
--disable-alsa-driver \
--disable-arts-driver --disable-artstest --disable-instruments-db \
--disable-asio-driver --disable-midishare-driver --disable-coremidi-driver --disable-coreaudio-driver --disable-mmemidi-driver
sed -i -e "s/bison (GNU Bison) //" config.h
make -j 8
make install
sed -i -e "s|-llinuxsampler|-llinuxsampler -L$PREFIX/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -lpthread|" $PREFIX/lib/pkgconfig/linuxsampler.pc
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# fltk

if [ ! -d fltk-1.3.3 ]; then
curl -O http://fltk.org/pub/fltk/1.3.3/fltk-1.3.3-source.tar.gz
tar -xf fltk-1.3.3-source.tar.gz
fi

if [ ! -f fltk-1.3.3/build-done ]; then
cd fltk-1.3.3
./configure --prefix=$PREFIX \
--disable-shared --disable-debug \
--disable-threads --disable-gl \
--enable-localjpeg --enable-localpng
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# glib

if [ ! -d $PREFIX/include/glib-2.0 ]; then
cp -r /usr/include/glib-2.0 $PREFIX/include/
fi

# ------------------------------------------------------------------------------------
# fluidsynth

if [ ! -d fluidsynth-1.1.6 ]; then
curl -L http://sourceforge.net/projects/fluidsynth/files/fluidsynth-1.1.6/fluidsynth-1.1.6.tar.gz/download -o fluidsynth-1.1.6.tar.gz
tar -xf fluidsynth-1.1.6.tar.gz
fi

if [ ! -f fluidsynth-1.1.6/build-done ]; then
cd fluidsynth-1.1.6
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

# ------------------------------------------------------------------------------------
# fftw3 (needs to be last as it modifies C[XX]FLAGS)

if [ ! -d fftw-3.3.4 ]; then
curl -O http://www.fftw.org/fftw-3.3.4.tar.gz
tar -xf fftw-3.3.4.tar.gz
fi

if [ ! -f fftw-3.3.4/build-done ]; then
export CFLAGS="$CFLAGS -O3 -ffast-math"
export CXXFLAGS="$CXXFLAGS -O3 -ffast-math"
cd fftw-3.3.4
./configure --enable-static --enable-sse2 --disable-shared --disable-debug --prefix=$PREFIX
make
make install
make clean
./configure --enable-static --enable-sse --enable-sse2 --enable-single --disable-shared --disable-debug --prefix=$PREFIX
make
make install
make clean
touch build-done
cd ..
fi

}

# ------------------------------------------------------------------------------------
# build base libs

export ARCH=32
build_base

export ARCH=64
build_base

# ------------------------------------------------------------------------------------
