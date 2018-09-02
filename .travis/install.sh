#!/bin/bash

set -e

sudo apt-get install -y \
    g++-multilib \
    pkg-config \
    pyqt5-dev-tools \
    python3-pyqt5.qtsvg \
    python3-rdflib \
    libgtk2.0-dev \
    libgtk-3-dev \
    libqt4-dev \
    qtbase5-dev \
    libasound2-dev \
    libpulse-dev \
    libmagic-dev \
    libgl1-mesa-dev \
    libx11-dev \
    liblo-static \
    fluidsynth-static \
    apple-x86-setup \
    mingw32-x-gcc \
    mingw32-x-fluidsynth \
    mingw32-x-liblo \
    mingw32-x-pkgconfig \
    mingw64-x-gcc \
    mingw64-x-fluidsynth \
    mingw64-x-liblo \
    mingw64-x-pkgconfig \
    wine-rt-dev

mkdir /tmp/osx-macports-pkgs
cd /tmp/osx-macports-pkgs
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-flac_1.2.1-1_all.deb
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libiconv_1.14-0_all.deb
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-liblo_0.26-1_all.deb
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libogg_1.3.0-1_all.deb
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libsndfile_1.0.25-0_all.deb
wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libvorbis_1.3.3-0_all.deb
cd ~
rm -r /tmp/osx-macports-pkgs
