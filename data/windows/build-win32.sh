#!/bin/bash

### fluidsynth.pc:
# -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -ldsound -lwinmm -lole32 -lws2_32
### linuxsampler.pc:
# -L${prefix}/lib/libgig -llinuxsampler -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lm -lrpcrt4

set -e

MINGW=i686-w64-mingw32
MINGW_PATH=/opt/mingw32

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export WIN32=true

export PATH=$MINGW_PATH/bin:$MINGW_PATH/$MINGW/bin:$PATH
export CC=$MINGW-gcc
export CXX=$MINGW-g++
export WINDRES=$MINGW-windres

export CFLAGS="-DBUILDING_CARLA_FOR_WINDOWS -DPTW32_STATIC_LIB"
export CXXFLAGS="$CFLAGS -DFLUIDSYNTH_NOT_A_DLL"
unset CPPFLAGS
unset LDFLAGS

export WINEARCH=win32
export WINEPREFIX=~/.winepy3_x86
export PYTHON_EXE="wine C:\\\\Python34\\\\python.exe"

export CXFREEZE="$PYTHON_EXE C:\\\\Python34\\\\Scripts\\\\cxfreeze"
export PYUIC="$PYTHON_EXE -m PyQt5.uic.pyuic"
export PYRCC="wine C:\\\\Python34\\\\Lib\\\\site-packages\\\\PyQt5\\\\pyrcc5.exe"

export DEFAULT_QT=5

make $JOBS

export PYTHONPATH=`pwd`/source

rm -rf ./data/windows/Carla
cp ./source/carla ./source/Carla.pyw
$PYTHON_EXE ./data/windows/app.py build_exe
rm -f ./source/Carla.pyw

cd data/windows/

cp ../../bin/*.dll Carla/
cp ../../bin/*.exe Carla/
rm Carla/carla-discovery-native.exe
rm Carla/carla-lv2-export.exe

# FIXME
rm Carla/carla-bridge-lv2-windows.exe
rm Carla/carla-bridge-native.exe

rm -f Carla/PyQt5.Qsci.pyd Carla/PyQt5.QtNetwork.pyd Carla/PyQt5.QtSql.pyd Carla/PyQt5.QtTest.pyd Carla/PyQt5.QtXml.pyd

cp $WINEPREFIX/drive_c/Python34/python34.dll                                Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/icu*.dll            Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libEGL.dll          Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libGLESv2.dll       Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Core.dll         Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Gui.dll          Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Widgets.dll      Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5OpenGL.dll       Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5PrintSupport.dll Carla/
cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Svg.dll          Carla/

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32 clean
make -C unzipfx-carla -f Makefile.win32

# Create zip of Carla
rm -f Carla.zip CarlaControl.zip
zip -r -9 Carla.zip Carla

# Create static build
rm -f Carla.exe CarlaControl.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
chmod +x Carla.exe

# Cleanup
rm -f Carla.zip CarlaControl.zip

# Create release zip
rm -rf Carla-2.0beta4-win32
mkdir Carla-2.0beta4-win32
mkdir Carla-2.0beta4-win32/vcredist
cp Carla.exe README.txt Carla-2.0beta4-win32
cp ~/.cache/winetricks/vcrun2010/vcredist_x86.exe Carla-2.0beta4-win32/vcredist
zip -r -9 Carla-2.0beta4-win32.zip Carla-2.0beta4-win32

cd ../..

# Testing:
echo "export WINEPREFIX=~/.winepy3_x86"
echo "$PYTHON_EXE ./source/carla"
