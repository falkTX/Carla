#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export MACOS="true"
export CC=clang
export CXX=clang++
export CXFREEZE=/opt/carla/bin/cxfreeze

# Build python stuff
export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla/lib/pkgconfig:/opt/carla64/lib/pkgconfig
make $JOBS UI RES WIDGETS

# Build theme
make $JOBS theme

# Build everything else
export PATH=/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla64/lib/pkgconfig
make backend $JOBS
make $JOBS

# Build Mac App
export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=`pwd`/source
unset PKG_CONFIG_PATH

# cd source
rm -rf ./build ./build-lv2

cp ./source/carla ./source/Carla.pyw
python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
rm ./source/*.pyw

cp ./source/carla-plugin                                 ./source/carla-plugin.pyw
cp ./source/modules/native-plugins/resources/bigmeter-ui ./source/bigmeter-ui.pyw
cp ./source/modules/native-plugins/resources/notes-ui    ./source/notes-ui.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/carla-plugin.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/bigmeter-ui.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/notes-ui.pyw
rm ./source/*.pyw

cd build

mkdir Carla.app/Contents/MacOS/backend
mkdir Carla.app/Contents/MacOS/bridges
mkdir Carla.app/Contents/MacOS/discovery
mkdir Carla.app/Contents/MacOS/styles
cp ../source/backend/*.dylib             Carla.app/Contents/MacOS/backend/
cp ../source/bridges/carla-bridge-*      Carla.app/Contents/MacOS/bridges/
cp ../source/discovery/carla-discovery-* Carla.app/Contents/MacOS/discovery/
cp ../source/modules/theme/styles/*      Carla.app/Contents/MacOS/styles/
cp -r ../source/modules/native-plugins/resources Carla.app/Contents/MacOS/

find . -type f -name "*.py" -delete
mv plugin/* Carla.app/Contents/MacOS/resources/
rmdir plugin

cd Carla.app/Contents/MacOS/styles
install_name_tool -change "/opt/carla/lib/QtCore.framework/Versions/5/QtCore"       @loader_path/../QtCore    carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtGui.framework/Versions/5/QtGui"         @loader_path/../QtGui     carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtWidgets.framework/Versions/5/QtWidgets" @loader_path/../QtWidgets carlastyle.dylib
cd ../../../..

mkdir ../build-lv2
cd ../build-lv2

cp -r ../source/plugin/carla-native.lv2/ carla-native.lv2
rm -r ./carla-native.lv2/resources
cp -r ../build/Carla.app/Contents/MacOS/resources/ carla-native.lv2/resources

mkdir carla-native.lv2/resources/styles
cp ../source/bridges/carla-bridge-*           carla-native.lv2/resources/
cp ../source/discovery/carla-discovery-*      carla-native.lv2/resources/
cp ../build/Carla.app/Contents/MacOS/styles/* carla-native.lv2/resources/styles/

cd ..
