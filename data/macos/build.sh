#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export MACOS="true"
export CC=clang
export CXX=clang++
export CXFREEZE=/opt/kxstudio/bin/cxfreeze

# Build python stuff
export PATH=/opt/kxstudio/bin:/opt/kxstudio64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/kxstudio64/lib/pkgconfig
# make $JOBS UI RES WIDGETS

# Build theme
rm -rf incs
mkdir incs
ln -s /opt/kxstudio/lib/QtCore.framework/Headers    incs/QtCore
ln -s /opt/kxstudio/lib/QtGui.framework/Headers     incs/QtGui
ln -s /opt/kxstudio/lib/QtWidgets.framework/Headers incs/QtWidgets
export CXXFLAGS="-I`pwd`/incs"
export LDFLAGS="-F/opt/kxstudio/lib/ -framework QtCore -framework QtGui -framework QtWidgets"
make $JOBS theme
rm -rf incs
exit 0

# Build backend
export PATH=/opt/kxstudio64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/kxstudio64/lib/pkgconfig
make $JOBS backend discovery plugin

cd ../..
exit 0

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
