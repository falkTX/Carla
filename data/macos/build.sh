#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

if [ -d /dev/shm ]; then
  # Linux
  export CC="i686-apple-darwin10-gcc"
  export CXX="i686-apple-darwin10-g++"
  export PATH=/usr/i686-apple-darwin10/bin/:$PATH
else
  # MacOS
  . ./data/macos/env-vars.sh
fi

# ./configure -prefix /Users/falktx/Source/Qt-5.2.1 -release -opensource -confirm-license -no-c++11 
# -no-javascript-jit -no-qml-debug -force-pkg-config -qt-zlib -no-mtdev -no-gif -qt-libpng -qt-libjpeg -qt-freetype -no-openssl -qt-pcre 
# -no-xinput2 -no-xcb-xlib -no-glib -no-cups -no-iconv -no-icu -no-fontconfig -no-dbus -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms 
# -no-compile-examples -nomake examples -nomake tests -make tools -make libs -qt-sql-sqlite -no-framework -no-sql-odbc

# Clean build
make clean

# Build
make $JOBS HAVE_FFMPEG=false HAVE_GTK2=false HAVE_GTK3=false

# rm -rf ./data/macos/build

# Build Mac App
# cd data/macos
# python3.3 setup.py build
# cxfreeze-3.3 --target-dir=./data/macos/Carla ./source/carla.py --include-modules=re,sip

# mkdir build/backend
# mkdir build/bridges
# mkdir build/discovery
# cp ../../source/backend/*.dylib             build/backend/
# cp ../../source/discovery/carla-discovery-* build/discovery/

# cd ../..

