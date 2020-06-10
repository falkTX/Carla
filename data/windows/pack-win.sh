#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check input

ARCH="${1}"
ARCH_PREFIX="${1}"

if [ x"${ARCH}" != x"32" ] && [ x"${ARCH}" != x"32nosse" ] && [ x"${ARCH}" != x"64" ]; then
  echo "usage: $0 32|32nosse|64"
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

PKG_FOLDER="Carla_2.1.1-win${ARCH}"

export WIN32=true

if [ x"${ARCH}" != x"32" ]; then
  export WIN64=true
  CPUARCH="x86_64"
else
  CPUARCH="i686"
fi

export PYTHONPATH="$(pwd)/source/frontend"

# ---------------------------------------------------------------------------------------------------------------------

export_vars() {

local _ARCH="${1}"
local _ARCH_PREFIX="${2}"
local _MINGW_PREFIX="${3}-w64-mingw32"

export DEPS_PREFIX=${TARGETDIR}/carla-w${ARCH_PREFIX}
export MSYS2_PREFIX=${TARGETDIR}/msys2-${CPUARCH}/mingw${ARCH}

export PATH=${DEPS_PREFIX}/bin:/opt/wine-staging/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PKG_CONFIG_PATH=${DEPS_PREFIX}/lib/pkgconfig:${MSYS2_PREFIX}/lib/pkgconfig

export AR=${_MINGW_PREFIX}-ar
export CC=${_MINGW_PREFIX}-gcc
export CXX=${_MINGW_PREFIX}-g++
export STRIP=${_MINGW_PREFIX}-strip
export WINDRES=${_MINGW_PREFIX}-windres

export CFLAGS="-DPTW32_STATIC_LIB -DFLUIDSYNTH_NOT_A_DLL"
export CFLAGS="${CFLAGS} -I${DEPS_PREFIX}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${DEPS_PREFIX}/lib"

export WINEARCH=win${ARCH}
export WINEDEBUG=-all
export WINEPREFIX=~/.winepy3_x${ARCH}
export PYTHON_EXE="wine ${MSYS2_PREFIX}/bin/python.exe"
export CXFREEZE="${PYTHON_EXE} ${MSYS2_PREFIX}/bin/cxfreeze"

}

# ---------------------------------------------------------------------------------------------------------------------

export_vars "${ARCH}" "${ARCH_PREFIX}" "${CPUARCH}"

# ---------------------------------------------------------------------------------------------------------------------

cd data/windows/

rm -rf Carla Carla.lv2 Carla.vst CarlaControl

# ---------------------------------------------------------------------------------------------------------------------
# freeze carla (console and gui)

${PYTHON_EXE} ./app-console.py build_exe
mv Carla/lib/library.zip Carla/lib/library-console.zip
${PYTHON_EXE} ./app-gui.py build_exe
mv Carla/lib/library.zip Carla/lib/library-gui.zip

mkdir Carla/lib/_lib
pushd Carla/lib/_lib
unzip -o ../library-console.zip
unzip -o ../library-gui.zip
zip -r -9 ../library.zip *
popd
rm -r Carla/lib/_lib Carla/lib/library-*.zip

# ---------------------------------------------------------------------------------------------------------------------
# freeze carla-control

${PYTHON_EXE} ./app-control.py build_exe

# ---------------------------------------------------------------------------------------------------------------------
# freeze pyqt plugin uis (resources)

TARGET_NAME="bigmeter-ui" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-bigmeter.zip
TARGET_NAME="midipattern-ui" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-midipattern.zip
TARGET_NAME="notes-ui" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-notes.zip
TARGET_NAME="carla-plugin" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-carla1.zip
TARGET_NAME="carla-plugin-patchbay" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-carla2.zip

mkdir Carla/resources/lib/_lib
pushd Carla/resources/lib/_lib
unzip -o ../library-bigmeter.zip
unzip -o ../library-midipattern.zip
unzip -o ../library-notes.zip
unzip -o ../library-carla1.zip
unzip -o ../library-carla2.zip
zip -r -9 ../library.zip *
popd
rm -r Carla/resources/lib/_lib Carla/resources/lib/library-*.zip

# ---------------------------------------------------------------------------------------------------------------------
# cleanup pyqt stuff

find Carla CarlaControl -name "QAxContainer*" -delete
find Carla CarlaControl -name "QtDBus*" -delete
find Carla CarlaControl -name "QtDesigner*" -delete
find Carla CarlaControl -name "QtBluetooth*" -delete
find Carla CarlaControl -name "QtHelp*" -delete
find Carla CarlaControl -name "QtLocation*" -delete
find Carla CarlaControl -name "QtMultimedia*" -delete
find Carla CarlaControl -name "QtMultimediaWidgets*" -delete
find Carla CarlaControl -name "QtNetwork*" -delete
find Carla CarlaControl -name "QtNetworkAuth*" -delete
find Carla CarlaControl -name "QtNfc*" -delete
find Carla CarlaControl -name "QtPositioning*" -delete
find Carla CarlaControl -name "QtPrintSupport*" -delete
find Carla CarlaControl -name "QtQml*" -delete
find Carla CarlaControl -name "QtQuick*" -delete
find Carla CarlaControl -name "QtQuickWidgets*" -delete
find Carla CarlaControl -name "QtRemoteObjects*" -delete
find Carla CarlaControl -name "QtSensors*" -delete
find Carla CarlaControl -name "QtSerialPort*" -delete
find Carla CarlaControl -name "QtSql*" -delete
find Carla CarlaControl -name "QtTest*" -delete
find Carla CarlaControl -name "QtWebChannel*" -delete
find Carla CarlaControl -name "QtWebKit*" -delete
find Carla CarlaControl -name "QtWebKitWidgets*" -delete
find Carla CarlaControl -name "QtWebSockets*" -delete
find Carla CarlaControl -name "QtWinExtras*" -delete
find Carla CarlaControl -name "QtXml*" -delete
find Carla CarlaControl -name "QtXmlPatterns*" -delete
find Carla CarlaControl -name "*.pyi" -delete

# ---------------------------------------------------------------------------------------------------------------------
# copy relevant files to binary dir

cp ../../bin/libcarla_standalone2.dll Carla/
cp ../../bin/libcarla_utils.dll       Carla/
cp ../../bin/carla-bridge-lv2.dll     Carla/
cp ../../bin/carla-bridge-*.exe       Carla/
cp ../../bin/carla-discovery-win*.exe Carla/

mkdir -p Carla/imageformats
cp ${MSYS2_PREFIX}/share/qt5/plugins/imageformats/qsvg.dll Carla/imageformats/

mkdir -p Carla/platforms
cp ${MSYS2_PREFIX}/share/qt5/plugins/platforms/qwindows.dll Carla/platforms/

mkdir -p Carla/styles
cp ../../bin/styles/carlastyle.dll Carla/styles/

# ---------------------------------------------------------------------------------------------------------------------
# also for CarlaControl

cp ../../bin/libcarla_utils.dll CarlaControl/

mkdir -p CarlaControl/imageformats
cp ${MSYS2_PREFIX}/share/qt5/plugins/imageformats/qsvg.dll CarlaControl/imageformats/

mkdir -p CarlaControl/platforms
cp ${MSYS2_PREFIX}/share/qt5/plugins/platforms/qwindows.dll CarlaControl/platforms/

mkdir -p CarlaControl/styles
cp ../../bin/styles/carlastyle.dll CarlaControl/styles/

# ---------------------------------------------------------------------------------------------------------------------
# prepare lv2 bundle

mkdir -p Carla.lv2
cp -r Carla/*                Carla.lv2/
cp ../../bin/carla.lv2/*.dll Carla.lv2/
cp ../../bin/carla.lv2/*.ttl Carla.lv2/
rm Carla.lv2/Carla.exe
rm Carla.lv2/CarlaDebug.exe

# ---------------------------------------------------------------------------------------------------------------------
# copy relevant files to binary dir

mkdir -p Carla.vst
cp -r Carla/*              Carla.vst/
cp ../../bin/CarlaVst*.dll Carla.vst/
rm Carla.vst/Carla.exe
rm Carla.vst/CarlaDebug.exe

# ---------------------------------------------------------------------------------------------------------------------
# stop here for development

if [ x"${CARLA_DEV}" != x"" ]; then
    exit 0
fi

# ---------------------------------------------------------------------------------------------------------------------
# create self-contained zip

# Build unzipfx
make -C unzipfx-carla -f Makefile.win32 clean
make -C unzipfx-carla -f Makefile.win32 ${MAKE_ARGS}

# Build unzipfx-control
make -C unzipfx-carla-control -f Makefile.win32 clean
make -C unzipfx-carla-control -f Makefile.win32 ${MAKE_ARGS}

# Create zip of Carla and CarlaControl
rm -f Carla.zip CarlaControl.zip
mv Carla Carla-2.1
mv CarlaControl CarlaControl-2.1
zip -r -9 Carla.zip Carla-2.1
zip -r -9 CarlaControl.zip CarlaControl-2.1

# Create static builds
rm -f Carla.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
chmod +x Carla.exe

rm -f CarlaControl.exe
cat unzipfx-carla-control/unzipfx2cat.exe CarlaControl.zip > CarlaControl.exe
chmod +x CarlaControl.exe

# Cleanup
rm -f Carla.zip CarlaControl.zip
rm -rf Carla-2.1 CarlaControl-2.1

# Create release zip
rm -rf ${PKG_FOLDER}
mkdir ${PKG_FOLDER}
cp -r Carla.exe CarlaControl.exe Carla.lv2 Carla.vst README.txt ${PKG_FOLDER}
unix2dos ${PKG_FOLDER}/README.txt
zip -r -9 ${PKG_FOLDER}.zip ${PKG_FOLDER}

cd ../..

# ---------------------------------------------------------------------------------------------------------------------
