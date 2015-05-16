#!/bin/bash

set -e

if [ -f Makefile ]; then
  cd data/linux
fi

VERSION="1.9.5~git20150311.4"

if [ ! -f carla-git-static_"$VERSION"_amd64.deb ]; then
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/carla-git-static_"$VERSION"_amd64.deb
    dpkg -x carla-git-static_"$VERSION"_amd64.deb carla-git-static_"$VERSION"_amd64
fi

if [ ! -f carla-git-static_"$VERSION"_i386.deb ]; then
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/carla-git-static_"$VERSION"_i386.deb
    dpkg -x carla-git-static_"$VERSION"_i386.deb carla-git-static_"$VERSION"_i386
fi

if [ ! -f unzipfx2cat32 ]; then
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla_1.9.5-1kxstudio1_i386.deb
    dpkg -x unzipfx-carla_1.9.5-1kxstudio1_i386.deb tmpfx
    mv tmpfx/opt/carla/unzipfx2cat unzipfx2cat32
    rm -rf tmpfx
fi

if [ ! -f unzipfx2cat64 ]; then
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/testing/+files/unzipfx-carla_1.9.5-1kxstudio1_amd64.deb
    dpkg -x unzipfx-carla_1.9.5-1kxstudio1_amd64.deb tmpfx
    mv tmpfx/opt/carla/unzipfx2cat unzipfx2cat64
    rm -rf tmpfx
fi

rm -rf Carla-2.0beta4-linux32
mkdir Carla-2.0beta4-linux32
cp -r carla-git-static_"$VERSION"_i386/opt/carla/Carla \
      carla-git-static_"$VERSION"_i386/opt/carla/carla.lv2/ \
      carla-git-static_"$VERSION"_i386/opt/carla/carla.vst/ Carla-2.0beta4-linux32
rm -r carla-git-static_"$VERSION"_i386

rm -rf Carla-2.0beta4-linux64
mkdir Carla-2.0beta4-linux64
cp -r carla-git-static_"$VERSION"_amd64/opt/carla/Carla \
      carla-git-static_"$VERSION"_amd64/opt/carla/carla.lv2/ \
      carla-git-static_"$VERSION"_amd64/opt/carla/carla.vst/ Carla-2.0beta4-linux64
rm -r carla-git-static_"$VERSION"_amd64

cd Carla-2.0beta4-linux32
unzip Carla || true
rm -f Carla carla/*posix32 carla.lv2/*posix32 carla.vst/*posix32
cd ..

cd Carla-2.0beta4-linux64
unzip Carla || true
rm -f Carla carla/*posix64 carla.lv2/*posix64 carla.vst/*posix64
cd ..

cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux32/carla/
cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux32/carla.lv2/
cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux32/carla.vst/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux32/carla/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux32/carla.lv2/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux32/carla.vst/

cp /usr/lib/carla/*posix32    Carla-2.0beta4-linux64/carla/
cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux64/carla/
cp /usr/lib/carla/*win64.exe  Carla-2.0beta4-linux64/carla/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux64/carla/
cp /usr/lib/carla/*wine64.dll Carla-2.0beta4-linux64/carla/
cp /usr/lib/carla/*posix32    Carla-2.0beta4-linux64/carla.lv2/
cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux64/carla.lv2/
cp /usr/lib/carla/*win64.exe  Carla-2.0beta4-linux64/carla.lv2/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux64/carla.lv2/
cp /usr/lib/carla/*wine64.dll Carla-2.0beta4-linux64/carla.lv2/
cp /usr/lib/carla/*posix32    Carla-2.0beta4-linux64/carla.vst/
cp /usr/lib/carla/*win32.exe  Carla-2.0beta4-linux64/carla.vst/
cp /usr/lib/carla/*win64.exe  Carla-2.0beta4-linux64/carla.vst/
cp /usr/lib/carla/*wine32.dll Carla-2.0beta4-linux64/carla.vst/
cp /usr/lib/carla/*wine64.dll Carla-2.0beta4-linux64/carla.vst/

cd Carla-2.0beta4-linux32
zip --symlinks -r -9 carla.zip carla
cat ../unzipfx2cat32 carla.zip > Carla
chmod +x Carla
rm -r carla carla.zip
cd ..

cd Carla-2.0beta4-linux64
zip --symlinks -r -9 carla.zip carla
cat ../unzipfx2cat64 carla.zip > Carla
chmod +x Carla
rm -r carla carla.zip
cd ..

cp README Carla-2.0beta4-linux32/
cp README Carla-2.0beta4-linux64/
