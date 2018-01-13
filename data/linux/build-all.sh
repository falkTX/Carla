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

if [ -f Makefile ]; then
  cd data/linux
fi

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
  apt-get install python-software-properties wget
  add-apt-repository ppa:kxstudio-debian/toolchain
  apt-get update
  touch /tmp/setup-repo
fi

if [ ! -f /tmp/setup-repo-list ]; then
  echo '
deb http://old-releases.ubuntu.com/ubuntu/ lucid main restricted universe multiverse
deb http://old-releases.ubuntu.com/ubuntu/ lucid-updates main restricted universe multiverse
deb http://old-releases.ubuntu.com/ubuntu/ lucid-backports main restricted universe multiverse
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
  apt-get install build-essential libglib2.0-dev uuid-dev git-core
  apt-get install autoconf libtool
  apt-get install bison flex libxml-libxml-perl libxml-parser-perl
  rm /usr/lib/libuuid.so
  touch /tmp/setup-repo-packages
fi

if [ ! -d ${CHROOT_CARLA_DIR} ]; then
  git clone git://github.com/falkTX/Carla --depth=1 ${CHROOT_CARLA_DIR}
  chmod -R 777 ${CHROOT_CARLA_DIR}/data/linux/
fi

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
mount -t proc none /proc/
mount -t sysfs none /sys/
mount -t devpts none /dev/pts
export HOME=/root
export LANG=C
export LC_ALL=C
unset LC_TIME

${CHROOT_CARLA_DIR}/data/linux/build-deps.sh

EOF

}

export ARCH=32
chroot_build_deps

export ARCH=64
chroot_build_deps

# ---------------------------------------------------------------------------------------------------------------------
