#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# check dependencies

if ! which debootstrap > /dev/null; then
  echo "debootstrap not found, please install it"
  exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

cd $(dirname $0)

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

CHROOT_CARLA_DIR="/tmp/carla-src"
PKG_FOLDER="Carla_2.0-RC1-linux"

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

if [ -d ${TARGETDIR}/chroot32 ]; then
    sudo umount -lf ${TARGETDIR}/chroot32/dev/pts || true
    sudo umount -lf ${TARGETDIR}/chroot32/sys || true
    sudo umount -lf ${TARGETDIR}/chroot32/proc || true
fi

if [ -d ${TARGETDIR}/chroot64 ]; then
    sudo umount -lf ${TARGETDIR}/chroot64/dev/pts || true
    sudo umount -lf ${TARGETDIR}/chroot64/sys || true
    sudo umount -lf ${TARGETDIR}/chroot64/proc || true
fi

if [ -d ${TARGETDIR}/chroot32 ]; then
    sudo mv ${TARGETDIR}/chroot32 ${TARGETDIR}/chroot32-deleteme
    sudo rm -rf ${TARGETDIR}/chroot32-deleteme || true
fi

if [ -d ${TARGETDIR}/chroot64 ]; then
    sudo mv ${TARGETDIR}/chroot64 ${TARGETDIR}/chroot64-deleteme
    sudo rm -rf ${TARGETDIR}/chroot64-deleteme || true
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# create chroots

if [ ! -d ${TARGETDIR}/chroot32 ]; then
    sudo debootstrap --arch=i386 lucid ${TARGETDIR}/chroot32 http://old-releases.ubuntu.com/ubuntu/
fi

if [ ! -d ${TARGETDIR}/chroot64 ]; then
    sudo debootstrap --arch=amd64 lucid ${TARGETDIR}/chroot64 http://old-releases.ubuntu.com/ubuntu/
fi

# ---------------------------------------------------------------------------------------------------------------------
# setup chroots

chroot_setup()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}

if [ ! -f ${CHROOT_DIR}/tmp/setup-aria2 ]; then
  pushd ${CHROOT_DIR}/tmp
  if [ x"${ARCH}" = x"32" ]; then
    wget -c https://github.com/q3aql/aria2-static-builds/releases/download/v1.34.0/aria2-1.34.0-linux-gnu-32bit-build1.tar.bz2
  else
    wget -c https://github.com/q3aql/aria2-static-builds/releases/download/v1.34.0/aria2-1.34.0-linux-gnu-64bit-build1.tar.bz2
  fi
  tar xf aria2-*.tar.bz2
  popd
fi

cat <<EOF | sudo chroot ${CHROOT_DIR}
mount -t proc none /proc/
mount -t sysfs none /sys/
mount -t devpts none /dev/pts
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

if [ ! -f /tmp/setup-repo ]; then
  apt-get update
  apt-get install -y python-software-properties wget
  add-apt-repository ppa:kxstudio-debian/libs
  add-apt-repository ppa:kxstudio-debian/toolchain
  apt-get update
  touch /tmp/setup-repo
fi

if [ ! -f /tmp/setup-repo-list ]; then
  echo '
deb http://old-releases.ubuntu.com/ubuntu/ lucid main restricted universe multiverse
deb http://old-releases.ubuntu.com/ubuntu/ lucid-updates main restricted universe multiverse
deb http://old-releases.ubuntu.com/ubuntu/ lucid-backports main restricted universe multiverse
' > /etc/apt/sources.list
  apt-get update
  touch /tmp/setup-repo-list
fi

if [ ! -f /tmp/setup-repo-upgrade ]; then
  dpkg-divert --local --rename --add /sbin/initctl
  ln -s /bin/true /sbin/initctl
  apt-get dist-upgrade
  touch /tmp/setup-repo-upgrade
fi

if [ ! -f /tmp/setup-repo-packages ]; then
  apt-get install -y build-essential autoconf libtool cmake libglib2.0-dev libgl1-mesa-dev git-core
  apt-get clean
  touch /tmp/setup-repo-packages
fi

if [ ! -f /tmp/setup-aria2 ]; then
  pushd /tmp/aria2-*
  make install
  popd
  rm -r /tmp/aria2-*
  touch /tmp/setup-aria2
fi

if [ ! -d ${CHROOT_CARLA_DIR} ]; then
  git clone --depth=1 git://github.com/falkTX/Carla ${CHROOT_CARLA_DIR}
fi

if [ ! -f ${CHROOT_CARLA_DIR}/source/native-plugins/external/README.md ]; then
  git clone --depth=1 git://github.com/falkTX/Carla-Plugins ${CHROOT_CARLA_DIR}/source/native-plugins/external
fi

cd ${CHROOT_CARLA_DIR}
git checkout .
git pull
git submodule update

# might be updated by git pull
chmod 777 data/linux/*.sh
chmod 777 data/linux/common.env

sync

EOF

}

export ARCH=32
chroot_setup

export ARCH=64
chroot_setup

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

chroot_build_deps()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
cp build-deps.sh common.env ${CHROOT_DIR}${CHROOT_CARLA_DIR}/data/linux/
sudo cp /etc/ca-certificates.conf ${CHROOT_DIR}/etc/
sudo cp -r /usr/share/ca-certificates/* ${CHROOT_DIR}/usr/share/ca-certificates/

cat <<EOF | sudo chroot ${CHROOT_DIR}
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

if [ ! -f /tmp/setup-repo-packages-extra1 ]; then
  if [ x"${ARCH}" != x"32" ]; then
    apt-get install -y g++-4.8-multilib ia32-libs
    apt-get clean
  fi
  touch /tmp/setup-repo-packages-extra1
fi

update-ca-certificates

${CHROOT_CARLA_DIR}/data/linux/build-deps.sh ${ARCH}

if [ ! -f /tmp/setup-repo-packages-extra2 ]; then
  apt-get install -y --no-install-recommends libasound2-dev libgtk2.0-dev libqt4-dev libx11-dev
  apt-get install -y --no-install-recommends pyqt4-dev-tools python3-pyqt4.qtopengl python3-liblo python3-sip
  apt-get install -y cx-freeze-python3 zip
  apt-get clean
  touch /tmp/setup-repo-packages-extra2
fi

EOF

}

export ARCH=32
chroot_build_deps

export ARCH=64
chroot_build_deps

# ---------------------------------------------------------------------------------------------------------------------
# build carla

chroot_build_carla()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
CHROOT_TARGET_DIR=/root/builds

cat <<EOF | sudo chroot ${CHROOT_DIR}
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

export CFLAGS="-I${CHROOT_TARGET_DIR}/carla${ARCH}/include"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-L${CHROOT_TARGET_DIR}/carla${ARCH}/lib"
export PKG_CONFIG_PATH=${CHROOT_TARGET_DIR}/carla${ARCH}/lib/pkgconfig
export RCC_QT4=/usr/bin/rcc
export LINUX=true
export DEFAULT_QT=4

cd ${CHROOT_CARLA_DIR}
make ${MAKE_ARGS}

if [ x"${ARCH}" != x"32" ]; then
  export CFLAGS="-I${CHROOT_TARGET_DIR}/carla32/include -m32"
  export CXXFLAGS=${CFLAGS}
  export LDFLAGS="-L${CHROOT_TARGET_DIR}/carla32/lib -m32"
  export PKG_CONFIG_PATH=${CHROOT_TARGET_DIR}/carla32/lib/pkgconfig
  make posix32 ${MAKE_ARGS}
fi

EOF

}

export ARCH=32
chroot_build_carla

export ARCH=64
chroot_build_carla

# ---------------------------------------------------------------------------------------------------------------------
# download carla extras

download_carla_extras()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
CARLA_VER="1.9.11+git20180916"
WINBR_VER="1.9.11+git20180916"
WINE32_VER="1.9.11+git20180916"
WINE64_VER="1.9.11.git20180916"

cat <<EOF | sudo chroot ${CHROOT_DIR}
set -e

cd ${CHROOT_CARLA_DIR}

if [ ! -d carla-pkgs${PKGS_NUM} ]; then
  mkdir -p tmp-carla-pkgs
  cd tmp-carla-pkgs
  wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win32_${WINBR_VER}_i386.deb
  wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-wine32_${WINE32_VER}_i386.deb
  if [ x"${ARCH}" != x"32" ]; then
    aria2c https://github.com/KXStudio/Repository/releases/download/initial/carla-bridge-wine64_${WINE64_VER}_amd64.deb
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win64_${WINBR_VER}_amd64.deb
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_${CARLA_VER}_amd64.deb
  else
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_${CARLA_VER}_i386.deb
  fi
  cd ..
  mv tmp-carla-pkgs carla-pkgs${PKGS_NUM}
fi

if [ ! -f carla-pkgs${PKGS_NUM}/extrated ]; then
  cd carla-pkgs${PKGS_NUM}
  dpkg -x carla-bridge-win32_${WINBR_VER}_i386.deb .
  dpkg -x carla-bridge-wine32_${WINE32_VER}_i386.deb .
  if [ x"${ARCH}" != x"32" ]; then
    dpkg -x carla-bridge-win64_${WINBR_VER}_amd64.deb .
    dpkg -x carla-bridge-wine64_${WINE64_VER}_amd64.deb .
    dpkg -x carla-git_${CARLA_VER}_amd64.deb .
  else
    dpkg -x carla-git_${CARLA_VER}_i386.deb .
  fi
  touch extrated
  cd ..
fi

if [ ! -f extra-bins/carla-bridge-win32.exe ]; then
    mkdir -p extra-bins
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*.exe  extra-bins/
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*.dll  extra-bins/
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*-gtk3 extra-bins/
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*-qt5  extra-bins/
fi

EOF

}

export ARCH=32
download_carla_extras

export ARCH=64
download_carla_extras

# ---------------------------------------------------------------------------------------------------------------------
# download carla extras

chroot_pack_carla()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
CXFREEZE_FLAGS="--include-modules=re,sip,subprocess,inspect"

cat <<EOF | sudo chroot ${CHROOT_DIR}
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

export PKG_CONFIG_PATH=${CHROOT_TARGET_DIR}/carla${ARCH}/lib/pkgconfig
export RCC_QT4=/usr/bin/rcc
export LINUX="true"

cd ${CHROOT_CARLA_DIR}
rm -rf ./tmp-install
make ${MAKE_ARGS} install DESTDIR=./tmp-install PREFIX=/usr

make -C data/windows/unzipfx-carla -f Makefile.linux ${MAKE_ARGS}
make -C data/windows/unzipfx-carla-control -f Makefile.linux ${MAKE_ARGS}

# ---------------------------------------------------------------------------------------------------------------------
# Standalone

rm -rf build-carla build-carla-control build-lv2 build-vst carla *.zip
mkdir build-carla
mkdir build-carla/resources
mkdir build-carla/src
mkdir build-carla/src/widgets

cp      extra-bins/*                               build-carla/
cp -r  ./tmp-install/usr/lib/carla/*               build-carla/
cp -LR ./tmp-install/usr/share/carla/resources/*   build-carla/resources/
cp     ./tmp-install/usr/share/carla/carla         build-carla/src/
cp     ./tmp-install/usr/share/carla/carla-control build-carla/src/
cp     ./tmp-install/usr/share/carla/*.py          build-carla/src/
cp     ./tmp-install/usr/share/carla/widgets/*.py  build-carla/src/widgets/

mv build-carla/resources/carla-plugin   build-carla/resources/carla-plugin.py
mv build-carla/resources/bigmeter-ui    build-carla/resources/bigmeter-ui.py
mv build-carla/resources/midipattern-ui build-carla/resources/midipattern-ui.py
mv build-carla/resources/notes-ui       build-carla/resources/notes-ui.py

cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/src/carla                   --target-dir=build-carla/
cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/src/carla-control           --target-dir=build-carla-control/
cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/resources/carla-plugin.py   --target-dir=build-carla/resources/
cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/resources/bigmeter-ui.py    --target-dir=build-carla/resources/
cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/resources/midipattern-ui.py --target-dir=build-carla/resources/
cxfreeze-python3 ${CXFREEZE_FLAGS} build-carla/resources/notes-ui.py       --target-dir=build-carla/resources/

cp /usr/lib/libpython3.2mu.so.1.0 build-carla/
cp /usr/lib/libffi.so.5           build-carla/
cp /usr/lib/libssl.so.0.9.8       build-carla/
cp /usr/lib/libcrypto.so.0.9.8    build-carla/
cp /lib/libbz2.so.1.0             build-carla/
cp /lib/libselinux.so.1           build-carla/
cp /root/builds/carla${ARCH}/share/misc/magic.mgc build-carla/

cp /usr/lib/libpython3.2mu.so.1.0 build-carla-control/
cp /usr/lib/libffi.so.5           build-carla-control/
cp /usr/lib/libssl.so.0.9.8       build-carla-control/
cp /usr/lib/libcrypto.so.0.9.8    build-carla-control/
cp /lib/libbz2.so.1.0             build-carla-control/
cp /lib/libselinux.so.1           build-carla-control/
cp build-carla/libcarla_utils.so  build-carla-control/
cp -r build-carla/styles          build-carla-control/

find build-carla -name "*.py" -delete
find build-carla -name PyQt4.QtAssistant.so -delete
find build-carla -name PyQt4.QtNetwork.so -delete
find build-carla -name PyQt4.QtScript.so -delete
find build-carla -name PyQt4.QtTest.so -delete
find build-carla -name PyQt4.QtXml.so -delete
rm -rf build-carla/src

find build-carla-control -name "*.py" -delete
find build-carla-control -name PyQt4.QtAssistant.so -delete
find build-carla-control -name PyQt4.QtNetwork.so -delete
find build-carla-control -name PyQt4.QtScript.so -delete
find build-carla-control -name PyQt4.QtTest.so -delete
find build-carla-control -name PyQt4.QtXml.so -delete
rm -rf build-carla-control/src

cd build-carla/resources/ && \
  rm *.so* carla-plugin-patchbay && \
  ln -s ../*.so* . && \
  ln -s carla-plugin carla-plugin-patchbay && \
cd ../..

mkdir build-lv2
cp -LR ./tmp-install/usr/lib/lv2/carla.lv2 build-lv2/
rm -r  build-lv2/carla.lv2/resources
cp -LR build-carla/resources build-lv2/carla.lv2/
cp     build-carla/magic.mgc build-lv2/carla.lv2/
cp     extra-bins/* build-lv2/carla.lv2/
rm     build-lv2/carla.lv2/resources/*carla*.so build-lv2/carla.lv2/resources/carla-plugin-patchbay
ln -s  ../libcarla_utils.so build-lv2/carla.lv2/resources/
ln -s carla-plugin build-lv2/carla.lv2/resources/carla-plugin-patchbay

mkdir build-vst
cp -LR ./tmp-install/usr/lib/vst/carla.vst build-vst/
rm -r  build-vst/carla.vst/resources
cp -LR build-carla/resources build-vst/carla.vst/
cp     build-carla/magic.mgc build-vst/carla.vst/
cp     extra-bins/* build-vst/carla.vst/
rm     build-vst/carla.vst/resources/*carla*.so build-vst/carla.vst/resources/carla-plugin-patchbay
ln -s  ../libcarla_utils.so build-vst/carla.vst/resources/
ln -s carla-plugin build-vst/carla.vst/resources/carla-plugin-patchbay

mv build-carla carla
zip --symlinks -r -9 carla.zip carla
cat data/windows/unzipfx-carla/unzipfx2cat carla.zip > Carla
chmod +x Carla
rm -rf carla carla.zip

mv build-carla-control carla-control
zip --symlinks -r -9 carla-control.zip carla-control
cat data/windows/unzipfx-carla-control/unzipfx2cat carla-control.zip > CarlaControl
chmod +x CarlaControl
rm -rf carla-control carla-control.zip

rm -rf ${PKG_FOLDER}${ARCH}
mkdir ${PKG_FOLDER}${ARCH}
cp data/linux/README ${PKG_FOLDER}${ARCH}/
mv Carla CarlaControl build-lv2/*.lv2 build-vst/*.vst ${PKG_FOLDER}${ARCH}/
tar cJf ${PKG_FOLDER}${ARCH}.tar.xz ${PKG_FOLDER}${ARCH}
mv ${PKG_FOLDER}${ARCH}.tar.xz /tmp/
rmdir build-lv2 build-vst

EOF

}

export ARCH=32
chroot_pack_carla

export ARCH=64
chroot_pack_carla

# ---------------------------------------------------------------------------------------------------------------------
