#!/bin/bash

set -e

if [ -f Makefile ]; then
  cd data/macos
fi

export MACOS=true
export CC=gcc-4.2
export CXX=g++-4.2
export PATH=/opt/kxstudio/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig

# -fPIC -DPIC
export CFLAGS="-O2 -mtune=generic -msse -msse2"
export CXXFLAGS=$CFLAGS

# ------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-0.28 ]; then
curl -O http://pkgconfig.freedesktop.org/releases/pkg-config-0.28.tar.gz
tar -xf pkg-config-0.28.tar.gz
fi

if [ ! -f pkg-config-0.28/Makefile ]; then
cd pkg-config-0.28
./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=/opt/kxstudio
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-1.2.8 ]; then
curl -O http://zlib.net/zlib-1.2.8.tar.gz
tar -xf zlib-1.2.8.tar.gz
fi

if [ ! -f zlib-1.2.8/zlib.pc ]; then
cd zlib-1.2.8
./configure --static --prefix=/opt/kxstudio
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-0.28 ]; then
curl -L http://download.sourceforge.net/liblo/liblo-0.28.tar.gz -o liblo-0.28.tar.gz
tar -xf liblo-0.28.tar.gz
fi

if [ ! -f liblo-0.28/liblo.pc ]; then
cd liblo-0.28
./configure --enable-static --disable-shared --prefix=/opt/kxstudio
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# mxml

if [ ! -d mxml-2.8 ]; then
curl -O http://www.msweet.org/files/project3/mxml-2.8.tar.gz
tar -xf mxml-2.8.tar.gz
fi

if [ ! -f mxml-2.8/mxml.pc ]; then
cd mxml-2.8
./configure --enable-static --disable-shared --prefix=/opt/kxstudio
make
sudo cp *.a    /opt/kxstudio/lib/
sudo cp *.pc   /opt/kxstudio/lib/pkgconfig/
sudo cp mxml.h /opt/kxstudio/include/
cd ..
fi

# ------------------------------------------------------------------------------------
# fltk

if [ ! -d fltk-1.3.2 ]; then
curl -O http://fltk.org/pub/fltk/1.3.2/fltk-1.3.2-source.tar.gz
tar -xf fltk-1.3.2-source.tar.gz
fi

if [ ! -f fltk-1.3.2/fltk-config ]; then
cd fltk-1.3.2
./configure --enable-static --enable-localjpeg --enable-localpng --enable-localzlib \
            --disable-shared --disable-x11 --disable-cairoext --disable-cairo  --disable-gl --prefix=/opt/kxstudio
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# qt4

if [ ! -d qt-everywhere-opensource-src-4.8.6 ]; then
curl -O http://download.qt-project.org/official_releases/qt/4.8/4.8.6/qt-everywhere-opensource-src-4.8.6.tar.gz
tar -xf qt-everywhere-opensource-src-4.8.6.tar.gz
fi

if [ ! -f qt-everywhere-opensource-src-4.8.6/bin/moc ]; then
cd qt-everywhere-opensource-src-4.8.6
./configure -prefix /opt/kxstudio -release -static -opensource -confirm-license -force-pkg-config -no-framework \
            -qt-freetype -qt-libjpeg -qt-libpng -qt-sql-sqlite -qt-zlib -harfbuzz -svg \
            -no-cups -no-dbus -no-fontconfig -no-iconv -no-icu -no-gif -no-glib -no-libmng -no-libtiff -no-nis -no-openssl -no-pch -no-sql-odbc \
            -no-audio-backend -no-multimedia -no-phonon -no-phonon-backend -no-qt3support -no-webkit -no-xmlpatterns \
            -no-declarative -no-declarative-debug -no-javascript-jit -no-script -no-scripttools \
            -nomake examples -nomake tests -make libs -make tools
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# qt5

if [ ! -d qtbase5-mac10.5 ]; then
git clone git://github.com/falkTX/qtbase5-mac10.5 --depth 1
fi

if [ ! -f qtbase5-mac10.5/bin/moc ]; then
cd qtbase5-mac10.5
export QMAKESPEC=macx-g++42
./configure -release -static -opensource -confirm-license -force-pkg-config \
            -prefix /opt/kxstudio -plugindir /opt/kxstudio/lib/qt5/plugins -headerdir /opt/kxstudio/include/qt5 \
            -qt-freetype -qt-libjpeg -qt-libpng -qt-pcre -qt-sql-sqlite -qt-zlib -opengl no -no-c++11 -no-framework -qpa cocoa \
            -no-directfb -no-eglfs -no-kms -no-linuxfb -no-mtdev -no-xcb -no-xcb-xlib \
            -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-neon -no-mips_dsp -no-mips_dspr2 \
            -no-cups -no-dbus -no-fontconfig -no-harfbuzz -no-iconv -no-icu -no-gif -no-glib -no-nis -no-openssl -no-pch -no-sql-ibase -no-sql-odbc \
            -no-audio-backend -no-javascript-jit -no-qml-debug -no-rpath -no-separate-debug-info \
            -no-compile-examples -nomake examples -nomake tests -make libs -make tools
make -j 2
sudo make install
cd ..
fi

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
# pyqt

if [ ! -d PyQt-mac-gpl-4.10.4 ]; then
curl -L http://download.sourceforge.net/pyqt/PyQt-mac-gpl-4.10.4.tar.gz -o PyQt-mac-gpl-4.10.4.tar.gz
tar -xf PyQt-mac-gpl-4.10.4.tar.gz
fi

if [ ! -f PyQt-mac-gpl-4.10.4/Makefile ]; then
cd PyQt-mac-gpl-4.10.4
python3 configure.py -g --confirm-license --no-deprecated
make
sudo make install
cd ..
fi

# ------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-5.2.1 ]; then
curl -L http://download.sourceforge.net/pyqt/PyQt-gpl-5.2.1.tar.gz -o PyQt-gpl-5.2.1.tar.gz
tar -xf PyQt-gpl-5.2.1.tar.gz
fi

if [ ! -f PyQt-gpl-5.2.1/Makefile ]; then
cd PyQt-gpl-5.2.1
python3 configure.py -g --confirm-license
cp _qt/*.h qpy/QtCore/
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
# fftw3

if [ ! -d fftw-3.3.4 ]; then
curl -O http://www.fftw.org/fftw-3.3.4.tar.gz
tar -xf fftw-3.3.4.tar.gz
fi

if [ ! -f fftw-3.3.4/build-done ]; then
# -fPIC -DPIC
export CFLAGS="-O2 -mtune=generic -msse -msse2 -ffast-math -mfpmath=sse"
export CXXFLAGS=$CFLAGS
cd fftw-3.3.4
./configure --enable-static --enable-sse2 --disable-shared --disable-debug --prefix=/opt/kxstudio
make
sudo make install
make clean
./configure --enable-static --enable-sse --enable-sse2 --enable-single --disable-shared --disable-debug --prefix=/opt/kxstudio
make
sudo make install
make clean
touch build-done
cd ..
fi

# ------------------------------------------------------------------------------------
