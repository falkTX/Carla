#!/bin/bash

# ------------------------------------------------------------------------------------
# stop on error

set -e

VERSION="2.2.0-RC1"
PKG_FOLDER="Carla_${VERSION}-macos"

# ------------------------------------------------------------------------------------
# cd to correct path

if [ ! -f Makefile ]; then
  cd ../..
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source data/macos/common.env

if [ $(clang -v  2>&1 | grep version | cut -d' ' -f4 | cut -d'.' -f1) -lt 11 ]; then
  export MACOS_VERSION_MIN="10.8"
else
  export MACOS_VERSION_MIN="10.12"
  PKG_FOLDER="${PKG_FOLDER}-10.12"
fi

export MACOS="true"
export USING_JUCE="true"

export CC=clang
export CXX=clang++

unset CPPFLAGS

##############################################################################################
# Complete 64bit build

export CFLAGS="-I${TARGETDIR}/carla64/include -m64 -mmacosx-version-min=${MACOS_VERSION_MIN}"
export CXXFLAGS="${CFLAGS} -stdlib=libc++ -Wno-unknown-pragmas -Wno-unused-private-field -Werror=auto-var-id"
export LDFLAGS="-L${TARGETDIR}/carla64/lib -m64 -mmacosx-version-min=${MACOS_VERSION_MIN} -stdlib=libc++"

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla/lib/pkgconfig:${TARGETDIR}/carla64/lib/pkgconfig

export MOC_QT5=moc
export RCC_QT5=rcc
export UIC_QT5=uic

make USING_JUCE=${USING_JUCE} USING_JUCE_AUDIO_DEVICES=${USING_JUCE} ${MAKE_ARGS}

##############################################################################################
# Build 32bit bridges

if [ "${MACOS_VERSION_MIN}" != "10.12" ]; then

export CFLAGS="-I${TARGETDIR}/carla32/include -m32 -mmacosx-version-min=${MACOS_VERSION_MIN}"
export CXXFLAGS="${CFLAGS} -stdlib=libc++ -Wno-unknown-pragmas -Wno-unused-private-field -Werror=auto-var-id"
export LDFLAGS="-L${TARGETDIR}/carla32/lib -m32 -mmacosx-version-min=${MACOS_VERSION_MIN} -stdlib=libc++"

export PATH=${TARGETDIR}/carla32/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${TARGETDIR}/carla32/lib/pkgconfig

make USING_JUCE=${USING_JUCE} posix32 ${MAKE_ARGS}

fi

##############################################################################################
# Build Mac App

export PATH=${TARGETDIR}/carla/bin:${TARGETDIR}/carla64/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PYTHONPATH=$(pwd)/source/frontend
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
rm -rf ./build/*.vst

cp ./source/frontend/carla          ./source/frontend/Carla.pyw
cp ./source/frontend/carla-control  ./source/frontend/Carla-Control.pyw
cp ./source/frontend/carla-plugin   ./source/frontend/carla-plugin.pyw
cp ./source/frontend/bigmeter-ui    ./source/frontend/bigmeter-ui.pyw
cp ./source/frontend/midipattern-ui ./source/frontend/midipattern-ui.pyw
cp ./source/frontend/notes-ui       ./source/frontend/notes-ui.pyw
env SCRIPT_NAME=Carla          python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
env SCRIPT_NAME=Carla-Control  python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla-Control
env SCRIPT_NAME=carla-plugin   python3 ./data/macos/bundle.py bdist_mac --bundle-name=carla-plugin
env SCRIPT_NAME=bigmeter-ui    python3 ./data/macos/bundle.py bdist_mac --bundle-name=bigmeter-ui
env SCRIPT_NAME=midipattern-ui python3 ./data/macos/bundle.py bdist_mac --bundle-name=midipattern-ui
env SCRIPT_NAME=notes-ui       python3 ./data/macos/bundle.py bdist_mac --bundle-name=notes-ui
rm ./source/frontend/*.pyw

mkdir -p build/Carla.app/Contents/MacOS/resources
mkdir -p build/Carla.app/Contents/MacOS/styles
mkdir -p build/Carla-Control.app/Contents/MacOS/styles

cp bin/*carla*.dylib     build/Carla.app/Contents/MacOS/
cp bin/carla-bridge-*    build/Carla.app/Contents/MacOS/
cp bin/carla-discovery-* build/Carla.app/Contents/MacOS/
cp bin/styles/*          build/Carla.app/Contents/MacOS/styles/

cp bin/*utils.dylib      build/Carla-Control.app/Contents/MacOS/
cp bin/styles/*          build/Carla-Control.app/Contents/MacOS/styles/

rm -f build/Carla.app/Contents/MacOS/carla-bridge-lv2-modgui
rm -f build/Carla.app/Contents/MacOS/carla-bridge-lv2-qt5

find build/ -type f -name "*.py" -delete
find build/ -type f -name "*.pyi" -delete
find build/ -type f -name "pylupdate.so" -delete
find build/ -type f -name "pyrcc.so" -delete
find build/ -type f -name "QtMacExtras*" -delete
find build/ -type f -name "QtNetwork*" -delete
find build/ -type f -name "QtSql*" -delete
find build/ -type f -name "QtTest*" -delete
find build/ -type f -name "QtXml*" -delete
if [ "${MACOS_VERSION_MIN}" != "10.12" ]; then
find build/ -type f -name "*.pyc" -delete
fi
rm -rf build/Carla.app/Contents/MacOS/lib/PyQt5/uic
rm -rf build/Carla.app/Contents/MacOS/resources/__pycache__
rm -rf build/Carla.app/Contents/MacOS/resources/patchcanvas
rm -rf build/Carla.app/Contents/MacOS/resources/widgets
rm -rf build/Carla.app/Contents/MacOS/resources/zynaddsubfx
rm -rf build/Carla-Control.app/Contents/MacOS/resources/__pycache__

# missed by cx-freeze
mkdir build/Carla.app/Contents/MacOS/iconengines
cp ${TARGETDIR}/carla/lib/qt5/plugins/iconengines/libqsvgicon.dylib build/Carla.app/Contents/MacOS/iconengines/
if [ "${MACOS_VERSION_MIN}" = "10.12" ]; then
mkdir build/Carla.app/Contents/MacOS/imageformats
mkdir build/Carla.app/Contents/MacOS/platforms
cp ${TARGETDIR}/carla/lib/qt5/plugins/imageformats/libq{jpeg,svg}.dylib             build/Carla.app/Contents/MacOS/imageformats/
cp ${TARGETDIR}/carla/lib/qt5/plugins/platforms/libq{cocoa,minimal,offscreen}.dylib build/Carla.app/Contents/MacOS/platforms/
fi

# continuing...
cd build/Carla.app/Contents/MacOS
for f in `find . -type f | grep -e Q -e libq -e carlastyle.dylib`; do
if [ "${MACOS_VERSION_MIN}" = "10.12" ] && (echo "$f" | grep -q pyc); then continue; fi
install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $f
install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $f
install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $f
install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $f
install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $f
install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $f
install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    $f
done
if [ "${MACOS_VERSION_MIN}" = "10.12" ]; then
cp ${TARGETDIR}/carla/lib/QtCore.framework/Versions/5/QtCore .
cp ${TARGETDIR}/carla/lib/QtGui.framework/Versions/5/QtGui .
cp ${TARGETDIR}/carla/lib/QtOpenGL.framework/Versions/5/QtOpenGL .
cp ${TARGETDIR}/carla/lib/QtPrintSupport.framework/Versions/5/QtPrintSupport .
cp ${TARGETDIR}/carla/lib/QtSvg.framework/Versions/5/QtSvg .
cp ${TARGETDIR}/carla/lib/QtWidgets.framework/Versions/5/QtWidgets .
cp ${TARGETDIR}/carla/lib/QtMacExtras.framework/Versions/5/QtMacExtras .
fi
cd ../../../..

cd build/Carla-Control.app/Contents/MacOS
for f in `find . -type f | grep -e Q -e libq -e carlastyle.dylib`; do
if [ "${MACOS_VERSION_MIN}" = "10.12" ] && (echo "$f" | grep -q pyc); then continue; fi
install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $f
install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $f
install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $f
install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $f
install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $f
install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $f
install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    $f
done
if [ "${MACOS_VERSION_MIN}" = "10.12" ]; then
cp ${TARGETDIR}/carla/lib/QtCore.framework/Versions/5/QtCore .
cp ${TARGETDIR}/carla/lib/QtGui.framework/Versions/5/QtGui .
cp ${TARGETDIR}/carla/lib/QtOpenGL.framework/Versions/5/QtOpenGL .
cp ${TARGETDIR}/carla/lib/QtPrintSupport.framework/Versions/5/QtPrintSupport .
cp ${TARGETDIR}/carla/lib/QtSvg.framework/Versions/5/QtSvg .
cp ${TARGETDIR}/carla/lib/QtWidgets.framework/Versions/5/QtWidgets .
cp ${TARGETDIR}/carla/lib/QtMacExtras.framework/Versions/5/QtMacExtras .
fi
cd ../../../..

mv build/carla-plugin.app/Contents/MacOS/carla-plugin     build/Carla.app/Contents/MacOS/resources/
mv build/bigmeter-ui.app/Contents/MacOS/bigmeter-ui       build/Carla.app/Contents/MacOS/resources/
mv build/midipattern-ui.app/Contents/MacOS/midipattern-ui build/Carla.app/Contents/MacOS/resources/
mv build/notes-ui.app/Contents/MacOS/notes-ui             build/Carla.app/Contents/MacOS/resources/

mv build/Carla.app/Contents/MacOS/lib/library.zip          build/Carla.app/Contents/MacOS/lib/library-carla1.zip
mv build/carla-plugin.app/Contents/MacOS/lib/library.zip   build/Carla.app/Contents/MacOS/lib/library-carla2.zip
mv build/bigmeter-ui.app/Contents/MacOS/lib/library.zip    build/Carla.app/Contents/MacOS/lib/library-bigmeter.zip
mv build/midipattern-ui.app/Contents/MacOS/lib/library.zip build/Carla.app/Contents/MacOS/lib/library-midipattern.zip
mv build/notes-ui.app/Contents/MacOS/lib/library.zip       build/Carla.app/Contents/MacOS/lib/library-notes.zip

mkdir build/Carla.app/Contents/MacOS/lib/_lib
pushd build/Carla.app/Contents/MacOS/lib/_lib
unzip -o ../library-bigmeter.zip
unzip -o ../library-midipattern.zip
unzip -o ../library-notes.zip
unzip -o ../library-carla2.zip
unzip -o ../library-carla1.zip
zip -r -9 ../library.zip *
popd
rm -r build/Carla.app/Contents/MacOS/lib/_lib build/Carla.app/Contents/MacOS/lib/library-*.zip

rm -rf build/carla-plugin.app build/bigmeter-ui.app build/midipattern-ui.app build/notes-ui.app

cd build/Carla.app/Contents/MacOS/resources/
ln -sf ../Qt* ../lib ../iconengines ../imageformats ../platforms ../styles .
ln -sf carla-plugin carla-plugin-patchbay
cd ../../../../..

mkdir build/carla.lv2
mkdir build/carla.lv2/resources
mkdir build/carla.lv2/styles

cp bin/carla.lv2/*.*        build/carla.lv2/
cp bin/carla-bridge-*       build/carla.lv2/
cp bin/carla-discovery-*    build/carla.lv2/
cp bin/libcarla_utils.dylib build/carla.lv2/
rm -f build/carla.lv2/carla-bridge-lv2-modgui
rm -f build/carla.lv2/carla-bridge-lv2-qt5
cp -LR build/Carla.app/Contents/MacOS/resources/* build/carla.lv2/resources/
cp     build/Carla.app/Contents/MacOS/styles/*    build/carla.lv2/styles/

./data/macos/generate-vst-bundles.sh
mv bin/CarlaVstShell.vst   build/carla.vst
mv bin/CarlaVstFxShell.vst build/carlafx.vst
rm -rf bin/*.vst

mkdir build/carla.vst/Contents/MacOS/resources
mkdir build/carla.vst/Contents/MacOS/styles

mkdir build/carlafx.vst/Contents/MacOS/resources
mkdir build/carlafx.vst/Contents/MacOS/styles

cp bin/carla-bridge-*       build/carla.vst/Contents/MacOS/
cp bin/carla-discovery-*    build/carla.vst/Contents/MacOS/
cp bin/libcarla_utils.dylib build/carla.vst/Contents/MacOS/
rm -f build/carla.vst/carla-bridge-lv2-modgui
rm -f build/carla.vst/carla-bridge-lv2-qt5
cp -LR build/Carla.app/Contents/MacOS/resources/* build/carla.vst/Contents/MacOS/resources/
cp     build/Carla.app/Contents/MacOS/styles/*    build/carla.vst/Contents/MacOS/styles/

cp bin/carla-bridge-*       build/carlafx.vst/Contents/MacOS/
cp bin/carla-discovery-*    build/carlafx.vst/Contents/MacOS/
cp bin/libcarla_utils.dylib build/carlafx.vst/Contents/MacOS/
rm -f build/carlafx.vst/carla-bridge-lv2-modgui
rm -f build/carlafx.vst/carla-bridge-lv2-qt5
cp -LR build/Carla.app/Contents/MacOS/resources/* build/carlafx.vst/Contents/MacOS/resources/
cp     build/Carla.app/Contents/MacOS/styles/*    build/carlafx.vst/Contents/MacOS/styles/

##############################################################################################

rm -rf ${PKG_FOLDER}
mkdir ${PKG_FOLDER}
cp data/macos/README ${PKG_FOLDER}/
mv build/carla.lv2   ${PKG_FOLDER}/
mv build/carla.vst   ${PKG_FOLDER}/
mv build/carlafx.vst ${PKG_FOLDER}/
mv build/Carla.app   ${PKG_FOLDER}/
mv build/Carla-Control.app ${PKG_FOLDER}/

##############################################################################################
# Build Mac plugin installer

pkgbuild \
    --identifier "studio.kx.carla.lv2" \
    --install-location "/Library/Audio/Plug-Ins/LV2/carla.lv2/" \
    --root "${PKG_FOLDER}/carla.lv2/" \
    "${PKG_FOLDER}/carla-lv2.pkg"

pkgbuild \
    --identifier "studio.kx.carla.vst2fx" \
    --install-location "/Library/Audio/Plug-Ins/VST/carlafx.vst/" \
    --root "${PKG_FOLDER}/carlafx.vst/" \
    "${PKG_FOLDER}/carla-vst2fx.pkg"

pkgbuild \
    --identifier "studio.kx.carla.vst2syn" \
    --install-location "/Library/Audio/Plug-Ins/VST/carla.vst/" \
    --root "${PKG_FOLDER}/carla.vst/" \
    "${PKG_FOLDER}/carla-vst2syn.pkg"

productbuild \
    --distribution data/macos/package.xml \
    --identifier studio.kx.carla \
    --package-path "${PKG_FOLDER}" \
    --version ${VERSION} \
    "${PKG_FOLDER}/Carla-Plugins.pkg"

rm -r ${PKG_FOLDER}/carla.lv2
rm -r ${PKG_FOLDER}/carla.vst
rm -r ${PKG_FOLDER}/carlafx.vst

rm ${PKG_FOLDER}/carla-lv2.pkg
rm ${PKG_FOLDER}/carla-vst2fx.pkg
rm ${PKG_FOLDER}/carla-vst2syn.pkg

##############################################################################################
