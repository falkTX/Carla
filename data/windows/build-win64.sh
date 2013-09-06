#!/bin/bash

set -e

MINGW=x86_64-w64-mingw32
MINGW_PATH=/opt/mingw64

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export WIN32=true

export PATH=$MINGW_PATH/bin:$MINGW_PATH/$MINGW/bin:$PATH
export AR=$MINGW-ar
export CC=$MINGW-gcc
export CXX=$MINGW-g++
export MOC=$MINGW-moc
export RCC=$MINGW-rcc
export UIC=$MINGW-uic
export STRIP=$MINGW-strip
export WINDRES=$MINGW-windres

export CFLAGS="-DPTW32_STATIC_LIB -I$MINGW_PATH/include"
export CXXFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL -I$MINGW_PATH/include"
export EXTRA_LIBS1="-lglib-2.0 -lgthread-2.0 -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -ldsound -lrpcrt4"
export EXTRA_LIBS2="$MINGW_PATH/lib/qt4/plugins/imageformats/libqsvg.a"

export WINEPREFIX=~/.winepy3_x64
export PYTHON_EXE="C:\\\\Python33\\\\python.exe"

export CXFREEZE="wine $PYTHON_EXE C:\\\\Python33\\\\Scripts\\\\cxfreeze"
export PYUIC="wine $PYTHON_EXE C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\uic\\\\pyuic.py"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\pyrcc4.exe -py3"

# Clean build
make clean

# Build PyQt4 resources
make $JOBS UI RES WIDGETS

# Build discovery
make $JOBS discovery EXTRA_LIBS="$EXTRA_LIBS1"
mv source/discovery/carla-discovery-native.exe source/discovery/carla-discovery-win32.exe

# Build backend
make $JOBS backend EXTRA_LIBS="$EXTRA_LIBS1 $EXTRA_LIBS2"

# Build Plugin bridges
# make $JOBS bridges EXTRA_LIBS="$EXTRA_LIBS1 $EXTRA_LIBS2"

# Build UI bridges
make $JOBS -C source/bridges ui_lv2-win32 ui_vst-hwnd EXTRA_LIBS="$EXTRA_LIBS2"

rm -rf ./data/windows/Carla
cp ./source/carla.py ./source/carla.pyw
$CXFREEZE --target-dir=".\\data\\windows\\Carla" ".\\source\\carla.pyw"
rm -f ./source/carla.pyw

rm -rf ./data/windows/CarlaControl
cp ./source/carla_control.py ./source/carla_control.pyw
$CXFREEZE --target-dir=".\\data\\windows\\CarlaControl" ".\\source\\carla_control.pyw"
rm -f ./source/carla_control.pyw

cd data/windows
mkdir Carla/backend
mkdir Carla/bridges
mkdir Carla/discovery
cp ../../source/backend/*.dll   Carla/backend/
cp ../../source/discovery/*.exe Carla/discovery/
mv CarlaControl/carla_control.exe CarlaControl/CarlaControl.exe

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtXml4.dll    Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/ Carla/

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll CarlaControl/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   CarlaControl/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    CarlaControl/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ CarlaControl/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/ CarlaControl/

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32
make -C unzipfx-carla-control -f Makefile.win32

# Create static build
rm -f Carla.zip CarlaControl.zip
zip -r -9 Carla.zip Carla
zip -r -9 CarlaControl.zip CarlaControl

rm -f Carla.exe CarlaControl.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
cat unzipfx-carla-control/unzipfx2cat.exe CarlaControl.zip > CarlaControl.exe
chmod +x Carla.exe
chmod +x CarlaControl.exe

# Cleanup
make -C unzipfx-carla -f Makefile.win32 clean
make -C unzipfx-carla-control -f Makefile.win32 clean
rm -f Carla.zip CarlaControl.zip
rm -f unzipfx-*/*.exe

# Testing:
echo "export WINEPREFIX=~/.winepy3_x64"
echo "wine $PYTHON_EXE ../../source/carla.py"
