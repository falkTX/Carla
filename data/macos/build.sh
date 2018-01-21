#!/bin/bash

# ------------------------------------------------------------------------------------
# stop on error

set -e

# ------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd ../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source data/macos/common.env

export MACOS="true"
export MACOS_OLD="true"

export CC=clang
export CXX=clang++

unset CPPFLAGS

##############################################################################################
# Complete 64bit build

export CFLAGS="-I${TARGETDIR}/carla64/include -m64"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${TARGETDIR}/carla64/lib -m64"

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla/lib/pkgconfig:${TARGETDIR}/carla64/lib/pkgconfig

make ${MAKE_ARGS}

##############################################################################################
# Build 32bit bridges

export CFLAGS="-I${TARGETDIR}/carla32/include -m32"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${TARGETDIR}/carla32/lib -m32"

export PATH=${TARGETDIR}/carla32/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla32/lib/pkgconfig

make posix32 ${MAKE_ARGS}

##############################################################################################
# Build Mac App

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=$(pwd)/source
unset CFLAGS
unset CXXFLAGS
unset LDLAGS
unset PKG_CONFIG_PATH

rm -rf ./build/Carla
rm -rf ./build/CarlaControl
rm -rf ./build/Carla.app
rm -rf ./build/CarlaControl.app
rm -rf ./build/exe.*
rm -rf ./build/*.lv2

cp ./source/carla                 ./source/Carla.pyw
cp ./source/carla-control         ./source/Carla-Control.pyw
cp ./bin/resources/carla-plugin   ./source/carla-plugin.pyw
cp ./bin/resources/bigmeter-ui    ./source/bigmeter-ui.pyw
cp ./bin/resources/midipattern-ui ./source/midipattern-ui.pyw
cp ./bin/resources/notes-ui       ./source/notes-ui.pyw
env SCRIPT_NAME=Carla          python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
env SCRIPT_NAME=Carla-Control  python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla-Control
env SCRIPT_NAME=carla-plugin   python3 ./data/macos/bundle.py bdist_mac --bundle-name=carla-plugin
env SCRIPT_NAME=bigmeter-ui    python3 ./data/macos/bundle.py bdist_mac --bundle-name=bigmeter-ui
env SCRIPT_NAME=midipattern-ui python3 ./data/macos/bundle.py bdist_mac --bundle-name=midipattern-ui
env SCRIPT_NAME=notes-ui       python3 ./data/macos/bundle.py bdist_mac --bundle-name=notes-ui
rm ./source/*.pyw

mkdir -p build/Carla.app/Contents/MacOS/resources
mkdir -p build/Carla.app/Contents/MacOS/styles
mkdir -p build/Carla-Control.app/Contents/MacOS/styles

cp     bin/*.dylib           build/Carla.app/Contents/MacOS/
cp     bin/carla-bridge-*    build/Carla.app/Contents/MacOS/
cp     bin/carla-discovery-* build/Carla.app/Contents/MacOS/
cp -LR bin/resources/*       build/Carla.app/Contents/MacOS/resources/
cp     bin/styles/*          build/Carla.app/Contents/MacOS/styles/

cp     bin/*utils.dylib      build/Carla-Control.app/Contents/MacOS/
cp     bin/styles/*          build/Carla-Control.app/Contents/MacOS/styles/

rm -f build/Carla.app/Contents/MacOS/carla-bridge-lv2-qt5

find build/ -type f -name "*.py" -delete
rm build/Carla.app/Contents/MacOS/resources/carla-plugin
rm build/Carla.app/Contents/MacOS/resources/carla-plugin-patchbay
rm build/Carla.app/Contents/MacOS/resources/*-ui
rm -rf build/Carla.app/Contents/MacOS/resources/__pycache__
rm -rf build/Carla.app/Contents/MacOS/resources/at1
rm -rf build/Carla.app/Contents/MacOS/resources/bls1
rm -rf build/Carla.app/Contents/MacOS/resources/rev1
rm -rf build/Carla.app/Contents/MacOS/resources/zynaddsubfx
rm -rf build/Carla-Control.app/Contents/MacOS/resources/__pycache__

cd build/Carla.app/Contents/MacOS
for f in `find . | grep -e Qt -e libq -e carlastyle.dylib`; do
install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $f
install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $f
install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $f
install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $f
install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $f
install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $f
done
cd ../../../..

cd build/Carla-Control.app/Contents/MacOS
for f in `find . | grep -e Qt -e libq -e carlastyle.dylib`; do
install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $f
install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $f
install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $f
install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $f
install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $f
install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $f
done
cd ../../../..

cp build/carla-plugin.app/Contents/MacOS/carla-plugin     build/Carla.app/Contents/MacOS/resources/
cp build/carla-plugin.app/Contents/MacOS/fcntl.so         build/Carla.app/Contents/MacOS/resources/ 2>/dev/null || true
cp build/bigmeter-ui.app/Contents/MacOS/bigmeter-ui       build/Carla.app/Contents/MacOS/resources/
cp build/midipattern-ui.app/Contents/MacOS/midipattern-ui build/Carla.app/Contents/MacOS/resources/
cp build/notes-ui.app/Contents/MacOS/notes-ui             build/Carla.app/Contents/MacOS/resources/
rm -rf build/carla-plugin.app build/bigmeter-ui.app build/midipattern-ui.app build/notes-ui.app

cd build/Carla.app/Contents/MacOS/resources/
ln -sf ../*.so* ../Qt* ../imageformats ../platforms .
ln -sf carla-plugin carla-plugin-patchbay
cd ../../../../..

mkdir build/carla.lv2
mkdir build/carla.lv2/resources
mkdir build/carla.lv2/styles

cp bin/carla.lv2/*.*        build/carla.lv2/
cp bin/carla-bridge-*       build/carla.lv2/
cp bin/carla-discovery-*    build/carla.lv2/
cp bin/libcarla_utils.dylib build/carla.lv2/
rm -f build/carla.lv2/carla-bridge-lv2-qt5
cp -LR build/Carla.app/Contents/MacOS/resources/* build/carla.lv2/resources/
cp     build/Carla.app/Contents/MacOS/styles/*    build/carla.lv2/styles/

##############################################################################################
