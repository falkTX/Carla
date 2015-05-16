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
export DEFAULT_QT=5
export PYUIC5=/opt/carla/bin/pyuic5

unset CPPFLAGS

##############################################################################################
# Complete 64bit build

export CFLAGS=-m64
export CXXFLAGS=-m64
export LDLAGS=-m64

export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla/lib/pkgconfig:/opt/carla64/lib/pkgconfig

make $JOBS

##############################################################################################
# Build 32bit bridges

export CFLAGS=-m32
export CXXFLAGS=-m32
export LDLAGS=-m32

export PATH=/opt/carla32/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=/opt/carla32/lib/pkgconfig

make posix32 $JOBS

##############################################################################################
# Build Mac App

export PATH=/opt/carla/bin:/opt/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=`pwd`/source
unset CFLAGS
unset CXXFLAGS
unset LDLAGS
unset PKG_CONFIG_PATH

rm -rf ./build/Carla
rm -rf ./build/exe.*
rm -rf ./build/*.lv2

cp ./source/carla               ./source/Carla.pyw
cp ./bin/resources/carla-plugin ./source/carla-plugin.pyw
cp ./bin/resources/bigmeter-ui  ./source/bigmeter-ui.pyw
cp ./bin/resources/notes-ui     ./source/notes-ui.pyw
env SCRIPT_NAME=Carla        python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
env SCRIPT_NAME=carla-plugin python3 ./data/macos/bundle.py bdist_mac --bundle-name=carla-plugin
env SCRIPT_NAME=bigmeter-ui  python3 ./data/macos/bundle.py bdist_mac --bundle-name=bigmeter-ui
env SCRIPT_NAME=notes-ui     python3 ./data/macos/bundle.py bdist_mac --bundle-name=notes-ui
rm ./source/*.pyw

mkdir -p build/Carla.app/Contents/MacOS/resources
mkdir -p build/Carla.app/Contents/MacOS/styles
cp     bin/*.dylib           build/Carla.app/Contents/MacOS/
cp     bin/carla-bridge-*    build/Carla.app/Contents/MacOS/
cp     bin/carla-discovery-* build/Carla.app/Contents/MacOS/
cp -LR bin/resources/*       build/Carla.app/Contents/MacOS/resources/
cp     bin/styles/*          build/Carla.app/Contents/MacOS/styles/

rm -f build/Carla.app/Contents/MacOS/carla-bridge-lv2-modgui
rm -f build/Carla.app/Contents/MacOS/carla-bridge-lv2-qt5

find build/ -type f -name "*.py" -delete
rm build/Carla.app/Contents/MacOS/resources/carla-plugin
rm build/Carla.app/Contents/MacOS/resources/*-ui
rm -rf build/Carla.app/Contents/MacOS/resources/__pycache__

cd build/Carla.app/Contents/MacOS/resources/
ln -sf ../*.so* ../Qt* ../imageformats ../platforms .
cd ../../../../..

cd build/Carla.app/Contents/MacOS/styles
install_name_tool -change "/opt/carla/lib/QtCore.framework/Versions/5/QtCore"       @executable_path/QtCore    carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtGui.framework/Versions/5/QtGui"         @executable_path/QtGui     carlastyle.dylib
install_name_tool -change "/opt/carla/lib/QtWidgets.framework/Versions/5/QtWidgets" @executable_path/QtWidgets carlastyle.dylib
cd ../../../../..

cp build/carla-plugin.app/Contents/MacOS/carla-plugin build/Carla.app/Contents/MacOS/resources/
cp build/carla-plugin.app/Contents/MacOS/fcntl.so     build/Carla.app/Contents/MacOS/resources/ 2>/dev/null || true
cp build/bigmeter-ui.app/Contents/MacOS/bigmeter-ui   build/Carla.app/Contents/MacOS/resources/
cp build/notes-ui.app/Contents/MacOS/notes-ui         build/Carla.app/Contents/MacOS/resources/
rm -rf build/carla-plugin.app build/bigmeter-ui.app build/notes-ui.app

mkdir build/carla.lv2
mkdir build/carla.lv2/resources
mkdir build/carla.lv2/styles

cp bin/carla.lv2/*.*        build/carla.lv2/
cp bin/carla-bridge-*       build/carla.lv2/
cp bin/carla-discovery-*    build/carla.lv2/
cp bin/libcarla_utils.dylib build/carla.lv2/
rm -f build/carla.lv2/carla-bridge-lv2-modgui
rm -f build/carla.lv2/carla-bridge-lv2-qt5
cp -LR build/Carla.app/Contents/MacOS/resources/* build/carla.lv2/resources/
cp     build/Carla.app/Contents/MacOS/styles/*    build/carla.lv2/styles/

##############################################################################################
