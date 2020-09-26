#!/bin/bash

VERSION="2.3.0-alpha1"

# ---------------------------------------------------------------------------------------------------------------------
# check dependencies

if ! which debootstrap > /dev/null; then
  echo "debootstrap not found, please install it"
  exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------
# startup as main script

if [ -z "${SOURCED_BY_DOCKER}" ]; then
    # stop on error
    set -e

    # cd to correct path
    cd $(dirname $0)
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

# where we build stuff inside the chroot
CHROOT_CARLA_DIR="/tmp/carla-src"

# used for downloading packages from kxstudio repos, in order to get lv2-gtk3 and windows bridges
CARLA_GIT_VER="2.2~rc1+git20200718"
PKGS_NUM="20200718"

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
    sudo mv ${TARGETDIR}/chroot32 ${TARGETDIR}/chroot32-deleteme2
    sudo rm -rf ${TARGETDIR}/chroot32-deleteme || true
fi

if [ -d ${TARGETDIR}/chroot64 ]; then
    sudo mv ${TARGETDIR}/chroot64 ${TARGETDIR}/chroot64-deleteme2
    sudo rm -rf ${TARGETDIR}/chroot64-deleteme || true
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# create chroots

prepare()
{

if [ ! -d ${TARGETDIR}/chroot32 ]; then
    sudo debootstrap --no-check-gpg --arch=i386 lucid ${TARGETDIR}/chroot32 http://old-releases.ubuntu.com/ubuntu/
fi

if [ ! -d ${TARGETDIR}/chroot64 ]; then
    sudo debootstrap --no-check-gpg --arch=amd64 lucid ${TARGETDIR}/chroot64 http://old-releases.ubuntu.com/ubuntu/
fi

}

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
  git clone --depth=1 -b develop git://github.com/falkTX/Carla ${CHROOT_CARLA_DIR}
fi

if [ ! -f ${CHROOT_CARLA_DIR}/source/native-plugins/external/README.md ]; then
  git clone git://github.com/falkTX/Carla-Plugins ${CHROOT_CARLA_DIR}/source/native-plugins/external
fi

cd ${CHROOT_CARLA_DIR}
git checkout .
git pull
git submodule init
git submodule update

# might be updated by git pull
chmod 777 data/linux/*.sh
chmod 777 data/linux/common.env

sync

EOF

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

chroot_build_deps()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
cp build-deps.sh build-pyqt.sh common.env ${CHROOT_DIR}${CHROOT_CARLA_DIR}/data/linux/
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
  apt-get install -y --no-install-recommends libdbus-1-dev libx11-dev libffi-static
  apt-get clean
  touch /tmp/setup-repo-packages-extra2
fi

${CHROOT_CARLA_DIR}/data/linux/build-pyqt.sh ${ARCH}

if [ ! -f /tmp/setup-repo-packages-extra4 ]; then
  apt-get install -y --no-install-recommends libasound2-dev libpulse-dev libgtk2.0-dev libqt4-dev qt4-dev-tools zip unzip
  apt-get install -y --no-install-recommends libfreetype6-dev libxcursor-dev libxext-dev
  apt-get clean
  touch /tmp/setup-repo-packages-extra4
fi

EOF

}

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

export OLDPATH=\${PATH}
export CFLAGS="-I${CHROOT_TARGET_DIR}/carla${ARCH}/include"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${CHROOT_TARGET_DIR}/carla${ARCH}/lib"
export PKG_CONFIG_PATH=${CHROOT_TARGET_DIR}/carla${ARCH}/lib/pkgconfig
export LINUX=true
export MOC_QT4=/usr/bin/moc-qt4
export RCC_QT4=/usr/bin/rcc
export PYRCC5=${CHROOT_TARGET_DIR}/carla${ARCH}/bin/pyrcc5
export PYUIC5=${CHROOT_TARGET_DIR}/carla${ARCH}/bin/pyuic5

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

# ---------------------------------------------------------------------------------------------------------------------
# download carla extras

download_carla_extras()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}

cat <<EOF | sudo chroot ${CHROOT_DIR}
set -e

cd ${CHROOT_CARLA_DIR}

if [ ! -d carla-pkgs${PKGS_NUM} ]; then
  mkdir -p tmp-carla-pkgs
  cd tmp-carla-pkgs
  wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win32_${CARLA_GIT_VER}_i386.deb
  if [ x"${ARCH}" != x"32" ]; then
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-bridge-win64_${CARLA_GIT_VER}_amd64.deb
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_${CARLA_GIT_VER}_amd64.deb
  else
    wget -c https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/carla-git_${CARLA_GIT_VER}_i386.deb
  fi
  cd ..
  mv tmp-carla-pkgs carla-pkgs${PKGS_NUM}
fi

if [ ! -f carla-pkgs${PKGS_NUM}/extracted ]; then
  cd carla-pkgs${PKGS_NUM}
  ar x carla-bridge-win32_${CARLA_GIT_VER}_i386.deb
  tar xf data.tar.xz
  rm control.tar.xz data.tar.xz debian-binary
  if [ x"${ARCH}" != x"32" ]; then
    ar x carla-bridge-win64_${CARLA_GIT_VER}_amd64.deb
    tar xf data.tar.xz
    rm control.tar.xz data.tar.xz debian-binary
    ar x carla-git_${CARLA_GIT_VER}_amd64.deb
    tar xf data.tar.xz
    rm control.tar.xz data.tar.xz debian-binary
  else
    ar x carla-git_${CARLA_GIT_VER}_i386.deb
    tar xf data.tar.xz
    rm control.tar.xz data.tar.xz debian-binary
  fi
  touch extracted
  cd ..
fi

if [ ! -f extra-bins${PKGS_NUM}/carla-bridge-win32.exe ]; then
    mkdir -p extra-bins${PKGS_NUM}
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*.exe  extra-bins${PKGS_NUM}/
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*.dll  extra-bins${PKGS_NUM}/
    cp carla-pkgs${PKGS_NUM}/usr/lib/carla/*-gtk3 extra-bins${PKGS_NUM}/
fi

EOF

}

# ---------------------------------------------------------------------------------------------------------------------
# download carla extras

chroot_pack_carla()
{

CHROOT_DIR=${TARGETDIR}/chroot${ARCH}
CHROOT_TARGET_DIR=/root/builds

cat <<EOF | sudo chroot ${CHROOT_DIR}
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

export PKG_CONFIG_PATH=${CHROOT_TARGET_DIR}/carla${ARCH}/lib/pkgconfig
export PATH=${CHROOT_TARGET_DIR}/carla${ARCH}/bin:\${PATH}
export LINUX=true
export MOC_QT4=/usr/bin/moc-qt4
export RCC_QT4=/usr/bin/rcc
export PYRCC5=${CHROOT_TARGET_DIR}/carla${ARCH}/bin/pyrcc5
export PYUIC5=${CHROOT_TARGET_DIR}/carla${ARCH}/bin/pyuic5

cd ${CHROOT_CARLA_DIR}

rm -rf ./tmp-install
make ${MAKE_ARGS} install DESTDIR=./tmp-install PREFIX=/usr

make -C data/windows/unzipfx-carla -f Makefile.linux clean
make -C data/windows/unzipfx-carla-control -f Makefile.linux clean

make -C data/windows/unzipfx-carla -f Makefile.linux ${MAKE_ARGS}
make -C data/windows/unzipfx-carla-control -f Makefile.linux ${MAKE_ARGS}

# ---------------------------------------------------------------------------------------------------------------------
# Standalone

rm -rf build-carla build-carla-control build-lv2 build-vst carla carla-control *.zip

mkdir build-carla
mkdir build-carla/resources
mkdir build-carla/src
mkdir build-carla/src/modgui
mkdir build-carla/src/patchcanvas
mkdir build-carla/src/widgets

cp      extra-bins${PKGS_NUM}/*                       build-carla/
cp -r  ./tmp-install/usr/lib/carla/*                  build-carla/
cp -LR ./tmp-install/usr/share/carla/resources/*      build-carla/resources/
cp     ./tmp-install/usr/share/carla/carla            build-carla/src/
cp     ./tmp-install/usr/share/carla/carla-control    build-carla/src/
cp     ./tmp-install/usr/share/carla/*.py             build-carla/src/
cp     ./tmp-install/usr/share/carla/modgui/*.py      build-carla/src/modgui/
cp     ./tmp-install/usr/share/carla/patchcanvas/*.py build-carla/src/patchcanvas/
cp     ./tmp-install/usr/share/carla/widgets/*.py     build-carla/src/widgets/

export PYTHONPATH="./build-carla/src"

# standalone
python3 ./data/linux/app-carla.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-carla.zip

# plugins
TARGET_NAME="bigmeter-ui" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-bigmeter.zip

TARGET_NAME="midipattern-ui" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-midipattern.zip

TARGET_NAME="notes-ui" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-notes.zip

TARGET_NAME="xycontroller-ui" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-xycontroller.zip

TARGET_NAME="carla-plugin" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-carla-p1.zip

TARGET_NAME="carla-plugin-patchbay" python3 ./data/linux/app-plugin.py build_exe
mv build-carla/lib/library.zip build-carla/lib/library-carla-p2.zip

# carla-control, no merge needed
python3 ./data/linux/app-carla-control.py build_exe

# merge library stuff into one
mkdir build-carla/lib/_lib
pushd build-carla/lib/_lib
unzip -o ../library-bigmeter.zip
unzip -o ../library-midipattern.zip
unzip -o ../library-notes.zip
unzip -o ../library-xycontroller.zip
unzip -o ../library-carla-p1.zip
unzip -o ../library-carla-p2.zip
unzip -o ../library-carla.zip
zip -r -9 ../library.zip *
popd
rm -r build-carla/lib/_lib build-carla/lib/library-*.zip

# move resource binaries into right dir
mv build-carla/{bigmeter-ui,midipattern-ui,notes-ui,xycontroller-ui,carla-plugin} build-carla/resources/
rm build-carla/carla-plugin-patchbay

# symlink for carla-plugin-patchbay, lib and styles
pushd build-carla/resources
rm carla-plugin-patchbay
ln -s carla-plugin carla-plugin-patchbay
ln -s ../lib .
ln -s ../styles .
popd

# magic for filetype detection in carla
cp ${CHROOT_TARGET_DIR}/carla${ARCH}/share/misc/magic.mgc build-carla/

# binaries for carla-control
cp build-carla/libcarla_utils.so  build-carla-control/
cp -r build-carla/styles          build-carla-control/

# delete unneeded stuff
find build-* -name "*.py" -delete
find build-* -name "*.pyi" -delete
find build-* -name "libQt*" -delete
rm -f build-*/lib/PyQt5/{pylupdate,pyrcc}.so
rm -f build-*/lib/PyQt5/{QtDBus,QtNetwork,QtPrintSupport,QtSql,QtTest,QtXml}.so
rm -f build-carla/carla-bridge-lv2-modgui
rm -f build-carla/libcarla_native-plugin.so
rm -rf build-carla/src
rm -rf build-*/lib/PyQt5/uic
rmdir build-carla/resources/{modgui,patchcanvas,widgets}

mkdir build-lv2
cp -LR ./tmp-install/usr/lib/lv2/carla.lv2 build-lv2/
rm -r  build-lv2/carla.lv2/resources
cp -LR build-carla/resources build-lv2/carla.lv2/
cp     build-carla/magic.mgc build-lv2/carla.lv2/
cp     extra-bins${PKGS_NUM}/* build-lv2/carla.lv2/
rm     build-lv2/carla.lv2/resources/carla-plugin-patchbay
rm -r  build-lv2/carla.lv2/resources/styles
ln -s  ../libcarla_utils.so build-lv2/carla.lv2/resources/
ln -s  ../styles            build-lv2/carla.lv2/resources/
ln -s carla-plugin build-lv2/carla.lv2/resources/carla-plugin-patchbay

mkdir build-vst
cp -LR ./tmp-install/usr/lib/vst/carla.vst build-vst/
rm -r  build-vst/carla.vst/resources
cp -LR build-carla/resources build-vst/carla.vst/
cp     build-carla/magic.mgc build-vst/carla.vst/
cp     extra-bins${PKGS_NUM}/* build-vst/carla.vst/
rm     build-vst/carla.vst/resources/carla-plugin-patchbay
rm -r  build-vst/carla.vst/resources/styles
ln -s  ../libcarla_utils.so build-vst/carla.vst/resources/
ln -s  ../styles            build-vst/carla.vst/resources/
ln -s carla-plugin build-vst/carla.vst/resources/carla-plugin-patchbay

rm build-{lv2,vst}/carla.*/carla-bridge-lv2-modgui
rm build-{lv2,vst}/carla.*/libcarla_native-plugin.so

mv build-carla carla-${VERSION}
zip --symlinks -r -9 carla.zip carla-${VERSION}
cat data/windows/unzipfx-carla/unzipfx2cat carla.zip > Carla
chmod +x Carla
rm -rf carla carla-${VERSION} carla.zip

mv build-carla-control carla-control-${VERSION}
zip --symlinks -r -9 carla-control.zip carla-control-${VERSION}
cat data/windows/unzipfx-carla-control/unzipfx2cat carla-control.zip > CarlaControl
chmod +x CarlaControl
rm -rf carla-control carla-control-${VERSION} carla-control.zip

rm -rf ${PKG_FOLDER}${ARCH}
mkdir ${PKG_FOLDER}${ARCH}
cp data/linux/README ${PKG_FOLDER}${ARCH}/
mv Carla CarlaControl build-lv2/*.lv2 build-vst/*.vst ${PKG_FOLDER}${ARCH}/
rmdir build-lv2 build-vst
tar cJf ${PKG_FOLDER}${ARCH}.tar.xz ${PKG_FOLDER}${ARCH}
mv ${PKG_FOLDER}${ARCH}.tar.xz /tmp/

EOF

}

# ---------------------------------------------------------------------------------------------------------------------
# run the functions

if [ -z "${SOURCED_BY_DOCKER}" ]; then
    # name of final dir and xz file, needed only by chroot_pack_carla
    export PKG_FOLDER="Carla_${VERSION}-linux"

    # cleanup
    prepare

    # 32bit build
    export ARCH=32
    chroot_setup
    chroot_build_deps
    chroot_build_carla
    download_carla_extras
    chroot_pack_carla

    # 64bit build
    export ARCH=64
    chroot_setup
    chroot_build_deps
    chroot_build_carla
    download_carla_extras
    chroot_pack_carla
fi

# ---------------------------------------------------------------------------------------------------------------------
