#!/bin/bash

set -e

MINGW=x86_64-w64-mingw32
MINGW_PATH=/opt/mingw64

JOBS="-j 2"

if [ ! -f Makefile ]; then
  cd ../..
fi

export WIN32=true
export WIN64=true

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
export EXTRA_LIBS="-lglib-2.0 -lgthread-2.0 -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -ldsound -lole32 -lrpcrt4 -lws2_32 -lwinmm"

export WINEARCH=win64
export WINEPREFIX=~/.winepy3_x64
export PYTHON_EXE="C:\\\\Python33\\\\python.exe"

export CXFREEZE="wine $PYTHON_EXE C:\\\\Python33\\\\Scripts\\\\cxfreeze"
export PYUIC="wine $PYTHON_EXE -m PyQt5.uic.pyuic"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt5\\\\pyrcc5.exe"

# Clean build
make clean

# Build PyQt5 resources
make source/carla_config.py
sed "s/config_UseQt5 = False/config_UseQt5 = True/" -i source/carla_config.py
make $JOBS UI RES WIDGETS

# Build discovery
make $JOBS discovery EXTRA_LIBS="$EXTRA_LIBS"
mv source/discovery/carla-discovery-native.exe source/discovery/carla-discovery-win64.exe

# Build backend
make $JOBS backend EXTRA_LIBS="$EXTRA_LIBS"

# Build Plugin bridges
# make $JOBS bridges EXTRA_LIBS="$EXTRA_LIBS"

# Build UI bridges
# make $JOBS -C source/bridges ui_lv2-win32 ui_vst-hwnd

rm -rf ./data/windows/Carla
cp ./source/carla ./source/carla.pyw
$CXFREEZE --target-dir=".\\data\\windows\\Carla" ".\\source\\carla.pyw"
rm -f ./source/carla.pyw

# rm -rf ./data/windows/CarlaControl
# cp ./source/carla_control.py ./source/carla_control.pyw
# $CXFREEZE --target-dir=".\\data\\windows\\CarlaControl" ".\\source\\carla_control.pyw"
# rm -f ./source/carla_control.pyw

cd data/windows
mkdir Carla/backend
mkdir Carla/bridges
mkdir Carla/discovery
cp ../../source/backend/*.dll   Carla/backend/
cp ../../source/discovery/*.exe Carla/discovery/
# mv CarlaControl/carla_control.exe CarlaControl/CarlaControl.exe

rm -f Carla/imageformats/*.so

cp $WINEPREFIX/drive_c/drive_c/Python33/python33.dll                        Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/libEGL.dll          Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/libGLESv2.dll       Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icuin49.dll         Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icuuc49.dll         Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icudt49.dll         Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Core.dll         Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Gui.dll          Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Widgets.dll      Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5OpenGL.dll       Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5PrintSupport.dll Carla/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Svg.dll          Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/imageformats/ Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/iconengines/  Carla/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/platforms/    Carla/

# cp $WINEPREFIX/drive_c/drive_c/Python33/python33.dll                        CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/libEGL.dll          CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/libGLESv2.dll       CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icuin49.dll         CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icuuc49.dll         CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/icudt49.dll         CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Core.dll         CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Gui.dll          CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Widgets.dll      CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5OpenGL.dll       CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5PrintSupport.dll CarlaControl/
# cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/Qt5Svg.dll          CarlaControl/
# cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/imageformats/ CarlaControl/
# cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/iconengines/  CarlaControl/
# cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt5/plugins/platforms/    CarlaControl/

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32
# make -C unzipfx-carla-control -f Makefile.win32

# Create static build
rm -f Carla.zip CarlaControl.zip
zip -r -9 Carla.zip Carla
# zip -r -9 CarlaControl.zip CarlaControl

rm -f Carla.exe CarlaControl.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
# cat unzipfx-carla-control/unzipfx2cat.exe CarlaControl.zip > CarlaControl.exe
chmod +x Carla.exe
# chmod +x CarlaControl.exe

# Cleanup
make -C unzipfx-carla -f Makefile.win32 clean
make -C unzipfx-carla-control -f Makefile.win32 clean
rm -f Carla.zip CarlaControl.zip
rm -f unzipfx-*/*.exe

cd ../..

# Testing:
echo "export WINEPREFIX=~/.winepy3_x64"
echo "wine $PYTHON_EXE ./source/carla -platformpluginpath \"C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt5\\\\plugins\\\\platforms\""
