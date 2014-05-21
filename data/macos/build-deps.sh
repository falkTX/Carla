#!/bin/bash

# ------------------------------------------------------------------------------------
# stop on error

set -e

# ------------------------------------------------------------------------------------
# cd to correct path

if [ -f Makefile ]; then
  cd data/macos
fi

# ------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=gcc
export CXX=g++

export CFLAGS="-O2 -mtune=generic -msse -msse2 -m$ARCH -fPIC -DPIC"
export CXXFLAGS=$CFLAGS

export PREFIX=/opt/kxstudio$ARCH
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
# zlib

if [ ! -d zlib-1.2.8 ]; then
curl -O http://zlib.net/zlib-1.2.8.tar.gz
tar -xf zlib-1.2.8.tar.gz
fi

if [ ! -f zlib-1.2.8_$ARCH/build-done ]; then
cp -r zlib-1.2.8 zlib-1.2.8_$ARCH
cd zlib-1.2.8_$ARCH
./configure --static --prefix=$PREFIX
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
# mxml

if [ ! -d mxml-2.8 ]; then
curl -O http://www.msweet.org/files/project3/mxml-2.8.tar.gz
tar -xf mxml-2.8.tar.gz
fi

if [ ! -f mxml-2.8_$ARCH/build-done ]; then
cp -r mxml-2.8 mxml-2.8_$ARCH
cd mxml-2.8_$ARCH
./configure --enable-static --disable-shared --prefix=$PREFIX
make
sudo cp *.a    $PREFIX/lib/
sudo cp *.pc   $PREFIX/lib/pkgconfig/
sudo cp mxml.h $PREFIX/include/
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# fftw3

if [ ! -d fftw-3.3.4 ]; then
curl -O http://www.fftw.org/fftw-3.3.4.tar.gz
tar -xf fftw-3.3.4.tar.gz
fi

if [ ! -f fftw-3.3.4_$ARCH/build-done ]; then
export CFLAGS="-O2 -mtune=generic -msse -msse2 -ffast-math -mfpmath=sse -m$ARCH -fPIC -DPIC"
export CXXFLAGS=$CFLAGS
cp -r fftw-3.3.4 fftw-3.3.4_$ARCH
cd fftw-3.3.4_$ARCH
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

# TODO - add qt (for 32bit and 64bit)

}

# ------------------------------------------------------------------------------------
# build base libs

export $ARCH=32
build_base

export $ARCH=64
build_base

# TODO
exit 0

# ------------------------------------------------------------------------------------
# python

if [ ! -d Python-3.3.5 ]; then
curl -O https://www.python.org/ftp/python/3.3.5/Python-3.3.5.tgz
tar -xf Python-3.3.5.tgz
fi

if [ ! -f Python-3.3.5/Makefile ]; then
cd Python-3.3.5
./configure --prefix=/opt/kxstudio
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# cxfreeze

if [ ! -d cx_Freeze-4.3.3 ]; then
curl -L http://download.sourceforge.net/cx-freeze/cx_Freeze-4.3.3.tar.gz -o cx_Freeze-4.3.3.tar.gz
tar -xf cx_Freeze-4.3.3.tar.gz
fi

if [ ! -d cx_Freeze-4.3.3/build ]; then
cd cx_Freeze-4.3.3
python3 setup.py build
sudo python3 setup.py install --prefix=/opt/kxstudio
cd ..
fi

# ------------------------------------------------------------------------------------
# sip

if [ ! -d sip-4.15.5 ]; then
curl -L http://download.sourceforge.net/pyqt/sip-4.15.5.tar.gz -o sip-4.15.5.tar.gz
tar -xf sip-4.15.5.tar.gz
fi

if [ ! -f sip-4.15.5/Makefile ]; then
cd sip-4.15.5
python3 configure.py
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# switch to clang for Qt

export CC=clang
export CXX=clang

# ------------------------------------------------------------------------------------
# qt5-base

if [ ! -d qtbase5-mac10.6 ]; then
/opt/local/bin/git clone git://github.com/falkTX/qtbase5-mac10.6 --depth 1
fi

if [ ! -f qtbase5-mac10.6/bin/moc ]; then
cd qtbase5-mac10.6
export QMAKESPEC=macx-g++42
./configure -release -shared -opensource -confirm-license -force-pkg-config \
            -prefix /opt/kxstudio -plugindir /opt/kxstudio/lib/qt5/plugins -headerdir /opt/kxstudio/include/qt5 \
            -qt-freetype -qt-libjpeg -qt-libpng -qt-pcre -qt-sql-sqlite -qt-zlib -no-framework -opengl desktop -qpa cocoa \
            -no-directfb -no-eglfs -no-kms -no-linuxfb -no-mtdev -no-xcb -no-xcb-xlib \
            -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-neon -no-mips_dsp -no-mips_dspr2 \
            -no-cups -no-dbus -no-fontconfig -no-harfbuzz -no-iconv -no-icu -no-gif -no-glib -no-nis -no-openssl -no-pch -no-sql-ibase -no-sql-odbc \
            -no-audio-backend -no-javascript-jit -no-qml-debug -no-separate-debug-info \
            -no-compile-examples -nomake examples -nomake tests -make libs -make tools
make -j 2
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-5.3.0 ]; then
curl -O http://download.qt-project.org/official_releases/qt/5.3/5.3.0/submodules/qtsvg-opensource-src-5.3.0.tar.gz
fi

if [ ! -f qtsvg-opensource-src-5.3.0/xx ]; then
cd qtsvg-opensource-src-5.3.0
exit 1
make -j 2
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# switch back to gcc

export CC=gcc-4.2
export CXX=g++-4.2

# ------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-5.2.1 ]; then
curl -L http://download.sourceforge.net/pyqt/PyQt-gpl-5.2.1.tar.gz -o PyQt-gpl-5.2.1.tar.gz
tar -xf PyQt-gpl-5.2.1.tar.gz
fi

if [ ! -f PyQt-gpl-5.2.1/Makefile ]; then
cd PyQt-gpl-5.2.1
python3 configure.py --confirm-license
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
