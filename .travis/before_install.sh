#!/bin/bash

set -e

if [ "${TARGET}" = "win32" ]; then
    sudo dpkg --add-architecture i386
fi

sudo add-apt-repository -y ppa:kxstudio-debian/kxstudio
sudo add-apt-repository -y ppa:kxstudio-debian/mingw
sudo add-apt-repository -y ppa:kxstudio-debian/toolchain

if [ "${TARGET}" = "win32" ] || [ "${TARGET}" = "win64" ]; then
    wget -qO- https://dl.winehq.org/wine-builds/winehq.key | sudo apt-key add -
    sudo apt-add-repository 'deb https://dl.winehq.org/wine-builds/ubuntu/ bionic main'
fi

sudo apt-get update -qq
sudo apt-get install kxstudio-repos
sudo apt-get update -qq
