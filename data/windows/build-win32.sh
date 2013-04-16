#!/bin/bash

set -e

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

export CXFREEZE="wine $PYTHON_EXE C:\\\\Python33\\\\Scripts\\\\cxfreeze"
export PYUIC="wine $PYTHON_EXE C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\uic\\\\pyuic.py"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\pyrcc4.exe -py3"

export CFLAGS="-DPTW32_STATIC_LIB -I$MINGW_PATH/include"
export CXXFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL -I$MINGW_PATH/include"
export EXTRA_LIBS1="-lglib-2.0 -lgthread-2.0 -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg"
export EXTRA_LIBS2="$MINGW_PATH/lib/qt4/plugins/imageformats/libqsvg.a $MINGW_PATH/lib/qt4/plugins/iconengines/libqsvgicon.a"

# Clean build
make clean

# Build PyQt4 resources
make $JOBS UI RES WIDGETS

# Build discovery
make $JOBS -C source/discovery WIN32=true EXTRA_LIBS="$EXTRA_LIBS1"
mv source/discovery/carla-discovery-native source/discovery/carla-discovery-win32.exe

# Build backend
make $JOBS -C source/backend/standalone ../libcarla_standalone.dll CARLA_RTAUDIO_SUPPORT=true WIN32=true EXTRA_LIBS="$EXTRA_LIBS1 $EXTRA_LIBS2"

rm -rf ./data/windows/Carla
cp ./source/carla.py ./source/carla.pyw
$CXFREEZE --target-dir=".\\data\\windows\\Carla" ".\\source\\carla.pyw"
rm -f ./source/carla.pyw

cd data/windows
mkdir Carla/backend
mkdir Carla/bridges
mkdir Carla/discovery
cp ../../source/backend/*.dll   Carla/backend/
cp ../../source/discovery/*.exe Carla/discovery/

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtXml4.dll    Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/ Carla/

rm -f pkg-config

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32

# Create static build
rm -f Carla.zip
zip -r -9 Carla.zip Carla

rm -f Carla.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
chmod +x Carla.exe

# Cleanup
make -C unzipfx-carla -f Makefile.win32 clean
rm -f Carla.zip
rm -f unzipfx-*/*.exe

# Testing:
echo "export WINEPREFIX=~/.winepy3"
echo "wine $PYTHON_EXE ../../source/carla.py"
