#!/bin/bash

VERSION="2.3.0-alpha1"

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd $(dirname $0)/../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# cleanup

if [ ! -f Makefile ] || [ ! -f source/frontend/carla ]; then
    echo "wrong dir"
    exit 1
fi

rm -rf data/windows/*.exe
rm -rf data/windows/*.lv2
rm -rf data/windows/*.vst
rm -rf data/windows/*.zip

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
