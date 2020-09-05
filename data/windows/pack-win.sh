#!/bin/bash

VERSION="2.2.0-RC1"

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

PKG_FOLDER="Carla_${VERSION}-win${ARCH}"

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
TARGET_NAME="xycontroller-ui" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-xycontroller.zip
TARGET_NAME="carla-plugin" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-carla1.zip
TARGET_NAME="carla-plugin-patchbay" ${PYTHON_EXE} ./app-plugin-ui.py build_exe
mv Carla/resources/lib/library.zip Carla/resources/lib/library-carla2.zip

mkdir Carla/resources/lib/_lib
pushd Carla/resources/lib/_lib
unzip -o ../library-bigmeter.zip
unzip -o ../library-midipattern.zip
unzip -o ../library-notes.zip
unzip -o ../library-xycontroller.zip
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

cp ../../bin/libcarla_utils.dll Carla/

mkdir -p Carla/imageformats
cp ${MSYS2_PREFIX}/share/qt5/plugins/imageformats/qsvg.dll Carla/imageformats/

mkdir -p Carla/platforms
cp ${MSYS2_PREFIX}/share/qt5/plugins/platforms/qwindows.dll Carla/platforms/

mkdir -p Carla/styles
cp ../../bin/styles/carlastyle.dll Carla/styles/

cp ${MSYS2_PREFIX}/bin/Qt5{Core,Gui,OpenGL,Svg,Widgets}.dll Carla/

cp ${MSYS2_PREFIX}/bin/libbz2-1.dll             Carla/
cp ${MSYS2_PREFIX}/bin/libcrypto-1_1-x64.dll    Carla/
cp ${MSYS2_PREFIX}/bin/libdouble-conversion.dll Carla/
cp ${MSYS2_PREFIX}/bin/libffi-6.dll             Carla/
cp ${MSYS2_PREFIX}/bin/libfreetype-6.dll        Carla/
cp ${MSYS2_PREFIX}/bin/libgcc_s_seh-1.dll       Carla/
cp ${MSYS2_PREFIX}/bin/libglib-2.0-0.dll        Carla/
cp ${MSYS2_PREFIX}/bin/libgraphite2.dll         Carla/
cp ${MSYS2_PREFIX}/bin/libharfbuzz-0.dll        Carla/
cp ${MSYS2_PREFIX}/bin/libiconv-2.dll           Carla/
cp ${MSYS2_PREFIX}/bin/libicudt64.dll           Carla/
cp ${MSYS2_PREFIX}/bin/libicuin64.dll           Carla/
cp ${MSYS2_PREFIX}/bin/libicuuc64.dll           Carla/
cp ${MSYS2_PREFIX}/bin/libintl-8.dll            Carla/
cp ${MSYS2_PREFIX}/bin/libpcre-1.dll            Carla/
cp ${MSYS2_PREFIX}/bin/libpcre2-16-0.dll        Carla/
cp ${MSYS2_PREFIX}/bin/libpng16-16.dll          Carla/
cp ${MSYS2_PREFIX}/bin/libpython3.7m.dll        Carla/
cp ${MSYS2_PREFIX}/bin/libstdc++-6.dll          Carla/
cp ${MSYS2_PREFIX}/bin/libwinpthread-1.dll      Carla/
cp ${MSYS2_PREFIX}/bin/libzstd.dll              Carla/
cp ${MSYS2_PREFIX}/bin/zlib1.dll                Carla/

# ---------------------------------------------------------------------------------------------------------------------
# also for CarlaControl

cp Carla/*.dll CarlaControl/

mkdir -p CarlaControl/imageformats
cp ${MSYS2_PREFIX}/share/qt5/plugins/imageformats/qsvg.dll CarlaControl/imageformats/

mkdir -p CarlaControl/platforms
cp ${MSYS2_PREFIX}/share/qt5/plugins/platforms/qwindows.dll CarlaControl/platforms/

mkdir -p CarlaControl/styles
cp ../../bin/styles/carlastyle.dll CarlaControl/styles/

# ---------------------------------------------------------------------------------------------------------------------
# Carla standalone specifics

cp ../../bin/libcarla_standalone2.dll Carla/
cp ../../bin/carla-bridge-lv2.dll     Carla/
cp ../../bin/carla-bridge-*.exe       Carla/
cp ../../bin/carla-discovery-win*.exe Carla/

# ---------------------------------------------------------------------------------------------------------------------
# prepare lv2 bundle

mkdir -p Carla.lv2
cp -r Carla/*                Carla.lv2/
cp ../../bin/carla.lv2/*.dll Carla.lv2/
cp ../../bin/carla.lv2/*.ttl Carla.lv2/
mv Carla.lv2/{Qt5,lib,zlib}*.dll Carla.lv2/resources/
mv Carla.lv2/resources/libcarla_utils.dll Carla.lv2/
rm Carla.lv2/Carla.exe
rm Carla.lv2/CarlaDebug.exe
rm Carla.lv2/resources/libcarla_standalone2.dll
rm -r Carla.lv2/lib

# ---------------------------------------------------------------------------------------------------------------------
# copy relevant files to binary dir

mkdir -p Carla.vst
cp -r Carla/*              Carla.vst/
cp ../../bin/CarlaVst*.dll Carla.vst/
mv Carla.vst/{Qt5,lib,zlib}*.dll Carla.vst/resources/
mv Carla.vst/resources/libcarla_utils.dll Carla.vst/
rm Carla.vst/Carla.exe
rm Carla.vst/CarlaDebug.exe
rm Carla.vst/resources/libcarla_standalone2.dll
rm -r Carla.vst/lib

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
mv Carla Carla-${VERSION}
mv CarlaControl CarlaControl-${VERSION}
zip -r -9 Carla.zip Carla-${VERSION}
zip -r -9 CarlaControl.zip CarlaControl-${VERSION}

# Create static builds
rm -f Carla.exe
cat unzipfx-carla/unzipfx2cat.exe Carla.zip > Carla.exe
chmod +x Carla.exe

rm -f CarlaControl.exe
cat unzipfx-carla-control/unzipfx2cat.exe CarlaControl.zip > CarlaControl.exe
chmod +x CarlaControl.exe

# Cleanup
rm -f Carla.zip CarlaControl.zip
rm -rf Carla-${VERSION} CarlaControl-${VERSION}

# Create release zip
rm -rf ${PKG_FOLDER}
mkdir ${PKG_FOLDER}
cp -r Carla.exe CarlaControl.exe Carla.lv2 Carla.vst README.txt ${PKG_FOLDER}
unix2dos ${PKG_FOLDER}/README.txt
zip -r -9 ${PKG_FOLDER}.zip ${PKG_FOLDER}

cd ../..

# ---------------------------------------------------------------------------------------------------------------------
