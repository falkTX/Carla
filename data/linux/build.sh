#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export CC=gcc-4.8
export CXX=g++-4.8
export CXFREEZE=/opt/carla/bin/cxfreeze
export MOC_QT5=moc
export RCC_QT5=moc

# Build python stuff
export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin
export PKG_CONFIG_PATH=/opt/carla/lib/pkgconfig:/opt/carla64/lib/pkgconfig
make $JOBS UI RES WIDGETS

# Build theme
make $JOBS theme

# Build everything else
export PATH=/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin
export PKG_CONFIG_PATH=/opt/carla64/lib/pkgconfig
make -C source/modules dgl
make backend $JOBS LDFLAGS="../../modules/dgl.a -lGL -lX11"
# make $JOBS

# Build Linux app
export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin
export PYTHONPATH=`pwd`/source
unset PKG_CONFIG_PATH

# cd source
rm -rf ./build ./build-lv2

cp ./source/carla                                        ./source/carla.pyw
cp ./source/carla-plugin                                 ./source/carla-plugin.pyw
cp ./source/modules/native-plugins/resources/bigmeter-ui ./source/bigmeter-ui.pyw
cp ./source/modules/native-plugins/resources/notes-ui    ./source/notes-ui.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/        ./source/carla.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/carla-plugin.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/bigmeter-ui.pyw
$CXFREEZE --include-modules=re,sip,subprocess,inspect --target-dir=./build/plugin/ ./source/notes-ui.pyw
rm ./source/*.pyw

cd build

mkdir backend
mkdir bridges
mkdir discovery
mkdir styles
cp ../source/backend/*.so                ./backend/
# cp ../source/bridges/carla-bridge-*      ./bridges/
# cp ../source/discovery/carla-discovery-* ./discovery/
cp ../source/modules/theme/styles/*      ./styles/
cp -r ../source/modules/native-plugins/resources .

find . -type f -name "*.py" -delete
mv plugin/* ./resources/
rmdir plugin

mkdir ../build-lv2
cd ../build-lv2

cp -r ../source/plugin/carla-native.lv2/ .
rm -r ./carla-native.lv2/resources
cp -r ../build/resources/ ./carla-native.lv2/

mkdir carla-native.lv2/resources/styles
# cp ../source/bridges/carla-bridge-*           carla-native.lv2/resources/
# cp ../source/discovery/carla-discovery-*      carla-native.lv2/resources/
cp ../build/./styles/* carla-native.lv2/resources/styles/

cd ..
