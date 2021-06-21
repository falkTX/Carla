#!/bin/bash

set -e

if [ "${TARGET}" = "macos" ]; then
    rm -f /etc/apt/sources.list.d/*.list
fi

sudo add-apt-repository -y ppa:kxstudio-debian/kxstudio
sudo add-apt-repository -y ppa:kxstudio-debian/mingw
sudo add-apt-repository -y ppa:kxstudio-debian/toolchain

if [ "${TARGET}" = "linux" ]; then
    wget -qO- https://dl.winehq.org/wine-builds/winehq.key | sudo apt-key add -
    sudo apt-add-repository -y 'deb https://dl.winehq.org/wine-builds/ubuntu/ bionic main'
    sudo add-apt-repository -y ppa:kxstudio-debian/ubuntus
    sudo dpkg --add-architecture i386
elif [ "${TARGET}" = "linux-strict" ] || [ "${TARGET}" = "linux-juce-strict" ]; then
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
fi

sudo apt-get update -qq
sudo apt-get install kxstudio-repos
sudo apt-get update -qq
