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
  export CXFREEZE=cxfreeze-3.3
fi

# ./configure -prefix /Users/falktx/Source/Qt-5.2.1 -release -opensource -confirm-license -no-c++11 
# -no-javascript-jit -no-qml-debug -force-pkg-config -qt-zlib -no-mtdev -no-gif -qt-libpng -qt-libjpeg -qt-freetype -no-openssl -qt-pcre 
# -no-xinput2 -no-xcb-xlib -no-glib -no-cups -no-iconv -no-icu -no-fontconfig -no-dbus -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms 
# -no-compile-examples -nomake examples -nomake tests -make tools -make libs -qt-sql-sqlite -no-framework -no-sql-odbc

# Clean build
# make clean

# Build
# make $JOBS

# Build Mac App
rm -rf ./data/macos/Carla

cp ./source/carla ./source/carla.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./data/macos/Carla ./source/carla.pyw
rm ./source/carla.pyw

cd data/macos

mkdir Carla/backend
mkdir Carla/bridges
mkdir Carla/discovery
cp ../../source/backend/*.dylib             Carla/backend/
cp ../../source/discovery/carla-discovery-* Carla/discovery/
cp -r ../../source/modules/theme/styles     Carla/

cd ../..
