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

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf ${TARGETDIR}/chroot32/
rm -rf ${TARGETDIR}/chroot64/

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
  apt-get dist-upgrade
  touch /tmp/setup-repo-upgrade
fi

if [ ! -f /tmp/setup-repo-packages ]; then
  apt-get install -y build-essential libglib2.0-dev uuid-dev git-core
  apt-get install -y autoconf libtool
  apt-get install -y bison flex libxml-libxml-perl libxml-parser-perl
  apt-get clean
  rm /usr/lib/libuuid.so
  touch /tmp/setup-repo-packages
fi

if [ ! -d ${CHROOT_CARLA_DIR} ]; then
  git clone git://github.com/falkTX/Carla --depth=1 ${CHROOT_CARLA_DIR}
  chmod -R 777 ${CHROOT_CARLA_DIR}/data/linux/
fi

cd ${CHROOT_CARLA_DIR}
git checkout .
git pull

# might be updated by git pull
chmod 777 data/linux/*.sh
chmod 777 data/linux/common.env

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

${CHROOT_CARLA_DIR}/data/linux/build-deps.sh ${ARCH}

if [ ! -f /tmp/setup-repo-packages-extra2 ]; then
  apt-get install -y --no-install-recommends libasound2-dev libgtk2.0-dev libqt4-dev libx11-dev
  apt-get install -y --no-install-recommends pyqt4-dev-tools python3-pyqt4.qtopengl python3-liblo python3-rdflib python3-sip
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

cat <<EOF | sudo chroot ${CHROOT_DIR}
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

set -e

export CFLAGS="-I${TARGETDIR}/carla${ARCH}/include"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-L${TARGETDIR}/carla${ARCH}/lib"
export PKG_CONFIG_PATH=${TARGETDIR}/carla${ARCH}/lib/pkgconfig
export RCC_QT4=/usr/bin/rcc
export LINUX="true"

cd ${CHROOT_CARLA_DIR}
make EXTERNAL_PLUGINS=false ${MAKE_ARGS}

if [ x"${ARCH}" != x"32" ]; then
  export CFLAGS="-I${TARGETDIR}/carla32/include -m32"
  export CXXFLAGS=${CFLAGS}
  export LDFLAGS="-L${TARGETDIR}/carla32/lib -m32"
  export PKG_CONFIG_PATH=${TARGETDIR}/carla32/lib/pkgconfig
  make posix32 ${MAKE_ARGS}
fi

EOF

}

export ARCH=32
chroot_build_carla

export ARCH=64
chroot_build_carla

# ---------------------------------------------------------------------------------------------------------------------
