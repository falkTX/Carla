#!/bin/bash

set -e

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export CC="gcc-mp-4.7"
export CXX="g++-mp-4.7"
export LDFLAGS="-L/usr/local/lib/"
export PYUIC="pyuic4-3.3"
export PYRCC="pyrcc4-3.3 -py3"

# Clean build
make clean

# Build PyQt4 resources
make $JOBS UI RES WIDGETS

# Build discovery
make $JOBS -C source/discovery MACOS=true

# Build backend
make $JOBS -C source/backend/standalone ../libcarla_standalone.dylib EXTRA_LIBS="-ldl" MACOS=true
# DGL_LIBS="-framework OpenGL -framework Cocoa" HAVE_OPENGL=true

rm -rf ./data/macos/build

# Build Mac App
# cd data/macos
# python3.3 setup.py build
# cxfreeze-3.3 --target-dir=./data/macos/Carla ./source/carla.py --include-modules=re,sip

# mkdir build/backend
# mkdir build/bridges
# mkdir build/discovery
# cp ../../source/backend/*.dylib             build/backend/
# cp ../../source/discovery/carla-discovery-* build/discovery/

# cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Carla/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Carla/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Carla/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Carla/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Carla/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtXml4.dll    Carla/
# cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ Carla/
# cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/ Carla/

# cd ../..

