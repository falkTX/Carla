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
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:/opt/kxstudio64/lib/pkgconfig
make $JOBS UI RES WIDGETS

# Build theme
make $JOBS theme

# Build everything else
export PATH=/opt/kxstudio64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/kxstudio64/lib/pkgconfig
make backend $JOBS
make $JOBS

# Build Mac App
export PATH=/opt/kxstudio/bin:/opt/kxstudio64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=`pwd`/source
unset PKG_CONFIG_PATH

# cd source
rm -rf ./build

cp ./source/carla ./source/Carla.pyw
python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
rm ./source/Carla.pyw

cd build

mkdir Carla.app/Contents/MacOS/backend
mkdir Carla.app/Contents/MacOS/bridges
mkdir Carla.app/Contents/MacOS/discovery
cp ../source/backend/*.dylib             Carla.app/Contents/MacOS/backend/
cp ../source/bridges/carla-bridge-*      Carla.app/Contents/MacOS/bridges/
cp ../source/discovery/carla-discovery-* Carla.app/Contents/MacOS/discovery/
cp -r ../source/modules/theme/styles     Carla.app/Contents/MacOS/

cd Carla.app/Contents/MacOS/styles
install_name_tool -change "/opt/kxstudio/lib/QtCore.framework/Versions/5/QtCore"       @loader_path/../QtCore    carlastyle.dylib
install_name_tool -change "/opt/kxstudio/lib/QtGui.framework/Versions/5/QtGui"         @loader_path/../QtGui     carlastyle.dylib
install_name_tool -change "/opt/kxstudio/lib/QtWidgets.framework/Versions/5/QtWidgets" @loader_path/../QtWidgets carlastyle.dylib
cd ../../../..

cd ..
