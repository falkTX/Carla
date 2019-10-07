#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check input

ARCH="${1}"
ARCH_PREFIX="${1}"

if [ x"${ARCH}" != x"32" ] && [ x"${ARCH}" != x"32nosse" ] && [ x"${ARCH}" != x"64" ]; then
  echo "usage: $0 32|nonosse|64"
  exit 1
fi

if [ x"${ARCH}" = x"32nosse" ]; then
  ARCH="32"
  MAKE_ARGS="NOOPT=true"
fi

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd $(dirname $0)/../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source data/windows/common.env

PKG_FOLDER="Carla_2.0-alpha2-win${ARCH}"

export WIN32=true

if [ x"${ARCH}" != x"32" ]; then
  export WIN64=true
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

# ---------------------------------------------------------------------------------------------------------------------

export_vars() {

local _ARCH="${1}"
local _ARCH_PREFIX="${2}"
local _MINGW_PREFIX="${3}-w64-mingw32"

export PREFIX=${TARGETDIR}/carla-w${_ARCH_PREFIX}
export MSYS2_PREFIX="${TARGETDIR}/msys2-${CPUARCH}/mingw${ARCH}"

export PATH=${PREFIX}/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig:${MSYS2_PREFIX}/lib/pkgconfig

export AR=${_MINGW_PREFIX}-ar
export CC=${_MINGW_PREFIX}-gcc
export CXX=${_MINGW_PREFIX}-g++
export STRIP=${_MINGW_PREFIX}-strip
export WINDRES=${_MINGW_PREFIX}-windres

export CFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL"
export CFLAGS="${CFLAGS} -I${PREFIX}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${PREFIX}/lib"
}

# ---------------------------------------------------------------------------------------------------------------------

export_vars "${ARCH}" "${ARCH_PREFIX}" "${CPUARCH}"

export WINEARCH=win${ARCH}
# export WINEDEBUG=-all
export WINEPREFIX=~/.winepy3_x${ARCH}
export PYTHON_EXE="wine ${MSYS2_PREFIX}/bin/python.exe"
export CXFREEZE="$PYTHON_EXE ${MSYS2_PREFIX}/bin/cxfreeze"
export PYTHONPATH="$(pwd)/source/frontend" # ;${MSYS2_PREFIX}/lib/python3.7

rm -rf ./data/windows/Carla ./data/windows/Carla.lv2 ./data/windows/Carla.vst
mkdir -p ./data/windows/Carla/Debug
cp ./source/frontend/carla ./source/frontend/Carla.pyw
$PYTHON_EXE ./data/windows/app-console.py build_exe
mv ./data/windows/Carla/carla.exe ./data/windows/Carla/Debug/Carla.exe
$PYTHON_EXE ./data/windows/app-gui.py build_exe
rm -f ./source/frontend/Carla.pyw

cd data/windows/

rm -rf dist
$CXFREEZE ../../bin/resources/bigmeter-ui
mv dist/lib/library.zip dist/lib/library-bigmeter.zip
$CXFREEZE ../../bin/resources/midipattern-ui
mv dist/lib/library.zip dist/lib/library-midipattern.zip
$CXFREEZE ../../bin/resources/notes-ui
mv dist/lib/library.zip dist/lib/library-notes.zip
$CXFREEZE ../../bin/resources/carla-plugin
mv dist/lib/library.zip dist/lib/library-carla1.zip
$CXFREEZE ../../bin/resources/carla-plugin-patchbay
mv dist/lib/library.zip dist/lib/library-carla2.zip

cp ../../bin/*.dll Carla/
cp ../../bin/*.exe Carla/
rm Carla/carla-discovery-native.exe
rm Carla/carla-lv2-export.exe
rm Carla/carla-native-plugin.exe

# rm -f Carla/PyQt5.Qsci.pyd Carla/PyQt5.QtNetwork.pyd Carla/PyQt5.QtSql.pyd Carla/PyQt5.QtTest.pyd Carla/PyQt5.QtXml.pyd
# rm -f dist/PyQt5.Qsci.pyd dist/PyQt5.QtNetwork.pyd dist/PyQt5.QtSql.pyd dist/PyQt5.QtTest.pyd dist/PyQt5.QtXml.pyd

# cp $WINEPREFIX/drive_c/Python34/python34.dll                                Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/icu*.dll            Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libEGL.dll          Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libGLESv2.dll       Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Core.dll         Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Gui.dll          Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Widgets.dll      Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5OpenGL.dll       Carla/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Svg.dll          Carla/

mv dist Carla/resources
mkdir Carla/resources/lib/_tmp
pushd Carla/resources/lib/_tmp
unzip -o ../library-bigmeter.zip
unzip -o ../library-midipattern.zip
unzip -o ../library-notes.zip
unzip -o ../library-carla1.zip
unzip -o ../library-carla2.zip
zip -r -9 ../library.zip *.pyc
popd
rm -r Carla/resources/lib/_tmp
rm Carla/resources/lib/library-*.zip

# cp $WINEPREFIX/drive_c/Python34/python34.dll                                Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/icu*.dll            Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libEGL.dll          Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/libGLESv2.dll       Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Core.dll         Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Gui.dll          Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Widgets.dll      Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5OpenGL.dll       Carla/resources/
# cp $WINEPREFIX/drive_c/Python34/Lib/site-packages/PyQt5/Qt5Svg.dll          Carla/resources/

mkdir Carla.lv2
cp Carla/*.exe                  Carla.lv2/
cp ../../bin/carla.lv2/*.dll    Carla.lv2/
cp ../../bin/carla.lv2/*.ttl    Carla.lv2/
cp ../../bin/libcarla_utils.dll Carla.lv2/
cp -r Carla/resources           Carla.lv2/

mkdir Carla.vst
cp Carla/*.exe                  Carla.vst/
cp Carla/carla-bridge-lv2.dll   Carla.vst/
cp ../../bin/CarlaVst*.dll      Carla.vst/
cp ../../bin/libcarla_utils.dll Carla.vst/
cp -r Carla/resources           Carla.vst/

rm Carla.lv2/Carla.exe
rm Carla.vst/Carla.exe

if [ x"${CARLA_DEV}" != x"" ]; then
    exit 0
fi

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32 clean
make -C unzipfx-carla -f Makefile.win32 ${MAKE_ARGS}

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
rm -rf ${PKG_FOLDER}
mkdir ${PKG_FOLDER}
cp -r Carla.exe Carla.lv2 Carla.vst README.txt ${PKG_FOLDER}
unix2dos ${PKG_FOLDER}/README.txt
zip -r -9 ${PKG_FOLDER}.zip ${PKG_FOLDER}

cd ../..
