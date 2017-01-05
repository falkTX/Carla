#!/bin/bash

set -e

do_once() {

git clone --depth=1 git://github.com/falkTX/Carla

cd Carla

make distclean
make uninstall EXPERIMENTAL_PLUGINS=true PREFIX=/usr

make -C data/windows/unzipfx-carla/ -f Makefile.linux clean
make -C data/windows/unzipfx-carla-control/ -f Makefile.linux clean

rm -rf Carla CarlaControl *.zip Carla-2.0* *.xz
rm -rf build build-carla build-carla-control build-lv2 build-vst
# extra

apt-get install linuxsampler-static fluidsynth-static ntk-static fftw3-static mxml-static zlib-static liblo-static
apt-get install libclthreads-static libclxclient-static zita-convolver-static zita-resampler-static
apt-get install pyqt4-dev-tools python3 python3-liblo python3-pyqt4 python3-pyqt4.qtopengl python3-sip
apt-get install libqt4-dev libasound2-dev libpulse-dev libmagic-dev libx11-dev libxft-dev
apt-get install libgtk2.0-dev libgl1-mesa-dev libglu1-mesa-dev
apt-get install cx-freeze-python3 zip
# libgtk-3-dev

export MOC_QT4=/usr/bin/moc-qt4
export RCC_QT4=/usr/bin/rcc
export UIC_QT4=/usr/bin/uic-qt4
export PATH=/opt/kxstudio/bin:$PATH
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig

make features

}

if [ ! -f extra/files-downloaded ]; then
mkdir -p extra
cd extra

if (dpkg --print-architecture | grep -q amd64); then
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_1.9.7+git20170105_amd64.deb
else
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_1.9.7+git20170105_i386.deb
fi
dpkg -x carla-git_*.deb .

wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-wine32_1.9.7+git20170107_i386.deb
dpkg -x carla-bridge-wine32_*.deb .

wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win32_1.9.7+git20170105_i386.deb
dpkg -x carla-bridge-win32_*.deb .

if (dpkg --print-architecture | grep -q amd64); then
wget https://github.com/KXStudio/Repository/releases/download/initial/carla-bridge-wine64_1.9.5.git20160114_amd64.deb
dpkg -x carla-bridge-wine64_*.deb .

wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win64_1.9.7+git20170105_amd64.deb
dpkg -x carla-bridge-win64_*.deb .
fi

if (dpkg --print-architecture | grep -q amd64); then
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla_1.9.5-1kxstudio1_amd64.deb
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla-control_1.0.0-0kxstudio1_amd64.deb
else
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla_1.9.5-1kxstudio1_i386.deb
wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla-control_1.0.0-0kxstudio1_i386.deb
fi
dpkg -x unzipfx-carla_*.deb .
dpkg -x unzipfx-carla-control_*.deb .

rm *.deb
touch files-downloaded

cd ..
fi

make EXPERIMENTAL_PLUGINS=true -j 8
# make -C data/windows/unzipfx-carla/ -f Makefile.linux -j 8
# make -C data/windows/unzipfx-carla-control/ -f Makefile.linux -j 8

if (dpkg --print-architecture | grep -q amd64); then
LDFLAGS=-L/usr/lib32 make posix32 -j 8
fi

make install PREFIX=/usr EXPERIMENTAL_PLUGINS=true

mkdir build-carla
mkdir build-carla/resources
mkdir build-carla/src

cp     extra/usr/lib/carla/*.dll      build-carla/
cp     extra/usr/lib/carla/*.exe      build-carla/
cp     extra/usr/lib/carla/*-gtk3     build-carla/
cp     extra/usr/lib/carla/*-qt5      build-carla/
cp -r  /usr/lib/carla/*               build-carla/
cp -LR /usr/share/carla/resources/*   build-carla/resources/
cp     /usr/share/carla/carla         build-carla/src/
cp     /usr/share/carla/carla-control build-carla/src/
cp -r  /usr/share/carla/*.py          build-carla/src/

mv build-carla/resources/carla-plugin   build-carla/resources/carla-plugin.py
mv build-carla/resources/bigmeter-ui    build-carla/resources/bigmeter-ui.py
mv build-carla/resources/midipattern-ui build-carla/resources/midipattern-ui.py
mv build-carla/resources/notes-ui       build-carla/resources/notes-ui.py
rm build-carla/carla-bridge-lv2-modgui

cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/src/carla                   --target-dir=build-carla/
cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/src/carla-control           --target-dir=build-carla-control/
cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/resources/carla-plugin.py   --target-dir=build-carla/resources/
cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/resources/bigmeter-ui.py    --target-dir=build-carla/resources/
cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/resources/midipattern-ui.py --target-dir=build-carla/resources/
cxfreeze-python3 --include-modules=re,sip,subprocess,inspect build-carla/resources/notes-ui.py       --target-dir=build-carla/resources/

cp /usr/lib/libpython3.2mu.so.1.0 build-carla/
cp /usr/lib/libffi.so.5           build-carla/
cp /usr/lib/libmagic.so.1         build-carla/
cp /usr/lib/libssl.so.0.9.8       build-carla/
cp /usr/lib/libcrypto.so.0.9.8    build-carla/
cp /lib/libbz2.so.1.0             build-carla/
cp /lib/libselinux.so.1           build-carla/

cp /usr/lib/libpython3.2mu.so.1.0 build-carla-control/
cp /usr/lib/libmagic.so.1         build-carla-control/
cp /usr/lib/libffi.so.5           build-carla-control/
cp /usr/lib/libssl.so.0.9.8       build-carla-control/
cp /usr/lib/libcrypto.so.0.9.8    build-carla-control/
cp /lib/libbz2.so.1.0             build-carla-control/
cp /lib/libselinux.so.1           build-carla-control/
cp build-carla/libcarla_utils.so  build-carla-control/

find build-carla -name "*.py" -delete
find build-carla -name PyQt4.QtAssistant.so -delete
find build-carla -name PyQt4.QtNetwork.so -delete
find build-carla -name PyQt4.QtScript.so -delete
find build-carla -name PyQt4.QtTest.so -delete
find build-carla -name PyQt4.QtXml.so -delete
rm -rf build-carla/src
rm -f  build-carla/*.def

find build-carla-control -name "*.py" -delete
find build-carla-control -name PyQt4.QtAssistant.so -delete
find build-carla-control -name PyQt4.QtNetwork.so -delete
find build-carla-control -name PyQt4.QtScript.so -delete
find build-carla-control -name PyQt4.QtTest.so -delete
find build-carla-control -name PyQt4.QtXml.so -delete
rm -rf build-carla-control/src
rm -f  build-carla-control/*.def

cd build-carla/resources/ && \
  rm *.so* carla-plugin-patchbay && \
  ln -s ../*.so* . && \
  ln -s carla-plugin carla-plugin-patchbay && \
cd ../..

mv build-carla carla
zip --symlinks -r -9 carla.zip carla
cat extra/opt/carla/unzipfx2cat carla.zip > Carla
chmod +x Carla
mv carla build-carla

mv build-carla-control carla-control
zip --symlinks -r -9 carla-control.zip carla-control
cat extra/opt/carla-control/unzipfx2cat carla-control.zip > CarlaControl
chmod +x CarlaControl
mv carla-control build-carla-control

mkdir build-lv2
cp -LR /usr/lib/lv2/carla.lv2 build-lv2/
rm -r  build-lv2/carla.lv2/resources
cp -LR build-carla/resources build-lv2/carla.lv2/
cp     extra/usr/lib/carla/*.dll  build-lv2/carla.lv2/
cp     extra/usr/lib/carla/*.exe  build-lv2/carla.lv2/
cp     extra/usr/lib/carla/*-gtk3 build-lv2/carla.lv2/
cp     extra/usr/lib/carla/*-qt5  build-lv2/carla.lv2/

mkdir build-vst
cp -LR /usr/lib/vst/carla.vst build-vst/
rm -r  build-vst/carla.vst/resources
cp -LR build-carla/resources build-vst/carla.vst/
cp     extra/usr/lib/carla/*.dll  build-vst/carla.vst/
cp     extra/usr/lib/carla/*.exe  build-vst/carla.vst/
cp     extra/usr/lib/carla/*-gtk3 build-vst/carla.vst/
cp     extra/usr/lib/carla/*-qt5  build-vst/carla.vst/

if (dpkg --print-architecture | grep -q amd64); then
FOLDER="Carla-2.0beta5-linux64"
else
FOLDER="Carla-2.0beta5-linux32"
fi

mkdir $FOLDER
cp data/linux/README $FOLDER/
mv Carla CarlaControl build-lv2/*.lv2 build-vst/*.vst $FOLDER/
tar cJf $FOLDER.tar.xz $FOLDER
