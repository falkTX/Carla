#!/bin/bash

MINGW=i686-w64-mingw32
MINGW_PATH=/opt/mingw32

JOBS="-j 4"

if [ ! -f Makefile ]; then
  cd ../..
fi

ln -s -f $MINGW_PATH/bin/$MINGW-pkg-config ./data/windows/pkg-config

export PATH=`pwd`/data/windows:$MINGW_PATH/bin:$MINGW_PATH/$MINGW/bin:$PATH
export AR=$MINGW-ar
export CC=$MINGW-gcc
export CXX=$MINGW-g++
export MOC=$MINGW-moc
export RCC=$MINGW-rcc
export UIC=$MINGW-uic
export STRIP=$MINGW-strip
export WINDRES=$MINGW-windres

export PKG_CONFIG_PATH=$MINGW_PATH/lib/pkgconfig

export WINEPREFIX=~/.winepy3

export PYTHON_EXE="C:\\\\Python33\\\\python.exe"

export PYUIC="wine $PYTHON_EXE C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\uic\\\\pyuic.py"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\pyrcc4.exe -py3"

export CFLAGS="-DPTW32_STATIC_LIB -I$MINGW_PATH/include"
export CXXFLAGS="-DPTW32_STATIC_LIB -D__WINDOWS_ASIO__ -I$MINGW_PATH/include"

# TODO: DirectSound DLLs for -D__WINDOWS_DS__

# Clean build
make clean

# Build PyQt4 resources
make $JOBS UI RES WIDGETS

# Build discovery
make $JOBS -C source/discovery win32

# Build backend
make $JOBS -C source/backend/standalone ../libcarla_standalone.dll CARLA_RTAUDIO_SUPPORT=true CARLA_SAMPLERS_SUPPORT=false DGL_LIBS="" OBJSN=""

if [ ! -f Makefile ]; then
  cd data/windows
fi

em -f pkg-config

# Testing:
echo "export WINEPREFIX=~/.winepy3"
echo "wine $PYTHON_EXE ../../source/carla.py"
