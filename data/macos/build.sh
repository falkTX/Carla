#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export MACOS="true"
export CC=clang
export CXX=clang++
export CXFREEZE="/opt/carla/bin/cxfreeze --include-modules=re,sip,subprocess,inspect"

##############################################################################################
# Complete 64bit build

export CFLAGS=-m64
export CXXFLAGS=-m64
export LDLAGS=-m64

export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla/lib/pkgconfig:/opt/carla64/lib/pkgconfig

# make $JOBS

##############################################################################################
# Build 32bit bridges

export CFLAGS=-m32
export CXXFLAGS=-m32
export LDLAGS=-m32

export PATH=/opt/carla32/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla32/lib/pkgconfig

# make posix32 $JOBS

##############################################################################################
# Build Mac App

export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=`pwd`/source
unset CFLAGS
unset CXXFLAGS
unset LDLAGS
unset PKG_CONFIG_PATH

# cd source
rm -rf ./build ./build-lv2

cp ./source/carla ./source/Carla.pyw
python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
rm ./source/Carla.pyw

#cp ./source/modules/native-plugins/resources/carla-plugin ./source/carla-plugin.pyw
#cp ./source/modules/native-plugins/resources/bigmeter-ui  ./source/bigmeter-ui.pyw
#cp ./source/modules/native-plugins/resources/notes-ui     ./source/notes-ui.pyw
#$CXFREEZE --target-dir=./build/plugin/ ./source/carla-plugin.pyw
#$CXFREEZE --target-dir=./build/plugin/ ./source/bigmeter-ui.pyw
#$CXFREEZE --target-dir=./build/plugin/ ./source/notes-ui.pyw
#rm ./source/*.pyw

cd build

mkdir -p Carla.app/Contents/MacOS
mkdir -p Carla.app/Contents/MacOS/styles
cp ../bin/*.dylib           Carla.app/Contents/MacOS/
cp ../bin/carla-bridge-*    Carla.app/Contents/MacOS/
cp ../bin/carla-discovery-* Carla.app/Contents/MacOS/
cp ../bin/styles/*          Carla.app/Contents/MacOS/styles/
#cp -r ../bin/resources Carla.app/Contents/MacOS/

#find . -type f -name "*.py" -delete
#mv plugin/* Carla.app/Contents/MacOS/resources/
#rmdir plugin

cd Carla.app/Contents/MacOS/styles
install_name_tool -change "/opt/carla/lib/QtCore.framework/Versions/5/QtCore"       @executable_path/QtCore    carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtGui.framework/Versions/5/QtGui"         @executable_path/QtGui     carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtWidgets.framework/Versions/5/QtWidgets" @executable_path/QtWidgets carlastyle.dylib
cd ../../../..

exit

mkdir ../build-lv2
cd ../build-lv2

cp -r ../bin/carla-native.lv2/ carla-native.lv2
rm -r ./carla-native.lv2/resources
cp -r ../build/Carla.app/Contents/MacOS/resources/ carla-native.lv2/resources

rm -rf carla-native.lv2/styles
mkdir carla-native.lv2/styles
cp ../bin/carla-bridge-*                      carla-native.lv2/
cp ../bin/carla-discovery-*                   carla-native.lv2/
cp ../build/Carla.app/Contents/MacOS/styles/* carla-native.lv2/styles/

cd ..

##############################################################################################
