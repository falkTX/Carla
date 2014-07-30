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
# qt5-base download (5.2.1 for now)

if [ ! -d qtbase-opensource-src-5.3.1 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.1/submodules/qtbase-opensource-src-5.3.1.tar.gz -o qtbase-opensource-src-5.3.1.tar.gz
tar -xf qtbase-opensource-src-5.3.1.tar.gz
fi

# ------------------------------------------------------------------------------------
# qt5-base (regular, 64bit, shared, framework)

if [ ! -f qtbase-opensource-src-5.3.1/build-done ]; then
cd qtbase-opensource-src-5.3.1
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

if [ ! -d qtmacextras-opensource-src-5.3.1 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.1/submodules/qtmacextras-opensource-src-5.3.1.tar.gz -o qtmacextras-opensource-src-5.3.1.tar.gz
tar -xf qtmacextras-opensource-src-5.3.1.tar.gz
fi

if [ ! -f qtmacextras-opensource-src-5.3.1/build-done ]; then
cd qtmacextras-opensource-src-5.3.1
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-5.3.1 ]; then
curl -L http://download.qt-project.org/official_releases/qt/5.3/5.3.1/submodules/qtsvg-opensource-src-5.3.1.tar.gz -o qtsvg-opensource-src-5.3.1.tar.gz
tar -xf qtsvg-opensource-src-5.3.1.tar.gz
fi

if [ ! -f qtsvg-opensource-src-5.3.1/build-done ]; then
cd qtsvg-opensource-src-5.3.1
qmake
make -j 2
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# python

if [ ! -d Python-3.4.1 ]; then
curl -O https://www.python.org/ftp/python/3.4.1/Python-3.4.1.tgz
tar -xf Python-3.4.1.tgz
fi

if [ ! -f Python-3.4.1/build-done ]; then
cd Python-3.4.1
./configure --prefix=$PREFIX
make
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# sip

if [ ! -d sip-4.16.2 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/sip/sip-4.16.2/sip-4.16.2.tar.gz -o sip-4.16.2.tar.gz
tar -xf sip-4.16.2.tar.gz
fi

if [ ! -f sip-4.16.2/build-done ]; then
cd sip-4.16.2
python3 configure.py
make
sudo make install
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-5.3.1 ]; then
curl -L http://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-5.3.1/PyQt-gpl-5.3.1.tar.gz -o PyQt-gpl-5.3.1.tar.gz
tar -xf PyQt-gpl-5.3.1.tar.gz
fi

if [ ! -f PyQt-gpl-5.3.1/build-done ]; then
cd PyQt-gpl-5.3.1
python3 configure.py --confirm-license
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
