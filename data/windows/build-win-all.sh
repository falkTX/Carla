#!/bin/bash

VERSION="2.0-RC1"

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd $(dirname $0)/../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# start

make distclean
pushd data/windows
./build-win.sh 32nosse
./pack-win.sh 32nosse
mv Carla_${VERSION}-win32.zip Carla_${VERSION}-win32-nosse.zip
popd

make distclean
pushd data/windows
./build-win.sh 32
./pack-win.sh 32
popd

make distclean
pushd data/windows
./build-win.sh 64
./pack-win.sh 64
popd

# ---------------------------------------------------------------------------------------------------------------------
