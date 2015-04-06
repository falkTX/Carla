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
# function to remove old stuff

cleanup()
{

sudo rm -rf /opt/carla32/ /opt/carla64/
sudo rm -rf cx_Freeze-*
sudo rm -rf Python-*
rm -rf fftw-*
rm -rf liblo-*
rm -rf mxml-*
rm -rf pkg-config-*
rm -rf PyQt-*
rm -rf qtbase-*
rm -rf qtmacextras-*
rm -rf qtsvg-*
rm -rf sip-*
rm -rf zlib-*

}

# ------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=gcc
export CXX=g++

export CFLAGS="-O2 -mtune=generic -msse -msse2 -m$ARCH -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m$ARCH"

export PREFIX=/opt/carla$ARCH
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

# ------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-0.28 ]; then
curl -O http://pkgconfig.freedesktop.org/releases/pkg-config-0.28.tar.gz
tar -xf pkg-config-0.28.tar.gz
fi

if [ ! -f pkg-config-0.28_$ARCH/build-done ]; then
cp -r pkg-config-0.28 pkg-config-0.28_$ARCH
cd pkg-config-0.28_$ARCH
./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=$PREFIX
make
sudo make install
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
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------

if [ "$ARCH" == "32" ]; then
return
fi

# ------------------------------------------------------------------------------------
# file/magic

if [ ! -d file-5.22 ]; then
curl -O ftp://ftp.astron.com/pub/file/file-5.22.tar.gz
tar -xf file-5.22.tar.gz
fi

if [ ! -f file-5.22/build-done ]; then
cd file-5.22
./configure --enable-static --disable-shared --prefix=$PREFIX
make
sudo make install
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
sudo make install
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
./configure --enable-static --disable-shared --prefix=$PREFIX
make
sudo cp *.a    $PREFIX/lib/
sudo cp *.pc   $PREFIX/lib/pkgconfig/
sudo cp mxml.h $PREFIX/include/
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
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-1.3.4 ]; then
curl -O http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.4.tar.gz
tar -xf libvorbis-1.3.4.tar.gz
fi

if [ ! -f libvorbis-1.3.4/build-done ]; then
cd libvorbis-1.3.4
./configure --enable-static --disable-shared --prefix=$PREFIX
make
sudo make install
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
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-1.0.25 ]; then
curl -O http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz
tar -xf libsndfile-1.0.25.tar.gz
fi

if [ ! -f libsndfile-1.0.25/build-done ]; then
cd libsndfile-1.0.25
sed -i -e "s/#include <Carbon.h>//" programs/sndfile-play.c
./configure --enable-static --disable-shared --disable-sqlite --prefix=$PREFIX
make
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-svn ]; then
/opt/local/bin/svn co https://svn.linuxsampler.org/svn/libgig/trunk libgig-svn -r 2593
fi

if [ ! -f libgig-svn/build-done ]; then
cd libgig-svn
env PATH=/opt/local/bin:$PATH /opt/local/bin/aclocal -I /opt/local/share/aclocal
env PATH=/opt/local/bin:$PATH /opt/local/bin/glibtoolize --force --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoheader
env PATH=/opt/local/bin:$PATH /opt/local/bin/automake --add-missing --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoconf
env PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=$PREFIX
env PATH=/opt/local/bin:$PATH make
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-svn ]; then
/opt/local/bin/svn co https://svn.linuxsampler.org/svn/linuxsampler/trunk linuxsampler-svn -r 2593
fi

if [ ! -f linuxsampler-svn/build-done ]; then
cd linuxsampler-svn
mkdir -p tmp
cd tmp
curl -L https://launchpad.net/~kxstudio-debian/+archive/ubuntu/libs/+files/linuxsampler-static_1.0.0%2Bsvn2677-1kxstudio1.debian.tar.xz -o linuxsampler-static.debian.tar.xz
/opt/local/bin/7z x linuxsampler-static.debian.tar.xz
/opt/local/bin/7z x linuxsampler-static.debian.tar
cd ..
mv tmp/debian/patches/*.patch .
patch -p1 < allow-no-drivers-build.patch
patch -p1 < disable-ladspa-fx.patch
rm -r *.patch tmp/
sed -i -e "s/HAVE_AU/HAVE_VST/" src/hostplugins/Makefile.am
env PATH=/opt/local/bin:$PATH /opt/local/bin/aclocal -I /opt/local/share/aclocal
env PATH=/opt/local/bin:$PATH /opt/local/bin/glibtoolize --force --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoheader
env PATH=/opt/local/bin:$PATH /opt/local/bin/automake --add-missing --copy
env PATH=/opt/local/bin:$PATH /opt/local/bin/autoconf
env PATH=/opt/local/bin:$PATH ./configure \
--enable-static --disable-shared --prefix=$PREFIX \
--disable-arts-driver --disable-artstest \
--disable-asio-driver --disable-midishare-driver --disable-coremidi-driver --disable-coreaudio-driver --disable-mmemidi-driver
env PATH=/opt/local/bin:$PATH ./scripts/generate_instrument_script_parser.sh
sed -i -e "s/bison (GNU Bison) //" config.h
env PATH=/opt/local/bin:$PATH make
sudo make install
sudo sed -i -e "s|-llinuxsampler|-llinuxsampler -L/opt/carla64/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -lpthread -lsqlite3|" /opt/carla64/lib/pkgconfig/linuxsampler.pc
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
sudo make install
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
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# glib

if [ ! -d glib-2.42.1 ]; then
curl -O http://ftp.gnome.org/pub/GNOME/sources/glib/2.42/glib-2.42.1.tar.xz
/opt/local/bin/7z x glib-2.42.1.tar.xz
/opt/local/bin/7z x glib-2.42.1.tar
fi

if [ ! -f glib-2.42.1/build-done ]; then
cd glib-2.42.1
chmod +x configure install-sh
env CFLAGS="$CFLAGS -I$PREFIX/include" LDFLAGS="$LDFLAGS -L$PREFIX/lib" PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=$PREFIX
env PATH=/opt/local/bin:$PATH make
sudo touch /opt/carla64/bin/gtester-report
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# fltk

ignore1()
{

if [ ! -d fltk-1.3.3 ]; then
curl -O http://fltk.org/pub/fltk/1.3.3/fltk-1.3.3-source.tar.gz
tar -xf fltk-1.3.3-source.tar.gz
fi

if [ ! -f fltk-1.3.3/build-done ]; then
cd fltk-1.3.3
./configure --disable-shared --disable-debug --prefix=$PREFIX \
--enable-threads --disable-gl \
--enable-localjpeg --enable-localzlib  --enable-localpng
make
sudo make install
touch build-done
cd ..
fi

}

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
sudo make install
sudo sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm -liconv -lintl|" /opt/carla64/lib/pkgconfig/fluidsynth.pc
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# fftw3

if [ ! -d fftw-3.3.4 ]; then
curl -O http://www.fftw.org/fftw-3.3.4.tar.gz
tar -xf fftw-3.3.4.tar.gz
fi

if [ ! -f fftw-3.3.4/build-done ]; then
export CFLAGS="-O2 -mtune=generic -msse -msse2 -ffast-math -mfpmath=sse -m$ARCH -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m$ARCH"
cd fftw-3.3.4
./configure --enable-static --enable-sse2 --disable-shared --disable-debug --prefix=$PREFIX
make
sudo make install
make clean
./configure --enable-static --enable-sse --enable-sse2 --enable-single --disable-shared --disable-debug --prefix=$PREFIX
make
sudo make install
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
# switch to clang for Qt5

export CC=clang
export CXX=clang

# ------------------------------------------------------------------------------------
# set flags for qt stuff

export CFLAGS="-O2 -mtune=generic -msse -msse2 -m64 -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
export LDFLAGS="-m64"

export PREFIX=/opt/carla
export PATH=$PREFIX/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

export CFG_ARCH=x86_64
export QMAKESPEC=macx-clang

# ------------------------------------------------------------------------------------
# qt5-base download

if [ ! -d qtbase-opensource-src-5.3.2 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtbase-opensource-src-5.3.2.tar.gz -o qtbase-opensource-src-5.3.2.tar.gz
tar -xf qtbase-opensource-src-5.3.2.tar.gz
fi

# ------------------------------------------------------------------------------------
# qt5-base (64bit, shared, framework)

if [ ! -f qtbase-opensource-src-5.3.2/build-done ]; then
cd qtbase-opensource-src-5.3.2
./configure -release -shared -opensource -confirm-license -force-pkg-config -platform macx-clang -framework \
            -prefix $PREFIX -plugindir $PREFIX/lib/qt5/plugins -headerdir $PREFIX/include/qt5 \
            -qt-freetype -qt-libjpeg -qt-libpng -qt-pcre -qt-sql-sqlite -qt-zlib -opengl desktop -qpa cocoa \
            -no-directfb -no-eglfs -no-kms -no-linuxfb -no-mtdev -no-xcb -no-xcb-xlib \
            -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-mips_dsp -no-mips_dspr2 \
            -no-cups -no-dbus -no-evdev -no-fontconfig -no-harfbuzz -no-iconv -no-icu -no-gif -no-glib -no-nis -no-openssl -no-pch -no-sql-ibase -no-sql-odbc \
            -no-audio-backend -no-qml-debug -no-separate-debug-info \
            -no-compile-examples -nomake examples -nomake tests -make libs -make tools
make -j 2
sudo make install
sudo ln -s /opt/carla/lib/QtCore.framework/Headers    /opt/carla/include/qt5/QtCore
sudo ln -s /opt/carla/lib/QtGui.framework/Headers     /opt/carla/include/qt5/QtGui
sudo ln -s /opt/carla/lib/QtWidgets.framework/Headers /opt/carla/include/qt5/QtWidgets
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-mac-extras

if [ ! -d qtmacextras-opensource-src-5.3.2 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtmacextras-opensource-src-5.3.2.tar.gz -o qtmacextras-opensource-src-5.3.2.tar.gz
tar -xf qtmacextras-opensource-src-5.3.2.tar.gz
fi

if [ ! -f qtmacextras-opensource-src-5.3.2/build-done ]; then
cd qtmacextras-opensource-src-5.3.2
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-5.3.2 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtsvg-opensource-src-5.3.2.tar.gz -o qtsvg-opensource-src-5.3.2.tar.gz
tar -xf qtsvg-opensource-src-5.3.2.tar.gz
fi

if [ ! -f qtsvg-opensource-src-5.3.2/build-done ]; then
cd qtsvg-opensource-src-5.3.2
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-multimedia

ignore2()
{

if [ ! -d qtmultimedia-opensource-src-5.3.2 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtmultimedia-opensource-src-5.3.2.tar.gz -o qtmultimedia-opensource-src-5.3.2.tar.gz
tar -xf qtmultimedia-opensource-src-5.3.2.tar.gz
fi

if [ ! -f qtmultimedia-opensource-src-5.3.2/build-done ]; then
cd qtmultimedia-opensource-src-5.3.2
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

}

# ------------------------------------------------------------------------------------
# qt5-webkit

ignore3()
{

if [ ! -d qtwebkit-opensource-src-5.3.2 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtwebkit-opensource-src-5.3.2.tar.gz -o qtwebkit-opensource-src-5.3.2.tar.gz
tar -xf qtwebkit-opensource-src-5.3.2.tar.gz
fi

if [ ! -f qtwebkit-opensource-src-5.3.2/build-done ]; then
cd qtwebkit-opensource-src-5.3.2
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

}

# ------------------------------------------------------------------------------------
# python

if [ ! -d Python-3.4.2 ]; then
curl -O https://www.python.org/ftp/python/3.4.2/Python-3.4.2.tgz
tar -xf Python-3.4.2.tgz
fi

if [ ! -f Python-3.4.2/build-done ]; then
cd Python-3.4.2
./configure --prefix=$PREFIX
make
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# sip

if [ ! -d sip-4.16.5 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/sip/sip-4.16.5/sip-4.16.5.tar.gz -o sip-4.16.5.tar.gz
tar -xf sip-4.16.5.tar.gz
fi

if [ ! -f sip-4.16.5/build-done ]; then
cd sip-4.16.5
python3 configure.py
make
sudo make install
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
export OLD_CFLAGS="$CFLAGS"
export OLD_LDFLAGS="$LDFLAGS"
export CFLAGS="$CFLAGS -I/opt/carla64/include"
export LDFLAGS="$LDFLAGS -L/opt/carla64/lib"
python3 setup.py build
sudo python3 setup.py install --prefix=$PREFIX
touch build-done
export CFLAGS="$OLD_CFLAGS"
export LDFLAGS="$OLD_LDFLAGS"
unset OLD_CFLAGS
unset OLD_LDFLAGS
cd ..
fi

# ------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-5.3.2 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-5.3.2/PyQt-gpl-5.3.2.tar.gz -o PyQt-gpl-5.3.2.tar.gz
tar -xf PyQt-gpl-5.3.2.tar.gz
fi

if [ ! -f PyQt-gpl-5.3.2/build-done ]; then
cd PyQt-gpl-5.3.2
sed -i -e "s/# Read the details./pylib_dir = ''/" configure.py
sed -i -e "s/qmake_QT=['webkitwidgets']/qmake_QT=['webkitwidgets', 'printsupport']/" configure.py
python3 configure.py --confirm-license -c
make
sudo make install
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
sed  -i -e 's/"python%s.%s"/"python%s.%sm"/' setup.py
python3 setup.py build
sudo python3 setup.py install --prefix=$PREFIX
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
