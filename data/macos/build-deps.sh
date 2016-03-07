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
  cd data/macos
fi

# ------------------------------------------------------------------------------------
# set target dir

TARGETDIR=$HOME/builds

# ------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf $TARGETDIR/carla/ $TARGETDIR/carla32/ $TARGETDIR/carla64/
rm -rf cx_Freeze-*
rm -rf Python-*
rm -rf PyQt-*
rm -rf file-*
rm -rf flac-*
rm -rf fltk-*
rm -rf fluidsynth-*
rm -rf fftw-*
rm -rf gettext-*
rm -rf glib-*
rm -rf libffi-*
rm -rf libgig-*
rm -rf liblo-*
rm -rf libogg-*
rm -rf libsndfile-*
rm -rf libvorbis-*
rm -rf linuxsampler-*
rm -rf mxml-*
rm -rf pkg-config-*
rm -rf pyliblo-*
rm -rf qtbase-*
rm -rf qtmacextras-*
rm -rf qtsvg-*
rm -rf sip-*
rm -rf zlib-*
rm -rf PaxHeaders.20420

}

# ------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=clang
export CXX=clang++

export PREFIX=$TARGETDIR/carla$ARCH
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

export CFLAGS="-O2 -mtune=generic -msse -msse2 -m$ARCH -fPIC -DPIC -I$PREFIX/include"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m$ARCH -L$PREFIX/lib"

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

if [ ! -d zlib-1.2.8 ]; then
curl -O http://zlib.net/zlib-1.2.8.tar.gz
tar -xf zlib-1.2.8.tar.gz
fi

if [ ! -f zlib-1.2.8/build-done ]; then
cd zlib-1.2.8
./configure --static --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# mxml

if [ ! -d mxml-2.9 ]; then
curl -O http://www.msweet.org/files/project3/mxml-2.9.tar.gz
tar -xf mxml-2.9.tar.gz
fi

if [ ! -f mxml-2.9/build-done ]; then
cd mxml-2.9
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
curl -O http://downloads.xiph.org/releases/ogg/libogg-1.3.2.tar.gz
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
curl -O http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz
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
curl -O https://svn.xiph.org/releases/flac/flac-1.3.1.tar.xz
/opt/local/bin/7z x flac-1.3.1.tar.xz
/opt/local/bin/7z x flac-1.3.1.tar
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
env PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=$PREFIX
env PATH=/opt/local/bin:$PATH make
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
env PATH=/opt/local/bin:$PATH /opt/local/bin/aclocal -I /opt/local/share/aclocal
env PATH=/opt/local/bin:$PATH /opt/local/bin/glibtoolize --force --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoheader
env PATH=/opt/local/bin:$PATH /opt/local/bin/automake --add-missing --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoconf
env PATH=/opt/local/bin:$PATH ./configure \
--enable-static --disable-shared --prefix=$PREFIX \
--disable-arts-driver --disable-artstest --disable-instruments-db \
--disable-asio-driver --disable-midishare-driver --disable-coremidi-driver --disable-coreaudio-driver --disable-mmemidi-driver
env PATH=/opt/local/bin:$PATH ./scripts/generate_instrument_script_parser.sh
sed -i -e "s/bison (GNU Bison) //" config.h
env PATH=/opt/local/bin:$PATH make
make install
sed -i -e "s|-llinuxsampler|-llinuxsampler -L$PREFIX/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -lpthread|" $PREFIX/lib/pkgconfig/linuxsampler.pc
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libffi

if [ ! -d libffi-3.2.1 ]; then
curl -O ftp://sourceware.org/pub/libffi/libffi-3.2.1.tar.gz
tar -xf libffi-3.2.1.tar.gz
fi

if [ ! -f libffi-3.2.1/build-done ]; then
cd libffi-3.2.1
./configure --enable-static --disable-shared --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# gettext

if [ ! -d gettext-0.18.3.2 ]; then
curl -O http://ftp.gnu.org/gnu/gettext/gettext-0.18.3.2.tar.gz
tar -xf gettext-0.18.3.2.tar.gz
fi

if [ ! -f gettext-0.18.3.2/build-done ]; then
cd gettext-0.18.3.2
env PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=$PREFIX
env PATH=/opt/local/bin:$PATH make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# glib

if [ ! -d glib-2.42.2 ]; then
curl -O http://gemmei.acc.umu.se/pub/GNOME/sources/glib/2.42/glib-2.42.2.tar.xz
/opt/local/bin/7z x glib-2.42.2.tar.xz
/opt/local/bin/7z x glib-2.42.2.tar
fi

if [ ! -f glib-2.42.2/build-done ]; then
cd glib-2.42.2
chmod +x configure install-sh
env PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=$PREFIX
env PATH=/opt/local/bin:$PATH make
touch $PREFIX/bin/gtester-report
make install
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
# fluidsynth

if [ ! -d fluidsynth-1.1.6 ]; then
curl -L http://sourceforge.net/projects/fluidsynth/files/fluidsynth-1.1.6/fluidsynth-1.1.6.tar.gz/download -o fluidsynth-1.1.6.tar.gz
tar -xf fluidsynth-1.1.6.tar.gz
fi

if [ ! -f fluidsynth-1.1.6/build-done ]; then
cd fluidsynth-1.1.6
env LDFLAGS="$LDFLAGS -framework Carbon -framework CoreFoundation" \
 ./configure --enable-static --disable-shared --prefix=$PREFIX \
 --disable-dbus-support --disable-aufile-support \
 --disable-pulse-support --disable-alsa-support --disable-portaudio-support --disable-oss-support --disable-jack-support \
 --disable-coreaudio --disable-coremidi --disable-dart --disable-lash --disable-ladcca \
 --without-readline \
 --enable-libsndfile-support
make
make install
sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm -liconv -lintl|" $PREFIX/lib/pkgconfig/fluidsynth.pc
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
export CFLAGS="-O3 -mtune=generic -msse -msse2 -ffast-math -mfpmath=sse -m$ARCH -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m$ARCH"
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
# set flags for qt stuff

export CFLAGS="-O2 -mtune=generic -msse -msse2 -m64 -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m64"

export PREFIX=$TARGETDIR/carla
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

export CFG_ARCH=x86_64
export QMAKESPEC=macx-clang

# ------------------------------------------------------------------------------------
# qt5-base download

if [ ! -d qtbase-opensource-src-5.5.1 ]; then
curl -L http://download.qt.io/official_releases/qt/5.5/5.5.1/submodules/qtbase-opensource-src-5.5.1.tar.gz -o qtbase-opensource-src-5.5.1.tar.gz
tar -xf qtbase-opensource-src-5.5.1.tar.gz
fi

# ------------------------------------------------------------------------------------
# qt5-base (64bit, shared, framework)

if [ ! -f qtbase-opensource-src-5.5.1/build-done ]; then
cd qtbase-opensource-src-5.5.1
if [ ! -f configured ]; then
./configure -release -shared -opensource -confirm-license -force-pkg-config -platform macx-clang -framework \
            -prefix $PREFIX -plugindir $PREFIX/lib/qt5/plugins -headerdir $PREFIX/include/qt5 \
            -qt-freetype -qt-libjpeg -qt-libpng -qt-pcre -opengl desktop -qpa cocoa \
            -no-directfb -no-eglfs -no-kms -no-linuxfb -no-mtdev -no-xcb -no-xcb-xlib \
            -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-mips_dsp -no-mips_dspr2 \
            -no-cups -no-dbus -no-evdev -no-fontconfig -no-harfbuzz -no-gif -no-glib -no-nis -no-openssl -no-pch -no-sql-ibase -no-sql-odbc \
            -no-audio-backend -no-qml-debug -no-separate-debug-info \
            -no-compile-examples -nomake examples -nomake tests -make libs -make tools
touch configured
fi
make -j 2
make install
ln -s $PREFIX/lib/QtCore.framework/Headers    $PREFIX/include/qt5/QtCore
ln -s $PREFIX/lib/QtGui.framework/Headers     $PREFIX/include/qt5/QtGui
ln -s $PREFIX/lib/QtWidgets.framework/Headers $PREFIX/include/qt5/QtWidgets
sed -i -e "s/ -lqtpcre/ /" $PREFIX/lib/pkgconfig/Qt5Core.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Core.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Core.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Gui.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Gui.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Widgets.pc
sed -i -e "s/ '/ /" $PREFIX/lib/pkgconfig/Qt5Widgets.pc
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-mac-extras

if [ ! -d qtmacextras-opensource-src-5.5.1 ]; then
curl -L http://download.qt.io/official_releases/qt/5.5/5.5.1/submodules/qtmacextras-opensource-src-5.5.1.tar.gz -o qtmacextras-opensource-src-5.5.1.tar.gz
tar -xf qtmacextras-opensource-src-5.5.1.tar.gz
fi

if [ ! -f qtmacextras-opensource-src-5.5.1/build-done ]; then
cd qtmacextras-opensource-src-5.5.1
qmake
make -j 2
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-5.5.1 ]; then
curl -L http://download.qt.io/official_releases/qt/5.5/5.5.1/submodules/qtsvg-opensource-src-5.5.1.tar.gz -o qtsvg-opensource-src-5.5.1.tar.gz
tar -xf qtsvg-opensource-src-5.5.1.tar.gz
fi

if [ ! -f qtsvg-opensource-src-5.5.1/build-done ]; then
cd qtsvg-opensource-src-5.5.1
qmake
make -j 2
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# python

if [ ! -d Python-3.4.4 ]; then
curl -O https://www.python.org/ftp/python/3.4.4/Python-3.4.4.tgz
tar -xf Python-3.4.4.tgz
fi

if [ ! -f Python-3.4.4/build-done ]; then
cd Python-3.4.4
./configure --prefix=$PREFIX
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# sip

if [ ! -d sip-4.17 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/sip/sip-4.17/sip-4.17.tar.gz -o sip-4.17.tar.gz
tar -xf sip-4.17.tar.gz
fi

if [ ! -f sip-4.17/build-done ]; then
cd sip-4.17
python3 configure.py
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# pyliblo

if [ ! -d pyliblo-0.9.2 ]; then
curl -O http://das.nasophon.de/download/pyliblo-0.9.2.tar.gz
tar -xf pyliblo-0.9.2.tar.gz
fi

if [ ! -f pyliblo-0.9.2/build-done ]; then
cd pyliblo-0.9.2
env CFLAGS="$CFLAGS -I$TARGETDIR/carla64/include" LDFLAGS="$LDFLAGS -L$TARGETDIR/carla64/lib" python3 setup.py build
python3 setup.py install --prefix=$PREFIX
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-5.5.1 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-5.5.1/PyQt-gpl-5.5.1.tar.gz -o PyQt-gpl-5.5.1.tar.gz
tar -xf PyQt-gpl-5.5.1.tar.gz
fi

if [ ! -f PyQt-gpl-5.5.1/build-done ]; then
cd PyQt-gpl-5.5.1
python3 configure.py --confirm-license -c
make
make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# cxfreeze

if [ ! -d cx_Freeze-4.3.3 ]; then
curl -L http://download.sourceforge.net/cx-freeze/cx_Freeze-4.3.3.tar.gz -o cx_Freeze-4.3.3.tar.gz
tar -xf cx_Freeze-4.3.3.tar.gz
fi

if [ ! -f cx_Freeze-4.3.3/build-done ]; then
cd cx_Freeze-4.3.3
sed -i -e 's/"python%s.%s"/"python%s.%sm"/' setup.py
python3 setup.py build
python3 setup.py install --prefix=$PREFIX
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
