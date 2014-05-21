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
  export CXFREEZE=cxfreeze
fi

# Clean build
# make clean

# Build
# make $JOBS

# Build Mac App
rm -rf ./data/macos/Carla

cp ./source/carla ./source/carla.pyw
# python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./data/macos/Carla ./source/carla.pyw
rm ./source/carla.pyw

cd data/macos

mkdir Carla/backend
mkdir Carla/bridges
mkdir Carla/discovery
cp ../../source/backend/*.dylib             Carla/backend/
cp ../../source/bridges/carla-bridge-*      Carla/bridges/
cp ../../source/discovery/carla-discovery-* Carla/discovery/
cp -r ../../source/modules/theme/styles     Carla/

cd ../..
